/**
 * @file input.h
 * @brief 输入处理器类定义
 */

#ifndef DASH_INPUT_H
#define DASH_INPUT_H

#include <string>
#include <memory>
#include <stack>
#include <fstream>
#include <vector>
#include <functional>  // 添加functional头文件以支持回调函数

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 输入源基类
     */
    class InputSource
    {
    public:
        /**
         * @brief 虚析构函数
         */
        virtual ~InputSource() = default;

        /**
         * @brief 读取一行输入
         *
         * @return std::string 读取的行，如果到达文件末尾则返回空字符串
         */
        virtual std::string readLine() = 0;

        /**
         * @brief 检查是否到达文件末尾
         *
         * @return true 到达文件末尾
         * @return false 未到达文件末尾
         */
        virtual bool isEOF() const = 0;

        /**
         * @brief 获取输入源名称
         *
         * @return std::string 输入源名称
         */
        virtual std::string getName() const = 0;
    };

    /**
     * @brief 文件输入源
     */
    class FileInputSource : public InputSource
    {
    private:
        std::ifstream file_;
        std::string filename_;

    public:
        /**
         * @brief 构造函数
         *
         * @param filename 文件名
         */
        explicit FileInputSource(const std::string &filename);

        /**
         * @brief 析构函数
         */
        ~FileInputSource() override;

        /**
         * @brief 读取一行输入
         *
         * @return std::string 读取的行
         */
        std::string readLine() override;

        /**
         * @brief 检查是否到达文件末尾
         *
         * @return true 到达文件末尾
         * @return false 未到达文件末尾
         */
        bool isEOF() const override;

        /**
         * @brief 获取输入源名称
         *
         * @return std::string 文件名
         */
        std::string getName() const override;
    };

    /**
     * @brief 字符串输入源
     */
    class StringInputSource : public InputSource
    {
    private:
        std::vector<std::string> lines_;
        size_t current_line_;
        std::string name_;

    public:
        /**
         * @brief 构造函数
         *
         * @param str 输入字符串
         * @param name 输入源名称
         */
        StringInputSource(const std::string &str, const std::string &name = "string");

        /**
         * @brief 读取一行输入
         *
         * @return std::string 读取的行
         */
        std::string readLine() override;

        /**
         * @brief 检查是否到达文件末尾
         *
         * @return true 到达文件末尾
         * @return false 未到达文件末尾
         */
        bool isEOF() const override;

        /**
         * @brief 获取输入源名称
         *
         * @return std::string 输入源名称
         */
        std::string getName() const override;
    };

    /**
     * @brief 标准输入源
     */
    class StdinInputSource : public InputSource
    {
    private:
        bool eof_;
        bool interactive_;
        std::string prompt_;
        bool use_readline_;  // 是否使用readline库

        // Tab自动补全相关函数类型定义
        using CompletionFunc = std::function<std::vector<std::string>(const std::string&, int, int)>;
        CompletionFunc completion_func_;  // Tab自动补全回调函数

        /**
         * @brief 初始化readline库
         */
        void initializeReadline();

        /**
         * @brief 清理readline库资源
         */
        void cleanupReadline();

        /**
         * @brief 读取带有readline功能的输入
         * 
         * @return std::string 读取的行
         */
        std::string readLineWithReadline();

    public:
        /**
         * @brief 构造函数
         *
         * @param interactive 是否交互模式
         * @param prompt 提示符
         * @param use_readline 是否使用readline库
         */
        explicit StdinInputSource(bool interactive = false, const std::string &prompt = "$ ", bool use_readline = true);

        /**
         * @brief 析构函数
         */
        ~StdinInputSource();

        /**
         * @brief 读取一行输入
         *
         * @return std::string 读取的行
         */
        std::string readLine() override;

        /**
         * @brief 检查是否到达文件末尾
         *
         * @return true 到达文件末尾
         * @return false 未到达文件末尾
         */
        bool isEOF() const override;

        /**
         * @brief 重置EOF标志
         */
        void resetEOF();

        /**
         * @brief 获取输入源名称
         *
         * @return std::string 输入源名称
         */
        std::string getName() const override;

        /**
         * @brief 设置提示符
         *
         * @param prompt 提示符
         */
        void setPrompt(const std::string &prompt);
        
        /**
         * @brief 设置Tab自动补全回调函数
         * 
         * @param func 补全函数
         */
        void setCompletionFunction(CompletionFunc func);

        /**
         * @brief 执行Tab自动补全
         * 
         * @param text 待补全的文本
         * @param start 待补全部分的起始位置
         * @param end 待补全部分的结束位置
         * @return std::vector<std::string> 补全结果列表
         */
        std::vector<std::string> complete(const std::string& text, int start, int end);
    };

    /**
     * @brief 输入处理器类
     *
     * 负责管理输入源栈，提供输入读取功能。
     */
    class InputHandler
    {
    public:
        /**
         * @brief 输入标志
         */
        enum InputFlags
        {
            IF_NONE = 0,
            IF_PUSH_FILE = 1, // 将当前输入源压入栈
            IF_NOFILE_OK = 2  // 如果文件不存在也不报错
        };

    private:
        Shell *shell_;
        std::stack<std::unique_ptr<InputSource>> input_stack_;

        /**
         * @brief Tab自动补全函数
         * 
         * @param text 待补全的文本
         * @param start 待补全部分的起始位置
         * @param end 待补全部分的结束位置
         * @return std::vector<std::string> 补全结果列表
         */
        std::vector<std::string> tabCompletion(const std::string& text, int start, int end);

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit InputHandler(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~InputHandler();

        /**
         * @brief 读取一行输入
         *
         * @param show_prompt 是否显示提示符
         * @return std::string 读取的行，如果到达文件末尾则返回空字符串
         */
        std::string readLine(bool show_prompt);

        /**
         * @brief 检查是否到达文件末尾
         *
         * @return true 到达文件末尾
         * @return false 未到达文件末尾
         */
        bool isEOF() const;

        /**
         * @brief 重置EOF状态（用于处理Ctrl+D）
         */
        void resetEOF();

        /**
         * @brief 将文件作为输入源压入栈
         *
         * @param filename 文件名
         * @param flags 标志
         * @return bool 是否成功
         */
        bool pushFile(const std::string &filename, int flags);

        /**
         * @brief 将字符串作为输入源压入栈
         *
         * @param str 输入字符串
         * @param name 输入源名称
         * @return bool 是否成功
         */
        bool pushString(const std::string &str, const std::string &name = "string");

        /**
         * @brief 弹出当前输入源
         *
         * @return bool 是否成功
         */
        bool popFile();

        /**
         * @brief 获取当前输入源名称
         *
         * @return std::string 输入源名称
         */
        std::string getCurrentSourceName() const;

        /**
         * @brief 设置提示符
         *
         * @param prompt 提示符
         */
        void setPrompt(const std::string &prompt);

        /**
         * @brief Tab自动补全函数（公共版本，用于测试）
         * 
         * @param text 待补全的文本
         * @param start 待补全部分的起始位置
         * @param end 待补全部分的结束位置
         * @return std::vector<std::string> 补全结果列表
         */
        std::vector<std::string> testTabCompletion(const std::string& text, int start, int end) {
            return tabCompletion(text, start, end);
        }
    };

} // namespace dash

#endif // DASH_INPUT_H