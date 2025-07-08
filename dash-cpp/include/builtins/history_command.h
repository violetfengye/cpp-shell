/**
 * @file history_command.h
 * @brief History命令类定义
 */

#ifndef DASH_HISTORY_COMMAND_H
#define DASH_HISTORY_COMMAND_H

#include <string>
#include <vector>
#include "builtins/builtin_command.h"

namespace dash
{

    /**
     * @brief History命令类
     *
     * 实现shell的history内置命令，用于显示和管理命令历史记录。
     */
    class HistoryCommand : public BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell对象指针
         */
        explicit HistoryCommand(Shell *shell);

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
         * @brief 显示帮助信息
         */
        void displayHelp();

        /**
         * @brief 显示历史记录
         * 
         * @param count 要显示的记录数量，0表示全部
         */
        void displayHistory(size_t count = 0);

        /**
         * @brief 清除历史记录
         */
        void clearHistory();

        /**
         * @brief 保存历史记录到文件
         * 
         * @param filename 文件名
         */
        void saveHistory(const std::string &filename);

        /**
         * @brief 从文件加载历史记录
         * 
         * @param filename 文件名
         */
        void loadHistory(const std::string &filename);
    };

} // namespace dash

#endif // DASH_HISTORY_COMMAND_H 