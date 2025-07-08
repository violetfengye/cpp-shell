/**
 * @file help_command.cpp
 * @brief Help命令实现
 */

#include "builtins/help_command.h"
#include "builtins/builtin_command.h"
#include "core/shell.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <map>

namespace dash
{
    // 由于Shell类中没有直接提供获取所有内置命令的方法，我们需要自己实现
    // 这些内置命令在Shell类的registerBuiltinCommands()方法中注册
    static std::vector<std::string> getBuiltinCommandNames()
    {
        std::vector<std::string> commands = {
            "cd", "echo", "exit", "pwd", "jobs", "fg", "bg", 
            "kill", "wait", "debug", "help", "history", "source"
        };
        return commands;
    }

    HelpCommand::HelpCommand(Shell *shell)
        : BuiltinCommand(shell, "help")
    {
    }

    HelpCommand::~HelpCommand()
    {
    }

    int HelpCommand::execute(const std::vector<std::string> &args)
    {
        if (args.size() <= 1) {
            // 显示所有内置命令的简要帮助
            printAllCommands();
        } else {
            // 显示指定命令的详细帮助
            std::string command_name = args[1];
            printCommandHelp(command_name);
        }

        return 0;
    }

    void HelpCommand::printAllCommands() const
    {
        // 获取所有内置命令
        std::vector<std::string> command_names = getBuiltinCommandNames();
        
        // 按字母顺序排序
        std::sort(command_names.begin(), command_names.end());

        std::cout << "Dash-CPP-Merge Shell 内置命令:" << std::endl;
        std::cout << "使用 'help 命令名' 获取有关特定命令的更多信息。" << std::endl;
        std::cout << std::endl;

        // 计算最长命令名的长度，用于对齐
        size_t max_length = 0;
        for (const auto &name : command_names) {
            max_length = std::max(max_length, name.length());
        }

        // 输出命令名和简要描述
        for (const auto &name : command_names) {
            // 尝试获取命令实例并获取其帮助信息
            BuiltinCommand* cmd = nullptr;
            
            // 这里我们使用一个简单的描述，因为我们无法直接获取命令实例
            std::string brief;
            
            if (name == "cd") {
                brief = "更改当前工作目录";
            } else if (name == "echo") {
                brief = "显示一行文本";
            } else if (name == "exit") {
                brief = "退出Shell";
            } else if (name == "pwd") {
                brief = "打印当前工作目录";
            } else if (name == "jobs") {
                brief = "列出活动作业";
            } else if (name == "fg") {
                brief = "将作业移至前台";
            } else if (name == "bg") {
                brief = "将作业移至后台";
            } else if (name == "kill") {
                brief = "向进程发送信号";
            } else if (name == "wait") {
                brief = "等待作业完成";
            } else if (name == "debug") {
                brief = "调试Shell";
            } else if (name == "help") {
                brief = "显示帮助信息";
            } else if (name == "history") {
                brief = "显示命令历史";
            } else if (name == "source") {
                brief = "执行脚本文件";
            } else {
                brief = "未知命令";
            }
            
            std::cout << "  " << std::left << std::setw(max_length + 2) << name;
            std::cout << brief << std::endl;
        }
    }

