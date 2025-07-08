/**
 * @file cd_command.h
 * @brief CD命令类定义
 */

#ifndef DASH_CD_COMMAND_H
#define DASH_CD_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"

namespace dash
{

    /**
     * @brief CD命令类
     *
     * 实现shell的cd内置命令，用于改变当前工作目录。
     */
    class CdCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit CdCommand(Shell *shell);

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
         * @brief 获取目标目录
         *
         * @param args 命令参数
         * @return std::string 目标目录路径
         */
        std::string getTargetDirectory(const std::vector<std::string> &args);

        /**
         * @brief 更新PWD和OLDPWD环境变量
         *
         * @param new_dir 新目录
         */
        void updatePwdVariables(const std::string &new_dir);
    };

} // namespace dash

#endif // DASH_CD_COMMAND_H