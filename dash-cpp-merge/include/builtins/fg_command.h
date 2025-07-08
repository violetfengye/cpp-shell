/**
 * @file fg_command.h
 * @brief FG命令头文件
 */

#ifndef DASH_FG_COMMAND_H
#define DASH_FG_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief FG命令类，用于将后台作业移至前台
     */
    class FgCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit FgCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~FgCommand() override;

        /**
         * @brief 执行FG命令
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

#endif // DASH_FG_COMMAND_H 