    void HelpCommand::printCommandHelp(const std::string &command_name) const
    {
        // 检查命令是否存在
        std::vector<std::string> command_names = getBuiltinCommandNames();
        bool command_exists = false;
        for (const auto &name : command_names) {
            if (name == command_name) {
                command_exists = true;
                break;
            }
        }
        
        if (!command_exists) {
            std::cerr << "help: 未找到内置命令: " << command_name << std::endl;
            return;
        }

        // 获取命令的帮助信息
        std::string help;
        
        // 由于我们无法直接获取命令实例，我们提供一些基本的帮助信息
        if (command_name == "cd") {
            help = "cd [目录]\n"
                   "  更改当前工作目录到指定目录。\n"
                   "  如果没有指定目录，则切换到用户主目录。";
        } else if (command_name == "echo") {
            help = "echo [参数...]\n"
                   "  显示参数，以空格分隔。\n"
                   "  支持转义字符和变量替换。";
        } else if (command_name == "exit") {
            help = "exit [状态码]\n"
                   "  退出Shell，状态码为N；如果N省略，则状态码为最后一条执行的命令的退出状态。";
        } else if (command_name == "pwd") {
            help = "pwd\n"
                   "  打印当前工作目录的绝对路径。";
        } else if (command_name == "jobs") {
            help = "jobs [-l] [-p] [-r] [-s]\n"
                   "  列出活动作业。\n"
                   "  选项:\n"
                   "    -l  显示进程ID\n"
                   "    -p  仅显示进程ID\n"
                   "    -r  仅显示运行中的作业\n"
                   "    -s  仅显示已停止的作业";
        } else if (command_name == "fg") {
            help = "fg [作业ID]\n"
                   "  将作业移至前台运行。\n"
                   "  作业ID可以是以下形式:\n"
                   "    %N        作业号为N的作业\n"
                   "    %+, %%    当前作业\n"
                   "    %-        上一个作业\n"
                   "    %string   命令以string开头的作业\n"
                   "    N         作业号为N的作业\n"
                   "  如果不指定作业ID，则默认使用当前作业。";
        } else if (command_name == "bg") {
            help = "bg [作业ID]\n"
                   "  将已停止的作业在后台继续运行。\n"
                   "  作业ID可以是以下形式:\n"
                   "    %N        作业号为N的作业\n"
                   "    %+, %%    当前作业\n"
                   "    %-        上一个作业\n"
                   "    %string   命令以string开头的作业\n"
                   "    N         作业号为N的作业\n"
                   "  如果不指定作业ID，则默认使用当前作业。";
        } else if (command_name == "kill") {
            help = "kill [-s 信号] PID或作业ID...\n"
                   "  向进程或作业发送信号。\n"
                   "  选项:\n"
                   "    -s 信号  指定要发送的信号，可以是信号名或信号编号\n"
                   "  如果不指定信号，则默认发送SIGTERM信号。";
        } else if (command_name == "wait") {
            help = "wait [PID或作业ID...]\n"
                   "  等待指定的进程或作业完成，并返回退出状态。\n"
                   "  如果不指定参数，则等待所有后台作业完成。";
        } else if (command_name == "debug") {
            help = "debug [子命令]\n"
                   "  调试工具。\n"
                   "  子命令:\n"
                   "    on, enable      启用调试模式\n"
                   "    off, disable    禁用调试模式\n"
                   "    show, status    显示当前调试状态\n"
                   "    vars, variables 显示所有变量\n"
                   "    jobs            显示作业信息\n"
                   "    help            显示此帮助信息\n"
                   "  如果不指定子命令，则显示当前调试状态。";
        } else if (command_name == "help") {
            help = "help [命令名]\n"
                   "  显示内置命令的帮助信息。\n"
                   "  如果指定了命令名，则显示该命令的详细帮助信息。\n"
                   "  否则，显示所有内置命令的简要帮助。";
        } else if (command_name == "history") {
            help = "history [选项] [参数]\n"
                   "  显示命令历史列表。\n"
                   "  选项:\n"
                   "    -c  清除历史列表\n"
                   "    -d 偏移量  删除指定位置的历史条目\n"
                   "    -n 数量  显示最近的n条历史记录\n"
                   "  如果不指定选项，则显示所有历史记录。";
        } else if (command_name == "source") {
            help = "source 文件名 [参数...]\n"
                   "  执行指定的脚本文件。\n"
                   "  可以传递参数给脚本文件。";
        } else {
            help = command_name + ": 没有帮助信息可用";
        }
        
        std::cout << help << std::endl;
    }

    std::string HelpCommand::getHelp() const
    {
        return "help [命令名]\n"
               "  显示内置命令的帮助信息。\n"
               "  如果指定了命令名，则显示该命令的详细帮助信息。\n"
               "  否则，显示所有内置命令的简要帮助。";
    }

} // namespace dash 