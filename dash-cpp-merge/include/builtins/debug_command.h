/**
 * @file debug_command.h
 * @brief DEBUG命令头文件
 */

#ifndef DASH_DEBUG_COMMAND_H
#define DASH_DEBUG_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief DEBUG命令类，用于调试Shell
     */
    class DebugCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit DebugCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~DebugCommand() override;

        /**
         * @brief 执行DEBUG命令
         * @param args 命令参数
         * @return 执行结果，0表示成功，非0表示失败
         */
        int execute(const std::vector<std::string> &args) override;

        /**
         * @brief 获取命令帮助信息
         * @return 帮助信息
         */
        std::string getHelp() const override;

    private:
        /**
         * @brief 显示调试状态
         */
        void showDebugStatus() const;

        /**
         * @brief 显示所有变量
         */
        void showVariables() const;

        /**
         * @brief 显示所有作业
         */
        void showJobs() const;
    };

} // namespace dash

#endif // DASH_DEBUG_COMMAND_H 