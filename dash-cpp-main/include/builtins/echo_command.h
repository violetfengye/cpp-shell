/**
 * @file echo_command.h
 * @brief Echo命令类定义
 */

#ifndef DASH_ECHO_COMMAND_H
#define DASH_ECHO_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"

namespace dash
{

    /**
     * @brief Echo命令类
     *
     * 实现shell的echo内置命令，用于输出文本。
     */
    class EchoCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit EchoCommand(Shell *shell);

        /**
         * @brief 执行命令
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
        std::string getName() const override;

        /**
         * @brief 获取命令帮助信息
         *
         * @return std::string 帮助信息
         */
        std::string getHelp() const override;

    private:
        /**
         * @brief 处理转义序列
         *
         * @param str 包含转义序列的字符串
         * @return std::string 处理后的字符串
         */
        std::string processEscapes(const std::string &str);
    };

} // namespace dash

#endif // DASH_ECHO_COMMAND_H