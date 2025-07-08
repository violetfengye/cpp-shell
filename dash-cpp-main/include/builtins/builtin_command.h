/**
 * @file builtin_command.h
 * @brief 内置命令基类定义
 */

#ifndef DASH_BUILTIN_COMMAND_H
#define DASH_BUILTIN_COMMAND_H

#include <string>
#include <vector>

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 内置命令基类
     *
     * 所有内置命令都应该继承自这个类，并实现 execute 方法。
     */
    class BuiltinCommand
    {
    protected:
        Shell *shell_;

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit BuiltinCommand(Shell *shell) : shell_(shell) {}

        /**
         * @brief 虚析构函数
         */
        virtual ~BuiltinCommand() = default;

        /**
         * @brief 执行命令
         *
         * @param args 命令参数
         * @return int 执行结果状态码
         */
        virtual int execute(const std::vector<std::string> &args) = 0;

        /**
         * @brief 获取命令名
         *
         * @return std::string 命令名
         */
        virtual std::string getName() const = 0;

        /**
         * @brief 获取命令帮助信息
         *
         * @return std::string 帮助信息
         */
        virtual std::string getHelp() const = 0;
    };

} // namespace dash

#endif // DASH_BUILTIN_COMMAND_H