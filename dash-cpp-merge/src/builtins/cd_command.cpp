/**
 * @file cd_command.cpp
 * @brief CD命令实现
 */

#include "builtins/cd_command.h"
#include "core/shell.h"
#include "variable/variable_manager.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <limits.h>
#include <pwd.h>

namespace dash
{

    CdCommand::CdCommand(Shell *shell)
        : BuiltinCommand(shell, "cd")
    {
    }

    CdCommand::~CdCommand()
    {
    }

    int CdCommand::execute(const std::vector<std::string> &args)
    {
        // 获取变量管理器
        VariableManager *var_manager = shell_->getVariableManager();
        if (!var_manager) {
            std::cerr << "cd: 无法获取变量管理器" << std::endl;
            return 1;
        }

        // 确定目标目录
        std::string target_dir;

        if (args.size() <= 1) {
            // cd 不带参数，切换到HOME目录
            target_dir = var_manager->getVariable("HOME");
            if (target_dir.empty()) {
                std::cerr << "cd: HOME未设置" << std::endl;
                return 1;
            }
        } else if (args[1] == "-") {
            // cd - 切换到上一个目录
            target_dir = var_manager->getVariable("OLDPWD");
            if (target_dir.empty()) {
                std::cerr << "cd: OLDPWD未设置" << std::endl;
                return 1;
            }
            std::cout << target_dir << std::endl;
        } else if (args[1] == "~") {
            // cd ~ 切换到HOME目录
            target_dir = var_manager->getVariable("HOME");
            if (target_dir.empty()) {
                std::cerr << "cd: HOME未设置" << std::endl;
                return 1;
            }
        } else if (args[1][0] == '~' && args[1][1] == '/') {
            // cd ~/path 切换到HOME/path目录
            std::string home = var_manager->getVariable("HOME");
            if (home.empty()) {
                std::cerr << "cd: HOME未设置" << std::endl;
                return 1;
            }
            target_dir = home + args[1].substr(1);
        } else if (args[1][0] == '~' && args[1].length() > 1) {
            // cd ~user 切换到user的HOME目录
            std::string username = args[1].substr(1);
            size_t slash_pos = username.find('/');
            std::string path_suffix;
            
            if (slash_pos != std::string::npos) {
                path_suffix = username.substr(slash_pos);
                username = username.substr(0, slash_pos);
            }
            
            struct passwd *pw = getpwnam(username.c_str());
            if (!pw) {
                std::cerr << "cd: 用户 " << username << " 不存在" << std::endl;
                return 1;
            }
            
            target_dir = pw->pw_dir + path_suffix;
        } else {
            // cd path 切换到指定目录
            target_dir = args[1];
        }

        // 保存当前目录
        char current_dir[PATH_MAX];
        if (getcwd(current_dir, sizeof(current_dir)) == nullptr) {
            std::cerr << "cd: 无法获取当前工作目录: " << strerror(errno) << std::endl;
            return 1;
        }

        // 切换目录
        if (chdir(target_dir.c_str()) != 0) {
            std::cerr << "cd: " << target_dir << ": " << strerror(errno) << std::endl;
            return 1;
        }

        // 获取新目录的绝对路径
        char new_dir[PATH_MAX];
        if (getcwd(new_dir, sizeof(new_dir)) == nullptr) {
            std::cerr << "cd: 无法获取新工作目录: " << strerror(errno) << std::endl;
            return 1;
        }

        // 更新PWD和OLDPWD环境变量
        var_manager->setVariable("OLDPWD", current_dir);
        var_manager->setVariable("PWD", new_dir);

        return 0;
    }

    std::string CdCommand::getHelp() const
    {
        return "cd [目录]\n"
               "  切换当前工作目录。\n"
               "  如果不指定目录，则切换到HOME目录。\n"
               "  如果目录是'-'，则切换到上一个工作目录。\n"
               "  如果目录是'~'或'~/'，则切换到HOME目录。\n"
               "  如果目录是'~user'，则切换到user的HOME目录。";
    }

} // namespace dash 