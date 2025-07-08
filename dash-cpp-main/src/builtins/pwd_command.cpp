/**
 * @file pwd_command.cpp
 * @brief Pwd命令类实现
 */

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <getopt.h>
#include <vector>
#include "builtins/pwd_command.h"
#include "core/shell.h"
#include "variable/variable_manager.h"
#include "utils/error.h"

namespace dash
{

    PwdCommand::PwdCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int PwdCommand::execute(const std::vector<std::string> &args)
    {
        bool physical = false; // 默认使用逻辑路径

        // 手动解析选项，避免使用 getopt
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string &arg = args[i];
            if (arg[0] == '-') {
                for (size_t j = 1; j < arg.size(); ++j) {
                    switch (arg[j]) {
                    case 'L':
                        physical = false;
                        break;
                    case 'P':
                        physical = true;
                        break;
                    default:
                        std::cerr << "pwd: 无效选项: -" << arg[j] << std::endl;
                        std::cerr << "pwd: 用法: pwd [-LP]" << std::endl;
                        return 1;
                    }
                }
            } else {
                std::cerr << "pwd: 无效参数: " << arg << std::endl;
                std::cerr << "pwd: 用法: pwd [-LP]" << std::endl;
                return 1;
            }
        }

        // 获取当前工作目录
        std::string cwd = getCurrentDirectory(physical);

        if (cwd.empty())
        {
            std::cerr << "pwd: 无法获取当前工作目录" << std::endl;
            return 1;
        }

        std::cout << cwd << std::endl;
        return 0;
    }

    std::string PwdCommand::getName() const
    {
        return "pwd";
    }

    std::string PwdCommand::getHelp() const
    {
        return "pwd [-LP] - 显示当前工作目录";
    }

    std::string PwdCommand::getCurrentDirectory(bool physical)
    {
        if (!physical)
        {
            // 使用逻辑路径（使用PWD环境变量）
            std::string pwd = shell_->getVariableManager()->get("PWD");
            if (!pwd.empty())
            {
                return pwd;
            }
        }

        // 使用物理路径或PWD不可用
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == nullptr)
        {
            return "";
        }

        return cwd;
    }

} // namespace dash