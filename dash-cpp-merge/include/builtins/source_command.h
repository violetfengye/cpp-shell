/**
 * @file source_command.h
 * @brief SOURCE命令头文件
 */

#ifndef DASH_SOURCE_COMMAND_H
#define DASH_SOURCE_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief SOURCE命令类，用于执行脚本文件
     */
    class SourceCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit SourceCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~SourceCommand() override;

        /**
         * @brief 执行SOURCE命令
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

#endif // DASH_SOURCE_COMMAND_H 