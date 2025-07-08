/**
 * @file kill_command.h
 * @brief kill 内置命令定义
 */

#ifndef DASH_KILL_COMMAND_H
#define DASH_KILL_COMMAND_H

#include "builtin_command.h"
#include "job/job_control.h"

namespace dash
{

    /**
     * @brief kill 内置命令
     */
    class KillCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit KillCommand(Shell *shell) : BuiltinCommand(shell) {}

        /**
         * @brief 执行 kill 命令
         *
         * @param args 命令参数
         * @return int 执行结果状态码
         */
        int execute(const std::vector<std::string> &args) override;

        /**
         * @brief 获取命令名
         *
         * @return std::string 命令名
         */
        std::string getName() const override { return "kill"; }

        /**
         * @brief 获取命令帮助信息
         *
         * @return std::string 帮助信息
         */
        std::string getHelp() const override { return "kill [-s signal | -signal] pid | %job_id ..."; }
    };

} // namespace dash

#endif // DASH_KILL_COMMAND_H