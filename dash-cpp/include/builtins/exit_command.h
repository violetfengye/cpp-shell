/**
 * @file exit_command.h
 * @brief Exit命令类定义
 */

#ifndef DASH_EXIT_COMMAND_H
#define DASH_EXIT_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"

namespace dash
{

    /**
     * @brief Exit命令类
     *
     * 实现shell的exit内置命令，用于退出shell。
     */
    class ExitCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit ExitCommand(Shell *shell);

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
    };

} // namespace dash

#endif // DASH_EXIT_COMMAND_H