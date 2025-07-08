/**
 * @file parser.h
 * @brief 语法解析器头文件
 */

#ifndef DASH_PARSER_H
#define DASH_PARSER_H

#include "core/lexer.h"

namespace dash
{
    // 前向声明
    class Shell;
    class Node;

    /**
     * @brief 语法解析器类
     */
    class Parser
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit Parser(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~Parser();

        /**
         * @brief 初始化解析器
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 解析输入
         * @return 语法树根节点
         */
        Node* parse();

    private:
        /**
         * @brief 消耗当前Token
         * @param token_type 期望的Token类型
         */
        void eat(TokenType token_type);

        /**
         * @brief 将Token类型转换为字符串
         * @param type Token类型
         * @return 字符串表示
         */
        std::string tokenTypeToString(TokenType type);

        /**
         * @brief 解析命令列表
         * @return 命令列表节点
         */
        Node* parseCommandList();

        /**
         * @brief 解析命令
         * @return 命令节点
         */
        Node* parseCommand();

        /**
         * @brief 解析管道
         * @return 管道节点
         */
        Node* parsePipeline();

        /**
         * @brief 解析简单命令
         * @return 简单命令节点
         */
        Node* parseSimpleCommand();

        /**
         * @brief 判断是否是重定向Token
         * @param type Token类型
         * @return 是否是重定向Token
         */
        bool isRedirectionToken(TokenType type);

        /**
         * @brief 解析if语句
         * @return if语句节点
         */
        Node* parseIf();

        /**
         * @brief 解析while语句
         * @return while语句节点
         */
        Node* parseWhile();

        /**
         * @brief 解析for语句
         * @return for语句节点
         */
        Node* parseFor();

        /**
         * @brief 解析case语句
         * @return case语句节点
         */
        Node* parseCase();

        /**
         * @brief 解析子shell
         * @return 子shell节点
         */
        Node* parseSubshell();

        /**
         * @brief 解析命令组
         * @return 命令组节点
         */
        Node* parseGroup();

    private:
        Shell* shell_;              ///< Shell实例指针
        Lexer* lexer_;              ///< 词法分析器指针
        Token current_token_;       ///< 当前Token
    };

} // namespace dash

#endif // DASH_PARSER_H 