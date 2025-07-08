/**
 * @file arithmetic.h
 * @brief 算术表达式处理
 */

#ifndef DASH_ARITHMETIC_H
#define DASH_ARITHMETIC_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../dash.h"
#include "../core/lexer.h"

namespace dash {

/**
 * @brief 算术表达式计算类
 */
class Arithmetic {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     */
    explicit Arithmetic(Shell& shell);

    /**
     * @brief 析构函数
     */
    ~Arithmetic();

    /**
     * @brief 计算算术表达式的值
     * 
     * @param expression 算术表达式字符串
     * @return long 表达式计算结果
     * @throws ShellException 表达式语法错误
     */
    long evaluate(const std::string& expression);

    /**
     * @brief 检查表达式语法是否正确
     * 
     * @param expression 算术表达式字符串
     * @return bool 表达式是否有效
     */
    bool isValid(const std::string& expression);

private:
    Shell& shell_;  // Shell实例引用

    /**
     * @brief 词法分析，将表达式转换为词法单元序列
     * 
     * @param expression 算术表达式字符串
     * @return std::vector<Token> 词法单元序列
     */
    std::vector<Token> tokenize(const std::string& expression);

    /**
     * @brief 语法分析并计算表达式
     * 
     * @param tokens 词法单元序列
     * @return long 计算结果
     */
    long parse(const std::vector<Token>& tokens);

    // 其他辅助方法
};

} // namespace dash

#endif // DASH_ARITHMETIC_H 