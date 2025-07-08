/**
 * @file debug_command.cpp
 * @brief 调试命令类实现
 */

#include "../../include/builtins/debug_command.h"
#include "../../include/core/shell.h"
#include <iostream>
#include <iomanip>

namespace dash
{
    // 初始化静态成员变量 - 默认为关闭状态
    bool DebugCommand::debug_enabled_ = false;
    bool DebugCommand::command_debug_enabled_ = false;
    bool DebugCommand::parser_debug_enabled_ = false;
    bool DebugCommand::executor_debug_enabled_ = false;
    bool DebugCommand::completion_debug_enabled_ = false;
    
    DebugCommand::DebugCommand(Shell* shell)
        : BuiltinCommand(shell)
    {
    }
    
    void DebugCommand::showHelp() const
    {
        std::cout << "debug [选项]" << std::endl;
        std::cout << "  控制调试信息的显示。" << std::endl;
        std::cout << "  选项:" << std::endl;
        std::cout << "    on/off          - 开启/关闭所有调试信息" << std::endl;
        std::cout << "    status          - 显示当前调试状态" << std::endl;
        std::cout << "    command on/off  - 开启/关闭命令调试信息" << std::endl;
        std::cout << "    parser on/off   - 开启/关闭解析器调试信息" << std::endl;
        std::cout << "    executor on/off - 开启/关闭执行器调试信息" << std::endl;
        std::cout << "    completion on/off - 开启/关闭补全调试信息" << std::endl;
        std::cout << "  示例:" << std::endl;
        std::cout << "    debug on        - 开启所有调试信息" << std::endl;
        std::cout << "    debug command on - 只开启命令调试信息" << std::endl;
        std::cout << "    debug status    - 显示当前调试状态" << std::endl;
    }
    
    void DebugCommand::showStatus() const
    {
        std::cout << "调试状态:" << std::endl;
        std::cout << "  全局调试模式:   " << (debug_enabled_ ? "开启" : "关闭") << std::endl;
        std::cout << "  命令调试模式:   " << (command_debug_enabled_ ? "开启" : "关闭") << std::endl;
        std::cout << "  解析器调试模式: " << (parser_debug_enabled_ ? "开启" : "关闭") << std::endl;
        std::cout << "  执行器调试模式: " << (executor_debug_enabled_ ? "开启" : "关闭") << std::endl;
        std::cout << "  补全调试模式:   " << (completion_debug_enabled_ ? "开启" : "关闭") << std::endl;
    }
    
    int DebugCommand::execute(const std::vector<std::string>& args)
    {
        if (args.size() == 1)
        {
            // 没有参数，显示帮助信息
            showHelp();
            return 0;
        }
        
        if (args.size() >= 2)
        {
            if (args[1] == "on")
            {
                // 开启所有调试模式
                debug_enabled_ = true;
                command_debug_enabled_ = true;
                parser_debug_enabled_ = true;
                executor_debug_enabled_ = true;
                completion_debug_enabled_ = true;
                std::cout << "已开启所有调试信息" << std::endl;
                return 0;
            }
            else if (args[1] == "off")
            {
                // 关闭所有调试模式
                debug_enabled_ = false;
                command_debug_enabled_ = false;
                parser_debug_enabled_ = false;
                executor_debug_enabled_ = false;
                completion_debug_enabled_ = false;
                std::cout << "已关闭所有调试信息" << std::endl;
                return 0;
            }
            else if (args[1] == "status")
            {
                // 显示当前调试状态
                showStatus();
                return 0;
            }
            else if (args[1] == "command" && args.size() >= 3)
            {
                // 控制命令调试模式
                if (args[2] == "on")
                {
                    command_debug_enabled_ = true;
                    debug_enabled_ = true;  // 至少有一个模式开启时，全局调试也开启
                    std::cout << "已开启命令调试信息" << std::endl;
                    return 0;
                }
                else if (args[2] == "off")
                {
                    command_debug_enabled_ = false;
                    // 检查是否所有模式都关闭了
                    if (!parser_debug_enabled_ && !executor_debug_enabled_ && !completion_debug_enabled_)
                    {
                        debug_enabled_ = false;
                    }
                    std::cout << "已关闭命令调试信息" << std::endl;
                    return 0;
                }
            }
            else if (args[1] == "parser" && args.size() >= 3)
            {
                // 控制解析器调试模式
                if (args[2] == "on")
                {
                    parser_debug_enabled_ = true;
                    debug_enabled_ = true;  // 至少有一个模式开启时，全局调试也开启
                    std::cout << "已开启解析器调试信息" << std::endl;
                    return 0;
                }
                else if (args[2] == "off")
                {
                    parser_debug_enabled_ = false;
                    // 检查是否所有模式都关闭了
                    if (!command_debug_enabled_ && !executor_debug_enabled_ && !completion_debug_enabled_)
                    {
                        debug_enabled_ = false;
                    }
                    std::cout << "已关闭解析器调试信息" << std::endl;
                    return 0;
                }
            }
            else if (args[1] == "executor" && args.size() >= 3)
            {
                // 控制执行器调试模式
                if (args[2] == "on")
                {
                    executor_debug_enabled_ = true;
                    debug_enabled_ = true;  // 至少有一个模式开启时，全局调试也开启
                    std::cout << "已开启执行器调试信息" << std::endl;
                    return 0;
                }
                else if (args[2] == "off")
                {
                    executor_debug_enabled_ = false;
                    // 检查是否所有模式都关闭了
                    if (!command_debug_enabled_ && !parser_debug_enabled_ && !completion_debug_enabled_)
                    {
                        debug_enabled_ = false;
                    }
                    std::cout << "已关闭执行器调试信息" << std::endl;
                    return 0;
                }
            }
            else if (args[1] == "completion" && args.size() >= 3)
            {
                // 控制补全调试模式
                if (args[2] == "on")
                {
                    completion_debug_enabled_ = true;
                    debug_enabled_ = true;  // 至少有一个模式开启时，全局调试也开启
                    std::cout << "已开启补全调试信息" << std::endl;
                    return 0;
                }
                else if (args[2] == "off")
                {
                    completion_debug_enabled_ = false;
                    // 检查是否所有模式都关闭了
                    if (!command_debug_enabled_ && !parser_debug_enabled_ && !executor_debug_enabled_)
                    {
                        debug_enabled_ = false;
                    }
                    std::cout << "已关闭补全调试信息" << std::endl;
                    return 0;
                }
            }
        }
        
        // 参数错误，显示帮助信息
        std::cerr << "参数错误" << std::endl;
        showHelp();
        return 1;
    }
    
    std::string DebugCommand::getName() const
    {
        return "debug";
    }
    
    std::string DebugCommand::getHelp() const
    {
        return "debug [选项] - 控制调试信息的显示";
    }
    
    bool DebugCommand::isDebugEnabled()
    {
        return debug_enabled_;
    }
    
    bool DebugCommand::isCommandDebugEnabled()
    {
        return debug_enabled_ && command_debug_enabled_;
    }
    
    bool DebugCommand::isParserDebugEnabled()
    {
        return debug_enabled_ && parser_debug_enabled_;
    }
    
    bool DebugCommand::isExecutorDebugEnabled()
    {
        return debug_enabled_ && executor_debug_enabled_;
    }
    
    bool DebugCommand::isCompletionDebugEnabled()
    {
        return debug_enabled_ && completion_debug_enabled_;
    }
} 