/**
 * @file tsl_command.h
 * @brief Tsl命令类定义，用于进行事务处理
 */

#ifndef DASH_TSL_COMMAND_H
#define DASH_TSL_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"
#include "utils/transaction.h"

namespace dash
{

    /**
     * @brief Tsl命令类
     *
     * 用于进行事务处理
     */
    class TslCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit TslCommand(Shell *shell);

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

#endif // DASH_TSL_COMMAND_H