/**
 * @file lexer.cpp
 * @brief 词法分析器类实现
 */

#include <iostream>
#include <sstream>
#include <cctype>
#include "core/lexer.h"
#include "core/shell.h"
#include "utils/error.h"

namespace dash
{

    // Token 实现

    Token::Token(TokenType type, const std::string &value, int line_number, int column)
        : type_(type), value_(value), line_number_(line_number), column_(column)
    {
    }

    std::string Token::toString() const
    {
        std::string type_str;

        switch (type_)
        {
        case TokenType::WORD:
            type_str = "WORD";
            break;
        case TokenType::ASSIGNMENT:
            type_str = "ASSIGNMENT";
            break;
        case TokenType::OPERATOR:
            type_str = "OPERATOR";
            break;
        case TokenType::IO_NUMBER:
            type_str = "IO_NUMBER";
            break;
        case TokenType::NEWLINE:
            type_str = "NEWLINE";
            break;
        case TokenType::END_OF_INPUT:
            type_str = "END_OF_INPUT";
            break;
        }

        std::ostringstream oss;
        oss << "[" << type_str << " '" << value_ << "' at " << line_number_ << ":" << column_ << "]";
        return oss.str();
    }

    // Lexer 实现

    Lexer::Lexer(Shell *shell)
        : shell_(shell), position_(0), line_number_(1), column_(1), eof_seen_(false)
    {
    }

    void Lexer::setInput(const std::string &input)
    {
        input_ = input;
        position_ = 0;
        line_number_ = 1;
        column_ = 1;
        eof_seen_ = false;

        // 清空词法单元队列
        std::queue<std::unique_ptr<Token>> empty;
        token_queue_.swap(empty);
    }

    char Lexer::currentChar() const
    {
        if (position_ >= input_.size())
        {
            return '\0'; // 表示输入结束
        }
        return input_[position_];
    }

    void Lexer::advance()
    {
        if (position_ < input_.size())
        {
            if (input_[position_] == '\n')
            {
                line_number_++;
                column_ = 1;
            }
            else
            {
                column_++;
            }
            position_++;
        }
    }

    char Lexer::peekChar() const
    {
        if (position_ + 1 >= input_.size())
        {
            return '\0'; // 表示输入结束
        }
        return input_[position_ + 1];
    }

    void Lexer::skipWhitespace()
    {
        while (std::isspace(currentChar()) && currentChar() != '\n')
        {
            advance();
        }
    }

    bool Lexer::isWordChar(char c) const
    {
        // 单词字符包括字母、数字、下划线和一些特殊字符
        return std::isalnum(c) || c == '_' || c == '/' || c == '.' || c == '-' || c == '+' || c == '@' || c == '$' || c == '*' || c == '?' || c == '(' || c == ')' || c == '`';
    }

    bool Lexer::isOperatorChar(char c) const
    {
        // 操作符字符
        return c == '|' || c == '&' || c == ';' || c == '<' || c == '>' || c == '(' || c == ')' || c == '{' || c == '}';
    }

    std::unique_ptr<Token> Lexer::parseWord()
    {
        int start_column = column_;
        std::string value;
        bool is_assignment = false;
        bool in_quotes = false;
        char quote_char = '\0';
        bool in_command_subst = false;
        int paren_count = 0;

        while (true)
        {
            char c = currentChar();

            // 处理命令替换 $(command)
            if (c == '$' && peekChar() == '(')
            {
                value += c;
                advance();
                value += c;
                advance();
                in_command_subst = true;
                paren_count = 1;
                continue;
            }
            
            // 处理命令替换中的括号
            if (in_command_subst)
            {
                if (c == '(')
                {
                    paren_count++;
                }
                else if (c == ')')
                {
                    paren_count--;
                    if (paren_count == 0)
                    {
                        in_command_subst = false;
                    }
                }
                
                value += c;
                advance();
                
                if (paren_count == 0)
                {
                    continue;
                }
                else if (c == '\0')
                {
                    throw ShellException(ExceptionType::SYNTAX, "Unterminated command substitution");
                }
                else
                {
                    continue;
                }
            }
            
            // 处理反引号命令替换 `command`
            if (c == '`')
            {
                value += c;
                advance();
                
                // 查找匹配的反引号
                while (currentChar() != '\0' && currentChar() != '`')
                {
                    value += currentChar();
                    advance();
                }
                
                if (currentChar() == '`')
                {
                    value += currentChar();
                    advance();
                }
                else
                {
                    throw ShellException(ExceptionType::SYNTAX, "Unterminated command substitution");
                }
                
                continue;
            }

            // 处理引号
            if (c == '"' || c == '\'')
            {
                if (!in_quotes)
                {
                    in_quotes = true;
                    quote_char = c;
                    // 不将引号添加到值中
                    advance();
                }
                else if (c == quote_char)
                {
                    in_quotes = false;
                    quote_char = '\0';
                    // 不将引号添加到值中
                    advance();
                }
                else
                {
                    value += c;
                    advance();
                }
                continue;
            }

            // 在引号内，所有字符都是单词的一部分
            if (in_quotes)
            {
                if (c == '\0')
                {
                    throw ShellException(ExceptionType::SYNTAX, "Unterminated quote");
                }
                value += c;
                advance();
                continue;
            }

            // 处理转义字符
            if (c == '\\')
            {
                value += c;
                advance();
                if (currentChar() != '\0')
                {
                    value += currentChar();
                    advance();
                }
                continue;
            }

            // 检查是否是赋值表达式（name=value）
            if (c == '=' && !value.empty() && !is_assignment)
            {
                is_assignment = true;
                value += c;
                advance();
                continue;
            }

            // 如果不是单词字符，则结束单词
            if (!isWordChar(c) && c != '=')
            {
                break;
            }

            value += c;
            advance();
        }

        // 创建相应类型的词法单元
        if (is_assignment)
        {
            return std::make_unique<Token>(TokenType::ASSIGNMENT, value, line_number_, start_column);
        }
        else
        {
            // 检查是否是 IO 编号
            bool is_io_number = true;
            for (char c : value)
            {
                if (!std::isdigit(c))
                {
                    is_io_number = false;
                    break;
                }
            }

            if (is_io_number && !value.empty() && (currentChar() == '>' || currentChar() == '<'))
            {
                return std::make_unique<Token>(TokenType::IO_NUMBER, value, line_number_, start_column);
            }
            else
            {
                return std::make_unique<Token>(TokenType::WORD, value, line_number_, start_column);
            }
        }
    }

