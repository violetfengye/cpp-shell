/**
 * @file node.h
 * @brief 语法树节点类定义
 */

#ifndef DASH_NODE_H
#define DASH_NODE_H

#include <string>
#include <vector>
#include <memory>
#include "../dash.h"

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 重定向类型
     */
    enum class RedirType
    {
        REDIR_INPUT,      // <
        REDIR_OUTPUT,     // >
        REDIR_APPEND,     // >>
        REDIR_INPUT_DUP,  // <&
        REDIR_OUTPUT_DUP, // >&
        REDIR_HEREDOC     // <<
    };

    /**
     * @brief 重定向结构体
     */
    struct Redirection
    {
        RedirType type;       // 重定向类型
        int fd;               // 文件描述符
        std::string filename; // 文件名或目标文件描述符

        Redirection(RedirType t, int f, const std::string &fn)
            : type(t), fd(f), filename(fn) {}
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
        explicit Node(NodeType type);

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
         * @brief 打印节点（用于调试）
         *
         * @param indent 缩进级别
         */
        virtual void print(int indent = 0) const = 0;
    };

    /**
     * @brief 命令节点
     */
    class CommandNode : public Node
    {
    private:
        std::vector<std::string> args_;
        std::vector<std::string> assignments_;
        std::vector<Redirection> redirections_;
        bool background_; // 是否在后台运行

    public:
        /**
         * @brief 构造函数
         */
        CommandNode();
        
        /**
         * @brief 设置后台运行标志
         *
         * @param background 是否在后台运行
         */
        void setBackground(bool background) { background_ = background; }
        
        /**
         * @brief 是否在后台运行
         *
         * @return true 在后台运行
         * @return false 在前台运行
         */
        bool isBackground() const { return background_; }

        /**
         * @brief 添加参数
         *
         * @param arg 参数
         */
        void addArg(const std::string &arg);

        /**
         * @brief 添加变量赋值
         *
         * @param assignment 变量赋值
         */
        void addAssignment(const std::string &assignment);

        /**
         * @brief 添加重定向
         *
         * @param redir 重定向
         */
        void addRedirection(const Redirection &redir);

        /**
         * @brief 获取参数
         *
         * @return const std::vector<std::string>& 参数列表
         */
        const std::vector<std::string> &getArgs() const { return args_; }

        /**
         * @brief 获取变量赋值
         *
         * @return const std::vector<std::string>& 变量赋值列表
         */
        const std::vector<std::string> &getAssignments() const { return assignments_; }

        /**
         * @brief 获取重定向
         *
         * @return const std::vector<Redirection>& 重定向列表
         */
        const std::vector<Redirection> &getRedirections() const { return redirections_; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief 管道节点
     */
    class PipeNode : public Node
    {
    private:
        std::unique_ptr<Node> left_;
        std::unique_ptr<Node> right_;
        bool background_;

    public:
        /**
         * @brief 构造函数
         *
         * @param left 左子节点
         * @param right 右子节点
         * @param background 是否在后台运行
         */
        PipeNode(std::unique_ptr<Node> left, std::unique_ptr<Node> right, bool background = false);

        /**
         * @brief 获取左子节点
         *
         * @return Node* 左子节点指针
         */
        Node *getLeft() const { return left_.get(); }

        /**
         * @brief 获取右子节点
         *
         * @return Node* 右子节点指针
         */
        Node *getRight() const { return right_.get(); }

        /**
         * @brief 是否在后台运行
         *
         * @return true 在后台运行
         * @return false 在前台运行
         */
        bool isBackground() const { return background_; }
        
        /**
         * @brief 设置后台运行标志
         *
         * @param background 是否在后台运行
         */
        void setBackground(bool background) { background_ = background; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief 列表节点（命令序列）
     */
    class ListNode : public Node
    {
    private:
        std::vector<std::unique_ptr<Node>> commands_;
        std::vector<std::string> operators_;

    public:
        /**
         * @brief 构造函数
         */
        ListNode();

        /**
         * @brief 添加命令
         *
         * @param command 命令节点
         * @param op 操作符（如 ;, &&, ||）
         */
        void addCommand(std::unique_ptr<Node> command, const std::string &op = "");

        /**
         * @brief 获取命令列表
         *
         * @return const std::vector<std::unique_ptr<Node>>& 命令列表
         */
        const std::vector<std::unique_ptr<Node>> &getCommands() const { return commands_; }

        /**
         * @brief 获取操作符列表
         *
         * @return const std::vector<std::string>& 操作符列表
         */
        const std::vector<std::string> &getOperators() const { return operators_; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief If 节点
     */
    class IfNode : public Node
    {
    private:
        std::unique_ptr<Node> condition_;
        std::unique_ptr<Node> then_part_;
        std::unique_ptr<Node> else_part_;

    public:
        /**
         * @brief 构造函数
         *
         * @param condition 条件
         * @param then_part Then 部分
         * @param else_part Else 部分
         */
        IfNode(std::unique_ptr<Node> condition, std::unique_ptr<Node> then_part, std::unique_ptr<Node> else_part = nullptr);

        /**
         * @brief 获取条件
         *
         * @return Node* 条件节点指针
         */
        Node *getCondition() const { return condition_.get(); }

        /**
         * @brief 获取 Then 部分
         *
         * @return Node* Then 部分节点指针
         */
        Node *getThenPart() const { return then_part_.get(); }

        /**
         * @brief 获取 Else 部分
         *
         * @return Node* Else 部分节点指针
         */
        Node *getElsePart() const { return else_part_.get(); }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief For 节点
     */
    class ForNode : public Node
    {
    private:
        std::string var_;
        std::vector<std::string> words_;
        std::unique_ptr<Node> body_;

    public:
        /**
         * @brief 构造函数
         *
         * @param var 循环变量
         * @param words 单词列表
         * @param body 循环体
         */
        ForNode(const std::string &var, const std::vector<std::string> &words, std::unique_ptr<Node> body);

        /**
         * @brief 获取循环变量
         *
         * @return const std::string& 循环变量
         */
        const std::string &getVar() const { return var_; }

        /**
         * @brief 获取单词列表
         *
         * @return const std::vector<std::string>& 单词列表
         */
        const std::vector<std::string> &getWords() const { return words_; }

        /**
         * @brief 获取循环体
         *
         * @return Node* 循环体节点指针
         */
        Node *getBody() const { return body_.get(); }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief While 节点
     */
    class WhileNode : public Node
    {
    private:
        std::unique_ptr<Node> condition_;
        std::unique_ptr<Node> body_;
        bool until_;

    public:
        /**
         * @brief 构造函数
         *
         * @param condition 条件
         * @param body 循环体
         * @param until 是否是 until 循环
         */
        WhileNode(std::unique_ptr<Node> condition, std::unique_ptr<Node> body, bool until = false);

        /**
         * @brief 获取条件
         *
         * @return Node* 条件节点指针
         */
        Node *getCondition() const { return condition_.get(); }

        /**
         * @brief 获取循环体
         *
         * @return Node* 循环体节点指针
         */
        Node *getBody() const { return body_.get(); }

        /**
         * @brief 是否是 until 循环
         *
         * @return true 是 until 循环
         * @return false 是 while 循环
         */
        bool isUntil() const { return until_; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief Case 节点
     */
    class CaseNode : public Node
    {
    public:
        /**
         * @brief Case 项
         */
        struct CaseItem
        {
            std::vector<std::string> patterns;
            std::unique_ptr<Node> commands;

            CaseItem(const std::vector<std::string> &p, std::unique_ptr<Node> c)
                : patterns(p), commands(std::move(c)) {}
        };

    private:
        std::string word_;
        std::vector<std::unique_ptr<CaseItem>> items_;

    public:
        /**
         * @brief 构造函数
         *
         * @param word 匹配词
         */
        explicit CaseNode(const std::string &word);

        /**
         * @brief 添加 Case 项
         *
         * @param patterns 模式列表
         * @param commands 命令
         */
        void addItem(const std::vector<std::string> &patterns, std::unique_ptr<Node> commands);

        /**
         * @brief 获取匹配词
         *
         * @return const std::string& 匹配词
         */
        const std::string &getWord() const { return word_; }

        /**
         * @brief 获取 Case 项列表
         *
         * @return const std::vector<std::unique_ptr<CaseItem>>& Case 项列表
         */
        const std::vector<std::unique_ptr<CaseItem>> &getItems() const { return items_; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

    /**
     * @brief 子 shell 节点
     */
    class SubshellNode : public Node
    {
    private:
        std::unique_ptr<Node> commands_;
        std::vector<Redirection> redirections_;

    public:
        /**
         * @brief 构造函数
         *
         * @param commands 命令
         */
        explicit SubshellNode(std::unique_ptr<Node> commands);

        /**
         * @brief 添加重定向
         *
         * @param redir 重定向
         */
        void addRedirection(const Redirection &redir);

        /**
         * @brief 获取命令
         *
         * @return Node* 命令节点指针
         */
        Node *getCommands() const { return commands_.get(); }

        /**
         * @brief 获取重定向列表
         *
         * @return const std::vector<Redirection>& 重定向列表
         */
        const std::vector<Redirection> &getRedirections() const { return redirections_; }

        /**
         * @brief 打印节点
         *
         * @param indent 缩进级别
         */
        void print(int indent = 0) const override;
    };

} // namespace dash

#endif // DASH_NODE_H