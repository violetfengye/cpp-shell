/**
 * @file output.h
 * @brief 输出处理
 */

#ifndef DASH_OUTPUT_H
#define DASH_OUTPUT_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include "../dash.h"

namespace dash {

/**
 * @brief 输出类型枚举
 */
enum class OutputType {
    NORMAL,     // 普通输出
    ERROR,      // 错误输出
    DEBUG,      // 调试输出
    PROMPT      // 提示符
};

/**
 * @brief 输出处理类
 */
class Output {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     */
    explicit Output(Shell& shell);

    /**
     * @brief 析构函数
     */
    ~Output();

    /**
     * @brief 输出普通消息
     * 
     * @param message 消息内容
     */
    void print(const std::string& message);

    /**
     * @brief 输出带换行的普通消息
     * 
     * @param message 消息内容
     */
    void println(const std::string& message);

    /**
     * @brief 输出错误消息
     * 
     * @param message 错误消息内容
     */
    void error(const std::string& message);

    /**
     * @brief 输出带换行的错误消息
     * 
     * @param message 错误消息内容
     */
    void errorln(const std::string& message);

    /**
     * @brief 输出调试消息
     * 
     * @param message 调试消息内容
     */
    void debug(const std::string& message);

    /**
     * @brief 输出带换行的调试消息
     * 
     * @param message 调试消息内容
     */
    void debugln(const std::string& message);

    /**
     * @brief 输出提示符
     * 
     * @param prompt 提示符内容
     */
    void prompt(const std::string& prompt);

    /**
     * @brief 设置是否启用彩色输出
     * 
     * @param enable 是否启用
     */
    void setColorEnabled(bool enable);

    /**
     * @brief 设置是否启用调试输出
     * 
     * @param enable 是否启用
     */
    void setDebugEnabled(bool enable);

    /**
     * @brief 获取是否启用彩色输出
     * 
     * @return bool 是否启用
     */
    bool isColorEnabled() const;

    /**
     * @brief 获取是否启用调试输出
     * 
     * @return bool 是否启用
     */
    bool isDebugEnabled() const;

    /**
     * @brief 设置输出流
     * 
     * @param outStream 标准输出流
     * @param errStream 错误输出流
     */
    void setStreams(std::ostream& outStream, std::ostream& errStream);

private:
    Shell& shell_;              // Shell实例引用
    bool colorEnabled_;         // 是否启用彩色输出
    bool debugEnabled_;         // 是否启用调试输出
    std::ostream* outStream_;   // 标准输出流
    std::ostream* errStream_;   // 错误输出流

    /**
     * @brief 输出指定类型的消息
     * 
     * @param message 消息内容
     * @param type 输出类型
     * @param newline 是否添加换行
     */
    void output(const std::string& message, OutputType type, bool newline);
};

} // namespace dash

#endif // DASH_OUTPUT_H 