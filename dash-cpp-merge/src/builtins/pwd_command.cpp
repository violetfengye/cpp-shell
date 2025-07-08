/**
 * @file pwd_command.cpp
 * @brief PWD命令实现
 */

#include "builtins/pwd_command.h"
#include "core/shell.h"
#include "variable/variable_manager.h"
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>

namespace dash
{

    PwdCommand::PwdCommand(Shell *shell)
        : BuiltinCommand(shell, "pwd")
    {
    }

    PwdCommand::~PwdCommand()
    {
    }

    int PwdCommand::execute(const std::vector<std::string> &args)
    {
        bool physical = true; // 默认使用物理路径
        
        // 解析选项
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "-L" || args[i] == "--logical") {
                physical = false;
            } else if (args[i] == "-P" || args[i] == "--physical") {
                physical = true;
            } else if (args[i] == "--help") {
                std::cout << getHelp() << std::endl;
                return 0;
            } else {
                std::cerr << "pwd: 未知选项: " << args[i] << std::endl;
                std::cerr << "尝试 'pwd --help' 获取更多信息。" << std::endl;
                return 1;
            }
        }

        if (!physical) {
            // 使用逻辑路径（从PWD环境变量获取）
            VariableManager *var_manager = shell_->getVariableManager();
            if (var_manager) {
                std::string pwd = var_manager->getVariable("PWD");
                if (!pwd.empty()) {
                    std::cout << pwd << std::endl;
                    return 0;
                }
            }
        }

        // 使用物理路径（从系统获取）
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << cwd << std::endl;
            return 0;
        } else {
            std::cerr << "pwd: 无法获取当前工作目录: " << strerror(errno) << std::endl;
            return 1;
        }
    }

    std::string PwdCommand::getHelp() const
    {
        return "pwd [选项]\n"
               "  打印当前工作目录。\n"
               "  选项:\n"
               "    -L, --logical    使用环境变量PWD中的值，可能包含符号链接\n"
               "    -P, --physical   使用物理目录结构，解析所有符号链接（默认）\n"
               "    --help           显示此帮助信息";
    }

} // namespace dash 