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
        int last_status_;

        /**
         * @brief 执行命令
         *
         * @param node 命令节点
         * @return int 执行结果状态码
         */
        int executeCommand(Node *node);

        /**
         * @brief 执行管道
         *
         * @param node 管道节点
         * @return int 执行结果状态码
         */
        int executePipe(Node *node);

        /**
         * @brief 执行序列
         *
         * @param node 序列节点
         * @return int 执行结果状态码
         */
        int executeSequence(Node *node);

        /**
         * @brief 执行后台命令
         *
         * @param node 后台节点
         * @return int 执行结果状态码
         */
        int executeBackground(Node *node);

        /**
         * @brief 执行 if 语句
         *
         * @param node if 节点
         * @return int 执行结果状态码
         */
        int executeIf(Node *node);

        /**
         * @brief 执行 for 循环
         *
         * @param node for 节点
         * @return int 执行结果状态码
         */
        int executeFor(Node *node);

        /**
         * @brief 执行 while/until 循环
         *
         * @param node while 节点
         * @return int 执行结果状态码
         */
        int executeWhile(Node *node);

        /**
         * @brief 执行 case 语句
         *
         * @param node case 节点
         * @return int 执行结果状态码
         */
        int executeCase(Node *node);

        /**
         * @brief 执行函数
         *
         * @param node 函数节点
         * @return int 执行结果状态码
         */
        int executeFunction(Node *node);

        /**
         * @brief 执行子 shell
         *
         * @param node 子 shell 节点
         * @return int 执行结果状态码
         */
        int executeSubshell(Node *node);

        /**
         * @brief 执行命令组
         *
         * @param node 命令组节点
         * @return int 执行结果状态码
         */
        int executeGroup(Node *node);

        /**
         * @brief 执行外部命令
         *
         * @param args 参数列表
         * @param redirections 重定向列表
         * @return int 执行结果状态码
         */
        int executeExternalCommand(const std::vector<std::string> &args, const std::vector<Redirection> &redirections);

        /**
         * @brief 设置重定向
         *
         * @param redirections 重定向列表
         * @param saved_fds 保存的文件描述符列表
         * @return bool 是否成功
         */
        bool setupRedirections(const std::vector<Redirection> &redirections, std::vector<int> &saved_fds);

        /**
         * @brief 恢复重定向
         *
         * @param redirections 重定向列表
         * @param saved_fds 保存的文件描述符列表
         */
        void restoreRedirections(const std::vector<Redirection> &redirections, std::vector<int> &saved_fds);

        /**
         * @brief 模式匹配
         *
         * @param str 要匹配的字符串
         * @param pattern 模式字符串
         * @return bool 是否匹配
         */
        bool patternMatch(const std::string &str, const std::string &pattern);

    public:
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
         * @brief 初始化执行器
         *
         * @return bool 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 执行节点
         *
         * @param node 节点
         * @return int 执行结果状态码
         */
        int execute(Node *node);

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