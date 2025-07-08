/**
 * @file history_command.h
 * @brief HISTORY命令头文件
 */

#ifndef DASH_HISTORY_COMMAND_H
#define DASH_HISTORY_COMMAND_H

#include "builtins/builtin_command.h"

namespace dash
{
    /**
     * @brief HISTORY命令类，用于显示和管理命令历史记录
     */
    class HistoryCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit HistoryCommand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~HistoryCommand() override;

        /**
         * @brief 执行HISTORY命令
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

#endif // DASH_HISTORY_COMMAND_H 