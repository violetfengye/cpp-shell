/**
 * @file cd_command.cpp
 * @brief CD命令类实现
 */

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <limits.h>
#include <sys/stat.h>
#include "builtins/cd_command.h"
#include "core/shell.h"
#include "variable/variable_manager.h"
#include "utils/error.h"

namespace dash
{

    CdCommand::CdCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int CdCommand::execute(const std::vector<std::string> &args)
    {
        std::string target_dir = getTargetDirectory(args);

        // 尝试改变目录
        if (chdir(target_dir.c_str()) != 0)
        {
            std::cerr << "cd: " << target_dir << ": " << strerror(errno) << std::endl;
            return 1;
        }

        // 更新PWD和OLDPWD环境变量
        updatePwdVariables(target_dir);

        return 0;
    }

    std::string CdCommand::getName() const
    {
        return "cd";
    }

    std::string CdCommand::getHelp() const
    {
        return "cd [dir] - 改变当前工作目录";
    }

    std::string CdCommand::getTargetDirectory(const std::vector<std::string> &args)
    {
        // 如果没有参数，使用HOME目录
        if (args.size() <= 1)
        {
            std::string home = shell_->getVariableManager()->get("HOME");
            if (home.empty())
            {
                throw ShellException(ExceptionType::RUNTIME, "cd: HOME not set");
            }
            return home;
        }

        // 如果参数是 "-"，使用OLDPWD目录
        if (args[1] == "-")
        {
            std::string oldpwd = shell_->getVariableManager()->get("OLDPWD");
            if (oldpwd.empty())
            {
                throw ShellException(ExceptionType::RUNTIME, "cd: OLDPWD not set");
            }
            std::cout << oldpwd << std::endl;
            return oldpwd;
        }

        // 否则使用指定的目录
        return args[1];
    }

    void CdCommand::updatePwdVariables(const std::string &new_dir)
    {
        char cwd[PATH_MAX];

        // 获取当前工作目录
        if (getcwd(cwd, sizeof(cwd)) == nullptr)
        {
            std::cerr << "cd: 无法获取当前工作目录: " << strerror(errno) << std::endl;
            return;
        }

        // 保存旧的PWD到OLDPWD
        std::string old_pwd = shell_->getVariableManager()->get("PWD");
        if (!old_pwd.empty())
        {
            shell_->getVariableManager()->set("OLDPWD", old_pwd);
        }

        // 更新PWD
        shell_->getVariableManager()->set("PWD", cwd);
    }

} // namespace dash