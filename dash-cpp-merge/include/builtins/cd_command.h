/**
 * @file cd_command.h
 * @brief CD命令头文件
 */

#ifndef DASH_CD_COMMAND_H
#define DASH_CD_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief CD命令类，用于改变当前工作目录
     */
    class CdCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit CdCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~CdCommand() override;

        /**
         * @brief 执行CD命令
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

#endif // DASH_CD_COMMAND_H 