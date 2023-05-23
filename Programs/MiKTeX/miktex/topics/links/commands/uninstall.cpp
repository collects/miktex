/**
 * @file topics/links/commands/uninstall.cpp
 * @author Christian Schenk
 * @brief links uninstall
 *
 * @copyright Copyright © 2021-2023 Christian Schenk
 *
 * This file is part of One MiKTeX Utility.
 *
 * One MiKTeX Utility is licensed under GNU General Public
 * License version 2 or any later version.
 */

#include <config.h>

#include <memory>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <miktex/Wrappers/PoptWrapper>

#include "internal.h"

#include "commands.h"

#include "LinksManager.h"

namespace
{
    class UninstallCommand :
        public OneMiKTeXUtility::Topics::Command
    {
        std::string Description() override
        {
            return T_("Uninstall all links");
        }

        int MIKTEXTHISCALL Execute(OneMiKTeXUtility::ApplicationContext& ctx, const std::vector<std::string>& arguments) override;

        std::string Name() override
        {
            return "uninstall";
        }

        std::string Synopsis() override
        {
            return "uninstall";
        }
    };
}

using namespace std;

using namespace MiKTeX::Wrappers;

using namespace OneMiKTeXUtility;
using namespace OneMiKTeXUtility::Topics;
using namespace OneMiKTeXUtility::Topics::Links;

unique_ptr<Command> Commands::Uninstall()
{
    return make_unique<UninstallCommand>();
}

enum Option
{
    OPT_AAA = 1,
};

static const struct poptOption uninstall_options[] =
{
    POPT_AUTOHELP
    POPT_TABLEEND
};

int UninstallCommand::Execute(ApplicationContext& ctx, const vector<string>& arguments)
{
    if (ctx.session->IsSharedSetup() && !ctx.session->IsAdminMode())
    {
        ctx.ui->FatalError(T_("this command must be run in admin mode"));
    }
    auto argv = MakeArgv(arguments);
    PoptWrapper popt(static_cast<int>(argv.size() - 1), &argv[0], uninstall_options);
    int option;
    while ((option = popt.GetNextOpt()) >= 0)
    {
    }
    if (option != -1)
    {
        ctx.ui->IncorrectUsage(fmt::format("{0}: {1}", popt.BadOption(POPT_BADOPTION_NOALIAS), popt.Strerror(option)));
    }
    if (!popt.GetLeftovers().empty())
    {
        ctx.ui->IncorrectUsage(T_("unexpected command arguments"));
    }
    LinksManager mgr;
    mgr.Init(ctx);
    mgr.Uninstall();
    return 0;
}
