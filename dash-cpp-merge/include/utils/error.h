/**
 * @file error.h
 * @brief 异常处理类定义
 */

#ifndef DASH_ERROR_H
#define DASH_ERROR_H

#include <string>
#include <stdexcept>

namespace dash
{

    /**
     * @brief 异常类型枚举
     */
    enum class ExceptionType
    {
        EXIT,      // 正常退出
        INTERRUPT, // 中断信号
        ERROR,     // 一般错误
        SYNTAX,    // 语法错误
        MEMORY,    // 内存错误
        IO,        // 输入输出错误
        RUNTIME,   // 运行时错误
        SYSTEM,    // 系统错误
        INTERNAL   // 内部错误
    };

    /**
     * @brief Shell 异常类
     */
    class ShellException : public std::runtime_error
    {
    private:
        ExceptionType type_;

    public:
        /**
         * @brief 构造函数
         *
         * @param type 异常类型
         * @param message 异常消息
         */
        ShellException(ExceptionType type, const std::string &message)
            : std::runtime_error(message), type_(type)
        {
        }

        /**
         * @brief 获取异常类型
         *
         * @return ExceptionType 异常类型
         */
        ExceptionType getType() const { return type_; }

        /**
         * @brief 获取异常类型字符串
         *
         * @return std::string 异常类型字符串
         */
        std::string getTypeString() const
        {
            switch (type_)
            {
            case ExceptionType::SYNTAX:
                return "Syntax Error";
            case ExceptionType::RUNTIME:
                return "Runtime Error";
            case ExceptionType::MEMORY:
                return "Memory Error";
            case ExceptionType::IO:
                return "IO Error";
            case ExceptionType::SYSTEM:
                return "System Error";
            case ExceptionType::INTERNAL:
                return "Internal Error";
            case ExceptionType::EXIT:
                return "Exit Request";
            case ExceptionType::ERROR:
                return "Error";
            case ExceptionType::INTERRUPT:
                return "Interrupt";
            default:
                return "Unknown Error";
            }
        }
    };

    /**
     * @brief 抛出退出异常
     *
     * @param exit_code 退出代码
     */
    void exitShell(int exit_code = 0);

    /**
     * @brief 抛出错误异常
     *
     * @param message 错误消息
     * @param exit_code 退出代码
     */
    void errorShell(const std::string &message, int exit_code = 1);

    /**
     * @brief 打印警告消息
     *
     * @param message 警告消息
     */
    void warnShell(const std::string &message);

} // namespace dash

#endif // DASH_ERROR_H 