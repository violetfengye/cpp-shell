/**
 * @file wait_command.h
 * @brief wait 内置命令定义
 */

#ifndef DASH_WAIT_COMMAND_H
#define DASH_WAIT_COMMAND_H

#include "builtin_command.h"
#include "job/job_control.h"

namespace dash
{

    /**
     * @brief wait 内置命令
     */
    class WaitCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit WaitCommand(Shell *shell) : BuiltinCommand(shell, "wait") {}

        /**
         * @brief 执行 wait 命令
         *
         * @param args 命令参数
         * @return int 执行结果状态码
         */
        int execute(const std::vector<std::string> &args) override;

        /**
         * @brief 获取命令帮助信息
         *
         * @return std::string 帮助信息
         */
        std::string getHelp() const override { return "wait [进程ID | %作业ID ...]"; }
    };

} // namespace dash

#endif // DASH_WAIT_COMMAND_H 