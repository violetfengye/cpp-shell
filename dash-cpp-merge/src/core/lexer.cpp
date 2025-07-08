/**
 * @file lexer.cpp
 * @brief 词法分析器实现
 */

#include "core/lexer.h"
#include "core/shell.h"
#include <iostream>
#include <cctype>
#include <cstring>

namespace dash
{

    Lexer::Lexer(Shell *shell)
        : shell_(shell), input_(), position_(0), line_number_(1), 
          current_char_('\0'), peeked_token_(TokenType::NONE)
    {
    }

    Lexer::~Lexer()
    {
    }

    bool Lexer::initialize()
    {
        return true;
    }

    void Lexer::setInput(const std::string &input)
    {
        input_ = input;
        position_ = 0;
        line_number_ = 1;
        current_char_ = position_ < input_.length() ? input_[position_] : '\0';
        peeked_token_ = TokenType::NONE;
    }

    Token Lexer::getNextToken()
    {
        // 如果有预读的Token，返回它
        if (peeked_token_ != TokenType::NONE) {
            Token token;
            token.type = peeked_token_;
            token.value = peeked_value_;
            token.line = line_number_;

            // 清除预读的Token
            peeked_token_ = TokenType::NONE;
            peeked_value_.clear();

            return token;
        }

        // 跳过空白字符
        skipWhitespace();

        // 如果到达输入末尾，返回EOF
        if (current_char_ == '\0') {
            return Token{TokenType::END, "", line_number_};
        }

        // 处理注释
        if (current_char_ == '#') {
            skipComment();
            return getNextToken();
        }

        // 处理换行符
        if (current_char_ == '\n') {
            advance();
            line_number_++;
            return Token{TokenType::NEWLINE, "\n", line_number_ - 1};
        }

        // 处理分号，命令分隔符
        if (current_char_ == ';') {
            advance();
            return Token{TokenType::SEMICOLON, ";", line_number_};
        }

        // 处理管道符号
        if (current_char_ == '|') {
            advance();
            if (current_char_ == '|') {
                advance();
                return Token{TokenType::OR, "||", line_number_};
            }
            return Token{TokenType::PIPE, "|", line_number_};
        }

        // 处理与运算符
        if (current_char_ == '&') {
            advance();
            if (current_char_ == '&') {
                advance();
                return Token{TokenType::AND, "&&", line_number_};
            }
            return Token{TokenType::BACKGROUND, "&", line_number_};
        }

        // 处理重定向
        if (current_char_ == '<') {
            advance();
            if (current_char_ == '<') {
                advance();
                if (current_char_ == '-') {
                    advance();
                    return Token{TokenType::HEREDOC_DASH, "<<-", line_number_};
                }
                return Token{TokenType::HEREDOC, "<<", line_number_};
            }
            if (current_char_ == '&') {
                advance();
                return Token{TokenType::LESSAMP, "<&", line_number_};
            }
            return Token{TokenType::LESS, "<", line_number_};
        }

        if (current_char_ == '>') {
            advance();
            if (current_char_ == '>') {
                advance();
                if (current_char_ == '&') {
                    advance();
                    return Token{TokenType::DGREATAMP, ">>&", line_number_};
                }
                return Token{TokenType::DGREAT, ">>", line_number_};
            }
            if (current_char_ == '&') {
                advance();
                return Token{TokenType::GREATAMP, ">&", line_number_};
            }
            if (current_char_ == '|') {
                advance();
                return Token{TokenType::CLOBBER, ">|", line_number_};
            }
            return Token{TokenType::GREAT, ">", line_number_};
        }

        // 处理括号
        if (current_char_ == '(') {
            advance();
            return Token{TokenType::LPAREN, "(", line_number_};
        }

        if (current_char_ == ')') {
            advance();
            return Token{TokenType::RPAREN, ")", line_number_};
        }

        if (current_char_ == '{') {
            advance();
            return Token{TokenType::LBRACE, "{", line_number_};
        }

        if (current_char_ == '}') {
            advance();
            return Token{TokenType::RBRACE, "}", line_number_};
        }

        // 处理单词（命令名、参数等）
        if (isWordStart(current_char_)) {
            return word();
        }

        // 处理双引号字符串
        if (current_char_ == '"') {
            return doubleQuotedString();
        }

        // 处理单引号字符串
        if (current_char_ == '\'') {
            return singleQuotedString();
        }

        // 处理反引号字符串
        if (current_char_ == '`') {
            return backQuotedString();
        }

        // 处理美元符号（变量、命令替换等）
        if (current_char_ == '$') {
            return dollar();
        }

        // 未知字符
        std::string value(1, current_char_);
        advance();
        return Token{TokenType::UNKNOWN, value, line_number_};
    }

    Token Lexer::peekNextToken()
    {
        // 如果已经有预读的Token，直接返回
        if (peeked_token_ != TokenType::NONE) {
            Token token;
            token.type = peeked_token_;
            token.value = peeked_value_;
            token.line = line_number_;
            return token;
        }

        // 保存当前状态
        size_t saved_position = position_;
        char saved_current_char = current_char_;
        int saved_line_number = line_number_;

        // 获取下一个Token
        Token token = getNextToken();

        // 保存预读的Token
        peeked_token_ = token.type;
        peeked_value_ = token.value;

        // 恢复状态
        position_ = saved_position;
        current_char_ = saved_current_char;
        line_number_ = saved_line_number;

        return token;
    }

    void Lexer::advance()
    {
        position_++;
        if (position_ < input_.length()) {
            current_char_ = input_[position_];
        } else {
            current_char_ = '\0';
        }
    }

