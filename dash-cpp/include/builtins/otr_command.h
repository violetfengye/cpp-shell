/**
 * @file otr_command.h
 * @brief Otr命令类定义
 */

#ifndef DASH_OTR_COMMAND_H
#define DASH_OTR_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"
#include "utils/transaction.h"

namespace dash
{

    /**
     * @brief Otr命令类
     *
     * 实现otr命令，用于将任何指令视作外部指令。
     */
    class OtrCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit OtrCommand(Shell *shell);

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

#endif // DASH_OTR_COMMAND_H