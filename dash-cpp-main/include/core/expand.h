/**
 * @file expand.h
 * @brief 路径和变量扩展
 */

#ifndef DASH_EXPAND_H
#define DASH_EXPAND_H

#include <string>
#include <vector>
#include <memory>
#include "../dash.h"

namespace dash {

/**
 * @brief 扩展类型枚举
 */
enum class ExpandType {
    PATHNAME,    // 路径名扩展 (*)
    TILDE,       // 波浪线扩展 (~)
    VARIABLE,    // 变量扩展 ($VAR)
    COMMAND,     // 命令扩展 $(cmd) 或 `cmd`
    ARITHMETIC,  // 算术扩展 $((expr))
    QUOTE,       // 引号处理
    SPECIAL      // 特殊字符处理
};

/**
 * @brief 扩展结果结构
 */
struct ExpandResult {
    std::vector<std::string> words;  // 扩展后的单词列表
    bool success;                    // 扩展是否成功
    std::string error;               // 错误信息
};

/**
 * @brief 路径和变量扩展类
 */
class Expand {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     */
    explicit Expand(Shell& shell);

    /**
     * @brief 析构函数
     */
    ~Expand();

    /**
     * @brief 执行所有类型的扩展
     * 
     * @param word 要扩展的单词
     * @return ExpandResult 扩展结果
     */
    ExpandResult expandWord(const std::string& word);

    /**
     * @brief 执行路径扩展
     * 
     * @param pattern 路径模式
     * @return std::vector<std::string> 匹配的路径列表
     */
    std::vector<std::string> expandPathname(const std::string& pattern);

    /**
     * @brief 执行波浪线扩展
     * 
     * @param path 包含波浪线的路径
     * @return std::string 扩展后的路径
     */
    std::string expandTilde(const std::string& path);

    /**
     * @brief 执行变量扩展
     * 
     * @param str 包含变量的字符串
     * @return std::string 扩展后的字符串
     */
    std::string expandVariable(const std::string& str);

    /**
     * @brief 执行命令扩展
     * 
     * @param command 命令字符串
     * @return std::string 命令输出
     */
    std::string expandCommand(const std::string& command);

    /**
     * @brief 执行算术扩展
     * 
     * @param expression 算术表达式
     * @return std::string 计算结果字符串
     */
    std::string expandArithmetic(const std::string& expression);

    /**
     * @brief 处理引号
     * 
     * @param str 包含引号的字符串
     * @return std::string 处理后的字符串
     */
    std::string handleQuotes(const std::string& str);

private:
    Shell& shell_;  // Shell实例引用

    /**
     * @brief 匹配通配符
     * 
     * @param pattern 通配符模式
     * @param str 要匹配的字符串
     * @return bool 是否匹配
     */
    bool matchPattern(const std::string& pattern, const std::string& str);

    /**
     * @brief 分割单词
     * 
     * @param str 输入字符串
     * @return std::vector<std::string> 单词列表
     */
    std::vector<std::string> splitWords(const std::string& str);
};

} // namespace dash

#endif // DASH_EXPAND_H 