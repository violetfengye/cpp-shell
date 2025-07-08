/**
 * @file bg_command.h
 * @brief BG命令头文件
 */

#ifndef DASH_BG_COMMAND_H
#define DASH_BG_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief BG命令类，用于将已停止的作业在后台继续运行
     */
    class BgCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit BgCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~BgCommand() override;

        /**
         * @brief 执行BG命令
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

#endif // DASH_BG_COMMAND_H 