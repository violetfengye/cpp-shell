/**
 * @file kill_command.h
 * @brief kill 内置命令定义
 */

#ifndef DASH_KILL_COMMAND_H
#define DASH_KILL_COMMAND_H

#include "builtin_command.h"
#include <string>
#include <vector>

namespace dash
{
    /**
     * @brief KILL命令类，用于向进程发送信号
     */
    class KillCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit KillCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~KillCommand() override;

        /**
         * @brief 执行KILL命令
         * @param args 命令参数
         * @return 执行结果，0表示成功，非0表示失败
         */
        int execute(const std::vector<std::string> &args) override;

        /**
         * @brief 获取命令帮助信息
         * @return 帮助信息
         */
        std::string getHelp() const override;
    };

} // namespace dash

#endif // DASH_KILL_COMMAND_H 