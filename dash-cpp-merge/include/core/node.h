/**
 * @file node.h
 * @brief 语法树节点类定义
 */

#ifndef DASH_NODE_H
#define DASH_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <utility>

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 节点类型枚举
     */
    enum class NodeType
    {
        COMMAND,    // 简单命令
        PIPE,       // 管道
        SEQUENCE,   // 命令序列
        BACKGROUND, // 后台命令
        IF,         // if 语句
        WHILE,      // while 循环
        FOR,        // for 循环
        CASE,       // case 语句
        FUNCTION,   // 函数定义
        SUBSHELL,   // 子 shell
        GROUP       // 命令组
    };

    /**
     * @brief 重定向类型
     */
    enum class RedirectionType
    {
        INPUT,      // <
        OUTPUT,     // >
        APPEND,     // >>
        DUPLICATE,  // <& 或 >&
        HERE_DOC    // <<
    };

    /**
     * @brief 重定向结构体
     */
    struct Redirection
    {
        RedirectionType type; // 重定向类型
        int fd;               // 文件描述符
        std::string filename; // 文件名或目标文件描述符
        std::string here_doc; // here document 内容，仅在 HERE_DOC 类型时有效

        Redirection(RedirectionType t, int f, const std::string &fn)
            : type(t), fd(f), filename(fn) {}

        Redirection(RedirectionType t, int f, const std::string &fn, const std::string &hd)
            : type(t), fd(f), filename(fn), here_doc(hd) {}
    };

    /**
     * @brief 节点基类
     */
    class Node
    {
    protected:
        NodeType type_;

    public:
        /**
         * @brief 构造函数
         *
         * @param type 节点类型
         */
        explicit Node(NodeType type) : type_(type) {}

        /**
         * @brief 虚析构函数
         */
        virtual ~Node() = default;

        /**
         * @brief 获取节点类型
         *
         * @return NodeType 节点类型
         */
        NodeType getType() const { return type_; }

        /**
         * @brief 获取命令名称（仅对命令节点有效）
         *
         * @return std::string 命令名称，非命令节点返回空字符串
         */
        virtual std::string getCommandName() const { return ""; }

        /**
         * @brief 获取重定向列表（仅对支持重定向的节点有效）
         *
         * @return std::vector<Redirection> 重定向列表，不支持重定向的节点返回空列表
         */
        virtual std::vector<Redirection> getRedirections() const { return {}; }

        /**
         * @brief 获取左子节点（仅对管道和序列节点有效）
         *
         * @return Node* 左子节点指针，不支持的节点返回nullptr
         */
        virtual Node* getLeftChild() const { return nullptr; }

        /**
         * @brief 获取右子节点（仅对管道和序列节点有效）
         *
         * @return Node* 右子节点指针，不支持的节点返回nullptr
         */
        virtual Node* getRightChild() const { return nullptr; }

        /**
         * @brief 获取子节点（仅对后台、子shell和命令组节点有效）
         *
         * @return Node* 子节点指针，不支持的节点返回nullptr
         */
        virtual Node* getChild() const { return nullptr; }

        /**
         * @brief 获取条件节点（仅对if和while节点有效）
         *
         * @return Node* 条件节点指针，不支持的节点返回nullptr
         */
        virtual Node* getCondition() const { return nullptr; }

        /**
         * @brief 获取then分支节点（仅对if节点有效）
         *
         * @return Node* then分支节点指针，不支持的节点返回nullptr
         */
        virtual Node* getThenBranch() const { return nullptr; }

        /**
         * @brief 获取else分支节点（仅对if节点有效）
         *
         * @return Node* else分支节点指针，不支持的节点返回nullptr
         */
        virtual Node* getElseBranch() const { return nullptr; }

        /**
         * @brief 获取循环体节点（仅对while和for节点有效）
         *
         * @return Node* 循环体节点指针，不支持的节点返回nullptr
         */
        virtual Node* getBody() const { return nullptr; }

        /**
         * @brief 获取变量名（仅对for节点有效）
         *
         * @return std::string 变量名，不支持的节点返回空字符串
         */
        virtual std::string getVarName() const { return ""; }

        /**
         * @brief 获取值列表（仅对for节点有效）
         *
         * @return std::vector<std::string> 值列表，不支持的节点返回空列表
         */
        virtual std::vector<std::string> getValues() const { return {}; }

        /**
         * @brief 获取匹配词（仅对case节点有效）
         *
         * @return std::string 匹配词，不支持的节点返回空字符串
         */
        virtual std::string getWord() const { return ""; }

        /**
         * @brief 获取case项列表（仅对case节点有效）
         *
         * @return std::vector<std::pair<std::vector<std::string>, Node*>> case项列表，不支持的节点返回空列表
         */
        virtual std::vector<std::pair<std::vector<std::string>, Node*>> getCases() const { return {}; }
    };

} // namespace dash

#endif // DASH_NODE_H 