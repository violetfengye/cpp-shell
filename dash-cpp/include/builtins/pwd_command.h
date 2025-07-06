/**
 * @file pwd_command.h
 * @brief Pwd命令类定义
 */

#ifndef DASH_PWD_COMMAND_H
#define DASH_PWD_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"

namespace dash
{

    /**
     * @brief Pwd命令类
     *
     * 实现shell的pwd内置命令，用于显示当前工作目录。
     */
    class PwdCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit PwdCommand(Shell *shell);

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
         * @brief 获取当前工作目录
         *
         * @param physical 是否使用物理路径（解析符号链接）
         * @return std::string 当前工作目录
         */
        std::string getCurrentDirectory(bool physical);
    };

} // namespace dash

#endif // DASH_PWD_COMMAND_H