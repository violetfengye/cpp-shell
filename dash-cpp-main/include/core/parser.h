/**
 * @file parser.h
 * @brief 解析器类定义
 */

#ifndef DASH_PARSER_H
#define DASH_PARSER_H

#include <string>
#include <memory>
#include <vector>
#include <stack>
#include "core/lexer.h"

namespace dash
{

    // 前向声明
    class Shell;
    class Node;

    /**
     * @brief 解析器类
     *
     * 负责将词法单元流解析为抽象语法树。
     */
    class Parser
    {
    private:
        Shell *shell_;
        std::unique_ptr<Lexer> lexer_;
        std::stack<std::string> if_stack_;
        std::stack<std::string> case_stack_;
        std::stack<std::string> loop_stack_;

        /**
         * @brief 解析简单命令
         *
         * @return std::unique_ptr<Node> 命令节点
         */
        std::unique_ptr<Node> parseSimpleCommand();

        /**
         * @brief 解析管道
         *
         * @return std::unique_ptr<Node> 管道节点
         */
        std::unique_ptr<Node> parsePipeline();

        /**
         * @brief 解析命令列表
         *
         * @return std::unique_ptr<Node> 列表节点
         */
        std::unique_ptr<Node> parseList();

        /**
         * @brief 解析 if 语句
         *
         * @return std::unique_ptr<Node> If 节点
         */
        std::unique_ptr<Node> parseIf();

        /**
         * @brief 解析 for 循环
         *
         * @return std::unique_ptr<Node> For 节点
         */
        std::unique_ptr<Node> parseFor();

        /**
         * @brief 解析 while/until 循环
         *
         * @param until 是否是 until 循环
         * @return std::unique_ptr<Node> While 节点
         */
        std::unique_ptr<Node> parseWhile(bool until = false);

        /**
         * @brief 解析 case 语句
         *
         * @return std::unique_ptr<Node> Case 节点
         */
        std::unique_ptr<Node> parseCase();

        /**
         * @brief 解析子 shell
         *
         * @return std::unique_ptr<Node> Subshell 节点
         */
        std::unique_ptr<Node> parseSubshell();

        /**
         * @brief 解析重定向
         *
         * @param node 要添加重定向的节点
         * @return bool 是否成功解析重定向
         */
        bool parseRedirection(Node *node);

        /**
         * @brief 解析一个单词
         *
         * @return std::string 解析的单词
         */
        std::string parseWord();

        /**
         * @brief 期望下一个词法单元是指定类型
         *
         * @param type 期望的词法单元类型
         * @param error_message 错误消息
         * @return std::unique_ptr<Token> 词法单元
         */
        std::unique_ptr<Token> expectToken(TokenType type, const std::string &error_message);

        /**
         * @brief 跳过换行符
         */
        void skipNewlines();

        /**
         * @brief 检查是否是保留字
         *
         * @param word 要检查的单词
         * @return bool 是否是保留字
         */
        bool isReservedWord(const std::string &word) const;

        /**
         * @brief 检查是否是重定向操作符
         *
         * @param token 要检查的词法单元
         * @return bool 是否是重定向操作符
         */
        bool isRedirectionOperator(const Token *token) const;

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit Parser(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~Parser();

        /**
         * @brief 解析命令
         *
         * @param interactive 是否是交互式模式
         * @return std::unique_ptr<Node> 命令树根节点
         */
        std::unique_ptr<Node> parseCommand(bool interactive = false);

        /**
         * @brief 设置输入
         *
         * @param input 输入字符串
         */
        void setInput(const std::string &input);

        /**
         * @brief 获取词法分析器
         *
         * @return Lexer* 词法分析器指针
         */
        Lexer *getLexer() const { return lexer_.get(); }
    };

} // namespace dash

#endif // DASH_PARSER_H