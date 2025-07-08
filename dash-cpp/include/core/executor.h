/**
 * @file executor.h
 * @brief 执行器类定义
 */

#ifndef DASH_EXECUTOR_H
#define DASH_EXECUTOR_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "core/node.h"

namespace dash
{

    // 前向声明
    class Shell;
    class Node;
    class JobControl;
    class BuiltinCommand;

    /**
     * @brief 执行器类
     *
     * 负责执行语法树节点。
     */
    class Executor
    {
    private:
        Shell *shell_;
        std::unordered_map<std::string, std::function<int(const std::vector<std::string> &)>> builtins_;
        std::vector<std::shared_ptr<BuiltinCommand>> builtin_commands_; // 存储内置命令对象
        int last_status_;

        /**
         * @brief 执行重定向
         *
         * @param redirections 重定向列表
         * @param saved_fds 保存的文件描述符映射
         * @return bool 是否成功
         */
        bool applyRedirections(const std::vector<Redirection> &redirections, std::unordered_map<int, int> &saved_fds);

        /**
         * @brief 恢复重定向
         *
         * @param saved_fds 保存的文件描述符映射
         */
        void restoreRedirections(std::unordered_map<int, int> &saved_fds);

        /**
         * @brief 执行命令
         *
         * @param command 命令节点
         * @return int 执行结果状态码
         */
        int executeCommand(const CommandNode *command);

        /**
         * @brief 执行管道
         *
         * @param pipe 管道节点
         * @return int 执行结果状态码
         */
        int executePipe(const PipeNode *pipe);

        /**
         * @brief 执行列表
         *
         * @param list 列表节点
         * @return int 执行结果状态码
         */
        int executeList(const ListNode *list);

        /**
         * @brief 执行 if 语句
         *
         * @param if_node if 节点
         * @return int 执行结果状态码
         */
        int executeIf(const IfNode *if_node);

        /**
         * @brief 执行 for 循环
         *
         * @param for_node for 节点
         * @return int 执行结果状态码
         */
        int executeFor(const ForNode *for_node);

        /**
         * @brief 执行 while/until 循环
         *
         * @param while_node while 节点
         * @return int 执行结果状态码
         */
        int executeWhile(const WhileNode *while_node);

        /**
         * @brief 执行 case 语句
         *
         * @param case_node case 节点
         * @return int 执行结果状态码
         */
        int executeCase(const CaseNode *case_node);

        /**
         * @brief 执行子 shell
         *
         * @param subshell 子 shell 节点
         * @return int 执行结果状态码
         */
        int executeSubshell(const SubshellNode *subshell);

        /**
         * @brief 执行外部命令
         *
         * @param command 命令
         * @param args 参数列表
         * @param redirections 重定向列表
         * @param background 是否后台运行
         * @return int 执行结果状态码
         */
        int executeExternalCommand(const std::string &command, const std::vector<std::string> &args,
                                   const std::vector<Redirection> &redirections, bool background);

        /**
         * @brief 检查是否是内置命令
         *
         * @param command 命令名
         * @return bool 是否是内置命令
         */
        bool isBuiltin(const std::string &command) const;

        /**
         * @brief 执行内置命令
         *
         * @param command 命令名
         * @param args 参数列表
         * @return int 执行结果状态码
         */
        int executeBuiltin(const std::string &command, const std::vector<std::string> &args);

        /**
         * @brief 注册内置命令
         */
        void registerBuiltins();

    public:
        /**
         * @brief 在子进程中执行命令
         *
         * @param command 命令
         * @param args 参数列表
         */
        void exec_in_child(const std::string &command, const std::vector<std::string> &args);

        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit Executor(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~Executor();

        /**
         * @brief 执行节点
         *
         * @param node 节点
         * @return int 执行结果状态码
         */
        int execute(const Node *node);

        /**
         * @brief 获取上一次执行状态
         *
         * @return int 状态码
         */
        int getLastStatus() const { return last_status_; }

        /**
         * @brief 设置上一次执行状态
         *
         * @param status 状态码
         */
        void setLastStatus(int status) { last_status_ = status; }

        /**
         * @brief 获取 Shell 对象
         *
         * @return Shell* Shell 对象指针
         */
        Shell *getShell() const { return shell_; }
    };

} // namespace dash

#endif // DASH_EXECUTOR_H