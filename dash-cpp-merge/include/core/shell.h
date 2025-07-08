/**
 * @file shell.h
 * @brief Shell 类定义
 */

#ifndef DASH_SHELL_H
#define DASH_SHELL_H

#include <string>
#include <memory>
#include <vector>
#include <signal.h> // 用于sig_atomic_t类型

namespace dash
{

    // 前向声明
    class InputHandler;
    class Parser;
    class Executor;
    class VariableManager;
    class JobControl;
    class PipeNode;
    class History;
    class BGJobAdapter;
    class Expand;

    /**
     * @brief Shell 类
     *
     * 负责协调各个组件的工作，是整个 shell 的核心控制器。
     */
    class Shell
    {
    private:
        std::unique_ptr<InputHandler> input_;
        std::unique_ptr<VariableManager> variable_manager_;
        std::unique_ptr<Parser> parser_;
        std::unique_ptr<Executor> executor_;
        std::unique_ptr<JobControl> job_control_;
        std::unique_ptr<History> history_;
        std::unique_ptr<Expand> expand_;

        bool interactive_;
        bool exit_requested_;
        int exit_status_;

        std::string script_file_;
        std::vector<std::string> script_args_;
        std::string command_string_;
        
        /**
         * @brief 设置信号处理函数
         */
        void setupSignalHandlers();

        /**
         * @brief 解析命令行参数
         *
         * @param argc 参数数量
         * @param argv 参数数组
         * @return bool 是否成功
         */
        bool parseArgs(int argc, char *argv[]);

        /**
         * @brief 设置环境变量
         */
        void setupEnvironment();

        /**
         * @brief 运行交互式模式
         *
         * @return int 退出状态码
         */
        int runInteractive();

        /**
         * @brief 运行脚本模式
         *
         * @return int 退出状态码
         */
        int runScript();

        /**
         * @brief 显示提示符
         */
        void displayPrompt();

        /**
         * @brief 执行管道
         *
         * @param node 管道节点
         * @return int 执行结果状态码
         */
        int execute_pipeline(const PipeNode *node);

    public:
        // 信号处理相关
        static volatile sig_atomic_t received_sigchld;
        static volatile sig_atomic_t received_sigint;
        
        /**
         * @brief 构造函数
         */
        Shell();

        /**
         * @brief 析构函数
         */
        ~Shell();

        /**
         * @brief 运行 shell
         *
         * @param argc 参数数量
         * @param argv 参数数组
         * @return int 退出状态码
         */
        int run(int argc, char *argv[]);

        /**
         * @brief 退出 shell
         *
         * @param status 退出状态码
         */
        void exit(int status);

        /**
         * @brief 获取输入处理器
         *
         * @return InputHandler* 输入处理器指针
         */
        InputHandler *getInput() const;

        /**
         * @brief 获取变量管理器
         *
         * @return VariableManager* 变量管理器指针
         */
        VariableManager *getVariableManager() const;

        /**
         * @brief 获取解析器
         *
         * @return Parser* 解析器指针
         */
        Parser *getParser() const;

        /**
         * @brief 获取执行器
         *
         * @return Executor* 执行器指针
         */
        Executor *getExecutor() const;

        /**
         * @brief 获取作业控制
         *
         * @return JobControl* 作业控制指针
         */
        JobControl *getJobControl() const;

        /**
         * @brief 获取历史记录管理器
         *
         * @return History* 历史记录管理器指针
         */
        History *getHistory() const;
        
        /**
         * @brief 获取展开器
         *
         * @return Expand* 展开器指针
         */
        Expand *getExpand() const;

        /**
         * @brief 是否是交互式模式
         *
         * @return true 是交互式模式
         * @return false 不是交互式模式
         */
        bool isInteractive() const;

        /**
         * @brief 获取退出状态码
         *
         * @return int 退出状态码
         */
        int getExitStatus() const;
        
        /**
         * @brief 检查是否请求退出
         *
         * @return true 已请求退出
         * @return false 未请求退出
         */
        bool isExitRequested() const { return exit_requested_; }
        
        /**
         * @brief 检查是否为内置命令
         *
         * @param command 命令名
         * @return true 是内置命令
         * @return false 不是内置命令
         */
        bool isBuiltinCommand(const std::string &command) const;
        
        /**
         * @brief 执行内置命令
         *
         * @param command 命令名
         * @param argc 参数数量
         * @param argv 参数数组
         * @return int 执行结果状态码
         */
        int executeBuiltinCommand(const std::string &command, int argc, char *argv[]);
    };

} // namespace dash

#endif // DASH_SHELL_H 