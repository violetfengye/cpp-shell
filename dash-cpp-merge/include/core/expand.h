/**
 * @file expand.h
 * @brief 变量扩展和命令替换头文件
 */

#ifndef DASH_EXPAND_H
#define DASH_EXPAND_H

#include <string>
#include <vector>

namespace dash
{
    // 前向声明
    class Shell;
    class Node;

    /**
     * @brief 变量扩展和命令替换类
     */
    class Expand
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit Expand(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~Expand();

        /**
         * @brief 初始化
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 展开命令
         * @param node 命令节点
         * @param expanded_args 展开后的参数
         * @return 是否展开成功
         */
        bool expandCommand(Node *node, std::vector<std::string> &expanded_args);

        /**
         * @brief 展开单词
         * @param word 单词
         * @param result 展开结果
         * @return 是否展开成功
         */
        bool expandWord(const std::string &word, std::vector<std::string> &result);

        /**
         * @brief 展开变量和命令替换
         * @param str 原始字符串
         * @return 展开后的字符串
         */
        std::string expandVariablesAndCommands(const std::string &str);

        /**
         * @brief 展开双引号字符串
         * @param str 原始字符串（不包括双引号）
         * @return 展开后的字符串
         */
        std::string expandDoubleQuoted(const std::string &str);

        /**
         * @brief 展开变量
         * @param name 变量名
         * @return 变量值
         */
        std::string expandVariable(const std::string &name);

        /**
         * @brief 执行命令并获取输出
         * @param command 命令
         * @return 命令输出
         */
        std::string executeCommand(const std::string &command);

    private:
        /**
         * @brief 检查字符串是否包含通配符
         * @param str 字符串
         * @return 是否包含通配符
         */
        bool containsWildcards(const std::string &str);

        /**
         * @brief 展开通配符
         * @param pattern 通配符模式
         * @param result 展开结果
         * @return 是否展开成功
         */
        bool expandGlob(const std::string &pattern, std::vector<std::string> &result);

        /**
         * @brief 查找匹配的闭合括号
         * @param str 字符串
         * @param start_pos 起始位置
         * @param open_bracket 开括号
         * @param close_bracket 闭括号
         * @return 闭括号位置，如果没有找到则返回string::npos
         */
        size_t findClosingBracket(const std::string &str, size_t start_pos, char open_bracket, char close_bracket);

    private:
        Shell *shell_;  ///< Shell实例指针
    };

} // namespace dash

#endif // DASH_EXPAND_H 