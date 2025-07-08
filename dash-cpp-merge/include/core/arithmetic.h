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
#include "../../include/dash.h"

namespace dash {

class Shell;

// 定义算术表达式中的词法单元类型
enum class ArithTokenType {
    NUMBER,   // 数字
    PLUS,     // 加
    MINUS,    // 减
    MULTIPLY, // 乘
    DIVIDE,   // 除
    MODULO,   // 模
    LPAREN,   // 左括号
    RPAREN,   // 右括号
    VARIABLE, // 变量
    END       // 结束
};

// 算术表达式词法单元
struct ArithToken {
    ArithTokenType type;
    std::string value;
    
    ArithToken(ArithTokenType t, const std::string& v = "") : type(t), value(v) {}
};

/**
 * @brief 算术表达式计算类
 */
class Arithmetic {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例指针
     */
    explicit Arithmetic(Shell* shell);

    /**
     * @brief 析构函数
     */
    ~Arithmetic();
    
    /**
     * @brief 初始化算术表达式计算器
     * 
     * @return bool 是否初始化成功
     */
    bool initialize();

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
    Shell* shell_;  // Shell实例指针

    /**
     * @brief 词法分析，将表达式转换为词法单元序列
     * 
     * @param expression 算术表达式字符串
     * @return std::vector<ArithToken> 词法单元序列
     */
    std::vector<ArithToken> tokenize(const std::string& expression);

    /**
     * @brief 语法分析并计算表达式
     * 
     * @param tokens 词法单元序列
     * @param index 当前处理的词法单元索引
     * @return long 计算结果
     */
    long parseExpression(const std::vector<ArithToken>& tokens, size_t& index);

    /**
     * @brief 解析项（乘法、除法、模运算）
     * 
     * @param tokens 词法单元序列
     * @param index 当前处理的词法单元索引
     * @return long 计算结果
     */
    long parseTerm(const std::vector<ArithToken>& tokens, size_t& index);

    /**
     * @brief 解析因子（数字、变量、括号表达式）
     * 
     * @param tokens 词法单元序列
     * @param index 当前处理的词法单元索引
     * @return long 计算结果
     */
    long parseFactor(const std::vector<ArithToken>& tokens, size_t& index);

    /**
     * @brief 获取变量值
     * 
     * @param name 变量名
     * @return long 变量值
     */
    long getVariableValue(const std::string& name);
};

} // namespace dash

#endif // DASH_ARITHMETIC_H 