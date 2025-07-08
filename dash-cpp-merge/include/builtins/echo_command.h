/**
 * @file echo_command.h
 * @brief ECHO命令头文件
 */

#ifndef DASH_ECHO_COMMAND_H
#define DASH_ECHO_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief ECHO命令类，用于输出字符串
     */
    class EchoCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit EchoCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~EchoCommand() override;

        /**
         * @brief 执行ECHO命令
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
         * @brief 解释转义序列
         * @param str 包含转义序列的字符串
         * @return 解释后的字符串
         */
        std::string interpretEscapes(const std::string &str);
    };

} // namespace dash

#endif // DASH_ECHO_COMMAND_H 