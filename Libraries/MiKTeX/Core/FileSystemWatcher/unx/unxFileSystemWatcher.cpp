/**
 * @file unxFileSystemWatcher.cpp
 * @author Christian Schenk
 * @brief File system watcher (Linux)
 *
 * @copyright Copyright © 2021-2023 Christian Schenk
 *
 * This file is part of the MiKTeX Core Library.
 *
 * The MiKTeX Core Library is licensed under GNU General Public License version
 * 2 or any later version.
 */

#include "config.h"

#include <unistd.h>
#include <sys/inotify.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "internal.h"

#include "unxFileSystemWatcher.h"

using namespace std;

using namespace MiKTeX::Core;
using namespace MiKTeX::Util;

unique_ptr<FileSystemWatcher> FileSystemWatcher::Create()
{
    return make_unique<unxFileSystemWatcher>();
}

unxFileSystemWatcher::unxFileSystemWatcher()
{
    watchFd = inotify_init();
    if (watchFd < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("inotify_init");
    }
}

unxFileSystemWatcher::~unxFileSystemWatcher()
{
    try
    {
        Stop();
        if (close(watchFd) < 0)
        {
            MIKTEX_FATAL_CRT_ERROR("close");
        }
    }
    catch (const exception&)
    {
    }
}

void unxFileSystemWatcher::AddDirectories(const vector<PathName>& directories)
{
    unique_lock<shared_mutex> l(mutex);
    for (const auto& dir : directories)
    {
        int wd = inotify_add_watch(watchFd, dir.GetData(), IN_ALL_EVENTS);
        if (wd < 0)
        {
            MIKTEX_FATAL_CRT_ERROR_2("inotify_add_Watch", "path", dir.ToString());
        }
        if (this->directories.find(wd) != this->directories.end())
        {
            continue;
        }
        trace_files->WriteLine("core", fmt::format("watching directory: {0}", Q_(dir.ToDisplayString())));
        this->directories[wd] = dir;
    }
}

bool unxFileSystemWatcher::Start()
{
    bool runningExpected = false;
    if (!running.compare_exchange_strong(runningExpected, true))
    {
        return false;
    }
    if (pipe(cancelEventPipe) != 0)
    {
        MIKTEX_FATAL_CRT_ERROR("pipe");
    }
    StartThreads();
    return true;
}

bool unxFileSystemWatcher::Stop()
{
    bool runningExpected = true;
    if (!running.compare_exchange_strong(runningExpected, false))
    {
        return false;
    }
    char buf[1];
    if (write(cancelEventPipe[1], buf, 1) < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("write");
    }
    StopThreads();
    if (close(cancelEventPipe[0]) < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("close");
    }
    if (close(cancelEventPipe[1]) < 0)
    {
        MIKTEX_FATAL_CRT_ERROR("close");
    }
    return true;
}

void unxFileSystemWatcher::WatchDirectories()
{
    vector<unsigned char> buffer;
    buffer.resize(4096);
    while (true)
    {
        int maxFd = -1;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(cancelEventPipe[0], &readfds);
        if (cancelEventPipe[0] > maxFd)
        {
            maxFd = cancelEventPipe[0];
        }
        FD_SET(watchFd, &readfds);
        if (watchFd > maxFd)
        {
            maxFd = watchFd;
        }
        if (select(maxFd + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
            MIKTEX_FATAL_CRT_ERROR("select");
        }
        if (FD_ISSET(watchFd, &readfds))
        {
            auto n = read(watchFd, &buffer[0], buffer.size());
            if (n < 0)
            {
                MIKTEX_FATAL_CRT_ERROR("read");
            }
            for (size_t idx = 0; idx < n;)
            {
                const struct inotify_event* evt = reinterpret_cast<const struct inotify_event*>(&buffer[idx]);
                HandleDirectoryChange(evt);
                idx += sizeof(struct inotify_event) + evt->len;
            }
            notifyCondition.notify_all();
        }
        if (FD_ISSET(cancelEventPipe[0], &readfds))
        {
            return;
        }
    }
}

void unxFileSystemWatcher::HandleDirectoryChange(const inotify_event* evt)
{
    FileSystemChangeEvent ev;
    if ((evt->mask & IN_CREATE) != 0)
    {
        ev.action = FileSystemChangeAction::Added;
    }
    else if ((evt->mask & IN_DELETE) != 0)
    {
        ev.action = FileSystemChangeAction::Removed;
    }
    else if ((evt->mask & IN_MODIFY) != 0)
    {
        ev.action = FileSystemChangeAction::Modified;
    }
    else
    {
        return;
    }
    unique_lock<shared_mutex> l(mutex);
    const auto& it = directories.find(evt->wd);
    if (it == directories.end())
    {
        return;
    }
    PathName dir = it->second;
    l.unlock();
    ev.fileName = dir;
    ev.fileName /= evt->name;
    lock_guard<std::mutex> l2(notifyMutex);
    pendingNotifications.push_back(ev);
}
