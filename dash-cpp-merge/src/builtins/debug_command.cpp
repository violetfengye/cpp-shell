/**
 * @file debug_command.cpp
 * @brief Debug命令实现
 */

#include "builtins/debug_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include "variable/variable_manager.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>

namespace dash
{
    // 调试模式标志（因为Shell类中没有调试模式相关方法，我们在这里添加一个静态变量）
    static bool debug_mode = false;

    DebugCommand::DebugCommand(Shell *shell)
        : BuiltinCommand(shell, "debug")
    {
    }

    DebugCommand::~DebugCommand()
    {
    }

    int DebugCommand::execute(const std::vector<std::string> &args)
    {
        if (args.size() <= 1) {
            // 显示当前调试状态
            showDebugStatus();
            return 0;
        }

        // 解析命令
        std::string subcommand = args[1];
        
        if (subcommand == "on" || subcommand == "enable") {
            // 启用调试模式
            debug_mode = true;
            std::cout << "调试模式已启用" << std::endl;
        } else if (subcommand == "off" || subcommand == "disable") {
            // 禁用调试模式
            debug_mode = false;
            std::cout << "调试模式已禁用" << std::endl;
        } else if (subcommand == "show" || subcommand == "status") {
            // 显示调试状态
            showDebugStatus();
        } else if (subcommand == "vars" || subcommand == "variables") {
            // 显示所有变量
            showVariables();
        } else if (subcommand == "jobs") {
            // 显示作业信息
            showJobs();
        } else if (subcommand == "help") {
            // 显示帮助信息
            std::cout << getHelp() << std::endl;
        } else {
            // 未知子命令
            std::cerr << "debug: 未知子命令: " << subcommand << std::endl;
            std::cerr << "尝试 'debug help' 获取更多信息。" << std::endl;
            return 1;
        }

        return 0;
    }

    void DebugCommand::showDebugStatus() const
    {
        std::cout << "调试模式: " << (debug_mode ? "启用" : "禁用") << std::endl;
    }

    void DebugCommand::showVariables() const
    {
        // 获取变量管理器
        VariableManager *var_manager = shell_->getVariableManager();
        if (!var_manager) {
            std::cerr << "debug: 无法获取变量管理器" << std::endl;
            return;
        }

        // 获取所有变量
        std::map<std::string, std::string> variables = var_manager->getAllVariables();

        // 计算最长变量名的长度，用于对齐
        size_t max_length = 0;
        for (const auto &var : variables) {
            max_length = std::max(max_length, var.first.length());
        }

        // 输出变量
        std::cout << "变量列表:" << std::endl;
        for (const auto &var : variables) {
            std::cout << "  " << std::left << std::setw(max_length + 2) << var.first;
            std::cout << "= " << var.second << std::endl;
        }
    }

    void DebugCommand::showJobs() const
    {
        // 获取作业控制器
        JobControl *job_control = shell_->getJobControl();
        if (!job_control) {
            std::cerr << "debug: 无法获取作业控制器" << std::endl;
            return;
        }

        // 输出作业信息
        std::cout << "作业列表:" << std::endl;
        
        bool has_jobs = false;
        
        // 遍历所有可能的作业ID
        for (int i = 1; i <= 100; i++) {
            Job* job = job_control->getJobByJobId(i);
            if (job) {
                has_jobs = true;
                
                std::cout << "  [" << job->getJobId() << "] ";
                
                // 输出状态
                switch (job->getState()) {
                case JobState::RUNNING:
                    std::cout << "运行中    ";
                    break;
                case JobState::STOPPED:
                    std::cout << "已停止    ";
                    break;
                case JobState::DONE:
                    std::cout << "已完成    ";
                    break;
                default:
                    std::cout << "未知状态  ";
                    break;
                }
                
                // 输出进程组ID和命令
                std::cout << "进程组: " << job->getPid() << "    ";
                std::cout << job->getCommand() << std::endl;
            }
        }
        
        if (!has_jobs) {
            std::cout << "  无作业" << std::endl;
        }
    }

    std::string DebugCommand::getHelp() const
    {
        return "debug [子命令]\n"
               "  调试工具。\n"
               "  子命令:\n"
               "    on, enable      启用调试模式\n"
               "    off, disable    禁用调试模式\n"
               "    show, status    显示当前调试状态\n"
               "    vars, variables 显示所有变量\n"
               "    jobs            显示作业信息\n"
               "    help            显示此帮助信息\n"
               "  如果不指定子命令，则显示当前调试状态。";
    }

} // namespace dash 