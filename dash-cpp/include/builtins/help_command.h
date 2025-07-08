/**
 * @file help_command.h
 * @brief 帮助命令类定义
 */

#ifndef DASH_HELP_COMMAND_H
#define DASH_HELP_COMMAND_H

#include "builtin_command.h"
#include <map>
#include <string>

namespace dash
{
    // 前向声明
    class Shell;
    
    /**
     * @brief 帮助命令类
     * 
     * 显示shell支持的命令列表或特定命令的详细帮助信息
     */
    class HelpCommand : public BuiltinCommand
    {
    private:
        // Shell实例指针
        Shell* shell_;
        
        // 命令帮助信息映射表
        std::map<std::string, std::string> command_help_;
        
        /**
         * @brief 初始化命令帮助信息
         */
        void initializeCommandHelp();
        
    public:
        /**
         * @brief 构造函数
         * 
         * @param shell Shell实例指针
         */
        HelpCommand(Shell* shell);
        
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
    };
}

#endif // DASH_HELP_COMMAND_H 