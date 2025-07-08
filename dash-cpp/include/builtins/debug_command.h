/**
 * @file debug_command.h
 * @brief 调试命令类定义
 */

#ifndef DASH_DEBUG_COMMAND_H
#define DASH_DEBUG_COMMAND_H

#include "builtin_command.h"
#include <string>

namespace dash
{
    // 前向声明
    class Shell;
    
    /**
     * @brief 调试命令类
     * 
     * 用于控制shell的调试信息显示
     */
    class DebugCommand : public BuiltinCommand
    {
    private:
        // 调试模式
        static bool debug_enabled_;
        
        // 命令调试模式
        static bool command_debug_enabled_;
        
        // 解析器调试模式
        static bool parser_debug_enabled_;
        
        // 执行器调试模式
        static bool executor_debug_enabled_;
        
        // 补全调试模式
        static bool completion_debug_enabled_;
        
        /**
         * @brief 显示帮助信息
         */
        void showHelp() const;
        
        /**
         * @brief 显示当前调试状态
         */
        void showStatus() const;
        
    public:
        /**
         * @brief 构造函数
         * 
         * @param shell Shell实例指针
         */
        DebugCommand(Shell* shell);
        
        /**
         * @brief 执行命令
         * 
         * @param args 命令参数
         * @return int 命令执行结果
         */
        int execute(const std::vector<std::string>& args) override;
        
        /**
         * @brief 获取命令名称
         * 
         * @return std::string 命令名称
         */
        std::string getName() const override;
        
        /**
         * @brief 获取命令帮助信息
         * 
         * @return std::string 命令帮助信息
         */
        std::string getHelp() const override;
        
        /**
         * @brief 检查是否启用调试模式
         * 
         * @return true 已启用
         * @return false 未启用
         */
        static bool isDebugEnabled();
        
        /**
         * @brief 检查是否启用命令调试模式
         * 
         * @return true 已启用
         * @return false 未启用
         */
        static bool isCommandDebugEnabled();
        
        /**
         * @brief 检查是否启用解析器调试模式
         * 
         * @return true 已启用
         * @return false 未启用
         */
        static bool isParserDebugEnabled();
        
        /**
         * @brief 检查是否启用执行器调试模式
         * 
         * @return true 已启用
         * @return false 未启用
         */
        static bool isExecutorDebugEnabled();
        
        /**
         * @brief 检查是否启用补全调试模式
         * 
         * @return true 已启用
         * @return false 未启用
         */
        static bool isCompletionDebugEnabled();
    };
}

#endif // DASH_DEBUG_COMMAND_H 