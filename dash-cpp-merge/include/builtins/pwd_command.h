/**
 * @file pwd_command.h
 * @brief PWD命令头文件
 */

#ifndef DASH_PWD_COMMAND_H
#define DASH_PWD_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief PWD命令类，用于打印当前工作目录
     */
    class PwdCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit PwdCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~PwdCommand() override;

        /**
         * @brief 执行PWD命令
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

#endif // DASH_PWD_COMMAND_H 