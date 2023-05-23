/**
 * @file WindowsVersion.cpp
 * @author Christian Schenk
 * @brief Windows version information
 *
 * @copyright Copyright © 1996-2023 Christian Schenk
 *
 * This file is part of the MiKTeX Core Library.
 *
 * The MiKTeX Core Library is licensed under GNU General Public License version
 * 2 or any later version.
 */

#include "config.h"

#include <miktex/Core/win/WindowsVersion>

#include "internal.h"

using namespace std;

using namespace MiKTeX::Core;

class LazyWindowsVersion
{

public:

    operator DWORD()
    {
        if (windowsVersion == 0)
        {
            // TODO: atomic
            windowsVersion = ::GetVersion();
        }
        return windowsVersion;
    }

private:

    DWORD windowsVersion = 0;
};

LazyWindowsVersion windowsVersion;

string WindowsVersion::GetMajorMinorBuildString()
{
    unsigned major = LOBYTE(LOWORD(windowsVersion));
    unsigned minor = HIBYTE(LOWORD(windowsVersion));
    unsigned build = 0;
    if (windowsVersion < 0x80000000)
    {
        build = HIWORD(windowsVersion);
    }
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build);
}

string WindowsVersion::GetMajorMinorString()
{
    unsigned major = LOBYTE(LOWORD(windowsVersion));
    unsigned minor = HIBYTE(LOWORD(windowsVersion));
    return std::to_string(major) + "." + std::to_string(minor);
}
