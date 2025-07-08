/**
 * @file help_command.h
 * @brief HELP命令头文件
 */

#ifndef DASH_HELP_COMMAND_H
#define DASH_HELP_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief HELP命令类，用于显示命令帮助信息
     */
    class HelpCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit HelpCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~HelpCommand() override;

        /**
         * @brief 执行HELP命令
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
         * @brief 打印所有命令的简要帮助
         */
        void printAllCommands() const;

        /**
         * @brief 打印指定命令的详细帮助
         * @param command_name 命令名称
         */
        void printCommandHelp(const std::string &command_name) const;
    };

} // namespace dash

#endif // DASH_HELP_COMMAND_H 