    std::unique_ptr<Token> Lexer::parseOperator()
    {
        int start_column = column_;
        std::string value;

        // 处理多字符操作符
        if (currentChar() == '&' && peekChar() == '&')
        {
            value = "&&";
            advance();
            advance();
        }
        else if (currentChar() == '|' && peekChar() == '|')
        {
            value = "||";
            advance();
            advance();
        }
        else if (currentChar() == '>' && peekChar() == '>')
        {
            value = ">>";
            advance();
            advance();
        }
        else if (currentChar() == '<' && peekChar() == '<')
        {
            value = "<<";
            advance();
            advance();
        }
        else if (currentChar() == '<' && peekChar() == '&')
        {
            value = "<&";
            advance();
            advance();
        }
        else if (currentChar() == '>' && peekChar() == '&')
        {
            value = ">&";
            advance();
            advance();
        }
        else
        {
            // 单字符操作符
            value = currentChar();
            advance();
        }

        return std::make_unique<Token>(TokenType::OPERATOR, value, line_number_, start_column);
    }

    void Lexer::parseComment()
    {
        // 跳过注释（从 # 到行尾）
        while (currentChar() != '\0' && currentChar() != '\n')
        {
            advance();
        }
    }

    std::unique_ptr<Token> Lexer::nextToken()
    {
        // 如果队列中有词法单元，则返回队列中的第一个
        if (!token_queue_.empty())
        {
            std::unique_ptr<Token> token = std::move(token_queue_.front());
            token_queue_.pop();
            return token;
        }

        // 如果已经看到 EOF，则返回 END_OF_INPUT 词法单元
        if (eof_seen_)
        {
            return std::make_unique<Token>(TokenType::END_OF_INPUT, "", line_number_, column_);
        }

        // 跳过空白字符
        skipWhitespace();

        char c = currentChar();

        // 检查输入结束
        if (c == '\0')
        {
            eof_seen_ = true;
            return std::make_unique<Token>(TokenType::END_OF_INPUT, "", line_number_, column_);
        }

        // 处理换行符
        if (c == '\n')
        {
            int start_column = column_;
            advance();
            return std::make_unique<Token>(TokenType::NEWLINE, "\n", line_number_ - 1, start_column);
        }

        // 处理注释
        if (c == '#')
        {
            parseComment();
            return nextToken(); // 递归调用以获取下一个有效词法单元
        }

        // 处理操作符
        if (isOperatorChar(c))
        {
            return parseOperator();
        }

        // 处理单词（包括命令、参数、变量赋值等）
        return parseWord();
    }

    const Token *Lexer::peekToken()
    {
        if (token_queue_.empty())
        {
            token_queue_.push(nextToken());
        }

        return token_queue_.front().get();
    }

    void Lexer::ungetToken(std::unique_ptr<Token> token)
    {
        std::queue<std::unique_ptr<Token>> new_queue;
        new_queue.push(std::move(token));

        while (!token_queue_.empty())
        {
            new_queue.push(std::move(token_queue_.front()));
            token_queue_.pop();
        }

        token_queue_.swap(new_queue);
    }

} // namespace dash