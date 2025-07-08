/**
 * @file builtin_command.h
 * @brief 内置命令基类头文件
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
     */
    class BuiltinCommand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         * @param name 命令名称
         */
        BuiltinCommand(Shell *shell, const std::string &name);

        /**
         * @brief 析构函数
         */
        virtual ~BuiltinCommand();

        /**
         * @brief 执行命令
         * @param args 命令参数
         * @return 执行结果，0表示成功，非0表示失败
         */
        virtual int execute(const std::vector<std::string> &args) = 0;

        /**
         * @brief 获取命令名称
         * @return 命令名称
         */
        std::string getName() const;

        /**
         * @brief 获取命令帮助信息
         * @return 帮助信息
         */
        virtual std::string getHelp() const = 0;

    protected:
        Shell *shell_;      ///< Shell实例指针
        std::string name_;  ///< 命令名称
    };

} // namespace dash

#endif // DASH_BUILTIN_COMMAND_H 