    void Lexer::skipWhitespace()
    {
        while (current_char_ != '\0' && isspace(current_char_) && current_char_ != '\n') {
            advance();
        }
    }

    void Lexer::skipComment()
    {
        // 跳过从#开始直到行尾的所有字符
        while (current_char_ != '\0' && current_char_ != '\n') {
            advance();
        }
    }

    bool Lexer::isWordStart(char c)
    {
        return isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-' || c == '+' || c == '?' || c == '*' || c == '@' || c == '!';
    }

    bool Lexer::isWordPart(char c)
    {
        return isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-' || c == '+' || c == '?' || c == '*' || c == '@' || c == '!' || c == ':' || c == ',';
    }

    Token Lexer::word()
    {
        std::string value;
        
        while (current_char_ != '\0' && isWordPart(current_char_)) {
            value += current_char_;
            advance();
        }
        
        // 检查是否是关键字
        if (value == "if") {
            return Token{TokenType::IF, value, line_number_};
        } else if (value == "then") {
            return Token{TokenType::THEN, value, line_number_};
        } else if (value == "else") {
            return Token{TokenType::ELSE, value, line_number_};
        } else if (value == "elif") {
            return Token{TokenType::ELIF, value, line_number_};
        } else if (value == "fi") {
            return Token{TokenType::FI, value, line_number_};
        } else if (value == "for") {
            return Token{TokenType::FOR, value, line_number_};
        } else if (value == "while") {
            return Token{TokenType::WHILE, value, line_number_};
        } else if (value == "do") {
            return Token{TokenType::DO, value, line_number_};
        } else if (value == "done") {
            return Token{TokenType::DONE, value, line_number_};
        } else if (value == "case") {
            return Token{TokenType::CASE, value, line_number_};
        } else if (value == "esac") {
            return Token{TokenType::ESAC, value, line_number_};
        } else if (value == "in") {
            return Token{TokenType::IN, value, line_number_};
        } else if (value == "function") {
            return Token{TokenType::FUNCTION, value, line_number_};
        } else if (value == "time") {
            return Token{TokenType::TIME, value, line_number_};
        }
        
        // 不是关键字，视为普通单词
        return Token{TokenType::WORD, value, line_number_};
    }

    Token Lexer::doubleQuotedString()
    {
        std::string value = "\"";
        advance();  // 跳过开头的双引号
        
        bool escaped = false;
        while (current_char_ != '\0') {
            if (current_char_ == '"' && !escaped) {
                value += current_char_;
                advance();
                break;
            }
            
            if (current_char_ == '\\' && !escaped) {
                escaped = true;
            } else {
                escaped = false;
            }
            
            value += current_char_;
            advance();
        }
        
        return Token{TokenType::DQUOTED, value, line_number_};
    }

    Token Lexer::singleQuotedString()
    {
        std::string value = "'";
        advance();  // 跳过开头的单引号
        
        while (current_char_ != '\0') {
            if (current_char_ == '\'') {
                value += current_char_;
                advance();
                break;
            }
            
            value += current_char_;
            advance();
        }
        
        return Token{TokenType::SQUOTED, value, line_number_};
    }

    Token Lexer::backQuotedString()
    {
        std::string value = "`";
        advance();  // 跳过开头的反引号
        
        bool escaped = false;
        while (current_char_ != '\0') {
            if (current_char_ == '`' && !escaped) {
                value += current_char_;
                advance();
                break;
            }
            
            if (current_char_ == '\\' && !escaped) {
                escaped = true;
            } else {
                escaped = false;
            }
            
            value += current_char_;
            advance();
        }
        
        return Token{TokenType::BQUOTED, value, line_number_};
    }

    Token Lexer::dollar()
    {
        std::string value = "$";
        advance();  // 跳过美元符号
        
        if (current_char_ == '{') {
            // ${...} 形式的变量替换
            value += current_char_;
            advance();
            
            while (current_char_ != '\0' && current_char_ != '}') {
                value += current_char_;
                advance();
            }
            
            if (current_char_ == '}') {
                value += current_char_;
                advance();
            }
            
            return Token{TokenType::PARAMETER, value, line_number_};
        } else if (current_char_ == '(') {
            // $(...) 形式的命令替换
            value += current_char_;
            advance();
            
            int nesting = 1;
            while (current_char_ != '\0' && nesting > 0) {
                if (current_char_ == '(') {
                    nesting++;
                } else if (current_char_ == ')') {
                    nesting--;
                }
                
                value += current_char_;
                advance();
                
                if (nesting == 0) {
                    break;
                }
            }
            
            return Token{TokenType::COMMAND, value, line_number_};
        } else if (isdigit(current_char_) || current_char_ == '*' || current_char_ == '@' || current_char_ == '#' ||
                  current_char_ == '?' || current_char_ == '-' || current_char_ == '$' || current_char_ == '!') {
            // $1, $*, $@, $#, $?, $-, $$, $! 等特殊参数
            value += current_char_;
            advance();
            return Token{TokenType::PARAMETER, value, line_number_};
        } else if (isalpha(current_char_) || current_char_ == '_') {
            // $NAME 形式的变量
            while (current_char_ != '\0' && (isalnum(current_char_) || current_char_ == '_')) {
                value += current_char_;
                advance();
            }
            return Token{TokenType::PARAMETER, value, line_number_};
        }
        
        // 单独的 $
        return Token{TokenType::PARAMETER, value, line_number_};
    }

} // namespace dash 