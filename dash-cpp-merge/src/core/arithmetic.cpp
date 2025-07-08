/**
 * @file arithmetic.cpp
 * @brief 算术表达式处理实现
 */

#include "../../include/core/arithmetic.h"
#include "../../include/core/shell.h"
#include "../../include/variable/variable_manager.h"
#include "../../include/utils/error.h"
#include <stdexcept>
#include <cctype>
#include <stack>
#include <sstream>
#include <cmath>
#include <iostream>

namespace dash {

Arithmetic::Arithmetic(Shell* shell) : shell_(shell) {
    // 初始化
}

Arithmetic::~Arithmetic() {
    // 清理资源
}

bool Arithmetic::initialize() {
    // 初始化算术表达式计算器
    return true;
}

long Arithmetic::evaluate(const std::string& expression) {
    if (expression.empty()) {
        return 0;
    }

    try {
        auto tokens = tokenize(expression);
        size_t index = 0;
        long result = parseExpression(tokens, index);
        
        // 确保所有词法单元都已处理
        if (index < tokens.size() && tokens[index].type != ArithTokenType::END) {
            throw std::runtime_error("表达式语法错误：未预期的词法单元");
        }
        
        return result;
    } catch (const std::exception& e) {
        // 转换为Shell异常
        throw ShellException(ExceptionType::SYNTAX, "算术表达式错误: " + std::string(e.what()));
    }
}

bool Arithmetic::isValid(const std::string& expression) {
    try {
        auto tokens = tokenize(expression);
        size_t index = 0;
        parseExpression(tokens, index);
        
        // 确保所有词法单元都已处理
        return (index >= tokens.size() || tokens[index].type == ArithTokenType::END);
    } catch (...) {
        return false;
    }
}

std::vector<ArithToken> Arithmetic::tokenize(const std::string& expression) {
    std::vector<ArithToken> tokens;
    
    for (size_t i = 0; i < expression.length(); ++i) {
        char c = expression[i];
        
        // 跳过空白字符
        if (std::isspace(c)) {
            continue;
        }
        
        // 处理数字
        if (std::isdigit(c)) {
            std::string number;
            while (i < expression.length() && std::isdigit(expression[i])) {
                number += expression[i++];
            }
            --i; // 回退一位，因为循环会再自增
            tokens.push_back(ArithToken(ArithTokenType::NUMBER, number));
        }
        // 处理标识符/变量
        else if (std::isalpha(c) || c == '_') {
            std::string identifier;
            while (i < expression.length() && (std::isalnum(expression[i]) || expression[i] == '_')) {
                identifier += expression[i++];
            }
            --i;
            tokens.push_back(ArithToken(ArithTokenType::VARIABLE, identifier));
        }
        // 处理操作符
        else if (c == '+') {
            tokens.push_back(ArithToken(ArithTokenType::PLUS));
        }
        else if (c == '-') {
            tokens.push_back(ArithToken(ArithTokenType::MINUS));
        }
        else if (c == '*') {
            tokens.push_back(ArithToken(ArithTokenType::MULTIPLY));
        }
        else if (c == '/') {
            tokens.push_back(ArithToken(ArithTokenType::DIVIDE));
        }
        else if (c == '%') {
            tokens.push_back(ArithToken(ArithTokenType::MODULO));
        }
        else if (c == '(') {
            tokens.push_back(ArithToken(ArithTokenType::LPAREN));
        }
        else if (c == ')') {
            tokens.push_back(ArithToken(ArithTokenType::RPAREN));
        }
        else {
            // 未识别的字符
            throw std::runtime_error(std::string("未识别的字符: ") + c);
        }
    }
    
    tokens.push_back(ArithToken(ArithTokenType::END));
    return tokens;
}

long Arithmetic::parseExpression(const std::vector<ArithToken>& tokens, size_t& index) {
    long result = parseTerm(tokens, index);
    
    while (index < tokens.size()) {
        if (tokens[index].type == ArithTokenType::PLUS) {
            ++index;
            result += parseTerm(tokens, index);
        }
        else if (tokens[index].type == ArithTokenType::MINUS) {
            ++index;
            result -= parseTerm(tokens, index);
        }
        else {
            break;
        }
    }
    
    return result;
}

long Arithmetic::parseTerm(const std::vector<ArithToken>& tokens, size_t& index) {
    long result = parseFactor(tokens, index);
    
    while (index < tokens.size()) {
        if (tokens[index].type == ArithTokenType::MULTIPLY) {
            ++index;
            result *= parseFactor(tokens, index);
        }
        else if (tokens[index].type == ArithTokenType::DIVIDE) {
            ++index;
            long divisor = parseFactor(tokens, index);
            if (divisor == 0) {
                throw std::runtime_error("除数不能为0");
            }
            result /= divisor;
        }
        else if (tokens[index].type == ArithTokenType::MODULO) {
            ++index;
            long divisor = parseFactor(tokens, index);
            if (divisor == 0) {
                throw std::runtime_error("模数不能为0");
            }
            result %= divisor;
        }
        else {
            break;
        }
    }
    
    return result;
}

long Arithmetic::parseFactor(const std::vector<ArithToken>& tokens, size_t& index) {
    if (index >= tokens.size()) {
        throw std::runtime_error("表达式不完整");
    }
    
    const ArithToken& token = tokens[index];
    
    if (token.type == ArithTokenType::NUMBER) {
        ++index;
        return std::stol(token.value);
    }
    else if (token.type == ArithTokenType::VARIABLE) {
        ++index;
        return getVariableValue(token.value);
    }
    else if (token.type == ArithTokenType::LPAREN) {
        ++index;
        long result = parseExpression(tokens, index);
        
        if (index >= tokens.size() || tokens[index].type != ArithTokenType::RPAREN) {
            throw std::runtime_error("缺少右括号");
        }
        
        ++index; // 跳过右括号
        return result;
    }
    else if (token.type == ArithTokenType::MINUS) {
        ++index;
        return -parseFactor(tokens, index);
    }
    else if (token.type == ArithTokenType::PLUS) {
        ++index;
        return parseFactor(tokens, index);
    }
    
    throw std::runtime_error("表达式语法错误");
}

long Arithmetic::getVariableValue(const std::string& name) {
    if (!shell_ || !shell_->getVariableManager()) {
        return 0;
    }
    
    std::string value = shell_->getVariableManager()->getVariable(name);
    
    try {
        return std::stol(value);
    }
    catch (...) {
        // 非数字变量当作0处理
        return 0;
    }
}

} // namespace dash