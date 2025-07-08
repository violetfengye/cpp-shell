/**
 * @file lexer.h
 * @brief 词法分析器类定义
 */

#ifndef DASH_LEXER_H
#define DASH_LEXER_H

#include <string>
#include <vector>
#include <memory>
#include <queue>

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 词法单元类型
     */
    enum class TokenType
    {
        WORD,        // 单词（命令、参数等）
        ASSIGNMENT,  // 变量赋值（name=value）
        OPERATOR,    // 操作符（|, &, ;, &&, ||, >, <, >> 等）
        IO_NUMBER,   // IO 编号（如 2> 中的 2）
        NEWLINE,     // 换行符
        END_OF_INPUT // 输入结束
    };

    /**
     * @brief 词法单元类
     */
    class Token
    {
    private:
        TokenType type_;
        std::string value_;
        int line_number_;
        int column_;

    public:
        /**
         * @brief 构造函数
         *
         * @param type 词法单元类型
         * @param value 词法单元值
         * @param line_number 行号
         * @param column 列号
         */
        Token(TokenType type, const std::string &value, int line_number, int column);

        /**
         * @brief 获取词法单元类型
         *
         * @return TokenType 词法单元类型
         */
        TokenType getType() const { return type_; }

        /**
         * @brief 获取词法单元值
         *
         * @return const std::string& 词法单元值
         */
        const std::string &getValue() const { return value_; }

        /**
         * @brief 获取行号
         *
         * @return int 行号
         */
        int getLineNumber() const { return line_number_; }

        /**
         * @brief 获取列号
         *
         * @return int 列号
         */
        int getColumn() const { return column_; }

        /**
         * @brief 将词法单元转换为字符串
         *
         * @return std::string 词法单元的字符串表示
         */
        std::string toString() const;
    };

    /**
     * @brief 词法分析器类
     */
    class Lexer
    {
    private:
        Shell *shell_;
        std::string input_;
        size_t position_;
        int line_number_;
        int column_;
        std::queue<std::unique_ptr<Token>> token_queue_;
        bool eof_seen_;

        /**
         * @brief 获取当前字符
         *
         * @return char 当前字符，如果到达输入末尾则返回 '\0'
         */
        char currentChar() const;

        /**
         * @brief 前进一个字符
         */
        void advance();

        /**
         * @brief 前瞻一个字符
         *
         * @return char 下一个字符，如果到达输入末尾则返回 '\0'
         */
        char peekChar() const;

        /**
         * @brief 跳过空白字符
         */
        void skipWhitespace();

        /**
         * @brief 解析单词
         *
         * @return std::unique_ptr<Token> 单词词法单元
         */
        std::unique_ptr<Token> parseWord();

        /**
         * @brief 解析操作符
         *
         * @return std::unique_ptr<Token> 操作符词法单元
         */
        std::unique_ptr<Token> parseOperator();

        /**
         * @brief 解析注释
         */
        void parseComment();

        /**
         * @brief 判断字符是否是单词字符
         *
         * @param c 要判断的字符
         * @return true 是单词字符
         * @return false 不是单词字符
         */
        bool isWordChar(char c) const;

        /**
         * @brief 判断字符是否是操作符字符
         *
         * @param c 要判断的字符
         * @return true 是操作符字符
         * @return false 不是操作符字符
         */
        bool isOperatorChar(char c) const;

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit Lexer(Shell *shell);

        /**
         * @brief 设置输入
         *
         * @param input 输入字符串
         */
        void setInput(const std::string &input);

        /**
         * @brief 获取下一个词法单元
         *
         * @return std::unique_ptr<Token> 下一个词法单元
         */
        std::unique_ptr<Token> nextToken();

        /**
         * @brief 前瞻下一个词法单元
         *
         * @return const Token* 下一个词法单元的指针，不转移所有权
         */
        const Token *peekToken();

        /**
         * @brief 将词法单元放回队列
         *
         * @param token 要放回的词法单元
         */
        void ungetToken(std::unique_ptr<Token> token);
    };

} // namespace dash

#endif // DASH_LEXER_H