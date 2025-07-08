/**
 * @file input.cpp
 * @brief 输入处理器类实现
 */

// 检查readline是否正确启用
#ifdef READLINE_ENABLED
#define READLINE_IS_ENABLED "是"
#else
#define READLINE_IS_ENABLED "否"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>  // 用于 std::numeric_limits
#include <cstdlib> // 用于 getenv
#include <unistd.h> // 用于 getcwd
#include <dirent.h> // 用于目录操作
#include <sys/stat.h> // 用于文件状态检查
#include <chrono>  // 用于时间测量
#include "../../include/core/input.h"
#include "../../include/core/shell.h"
#include "../../include/utils/error.h"
#include "../../include/builtins/debug_command.h"
#include "debug.h"

// 如果启用了readline库
#ifdef READLINE_ENABLED
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace dash
{
    // Readline库全局回调函数和相关变量
#ifdef READLINE_ENABLED
    // 全局StdinInputSource指针，用于回调
    static StdinInputSource* g_stdin_source = nullptr;

    // 自定义Tab键处理函数
    int custom_complete(int count, int key)
    {
        dash::DebugLog::logCompletion("custom_complete called, count=" + std::to_string(count));
        
        if (!g_stdin_source) {
            dash::DebugLog::logCompletion("g_stdin_source is NULL, using default completion");
            return rl_complete(count, key);
        }
        
        // 获取当前行和光标位置
        std::string line_buffer = rl_line_buffer ? rl_line_buffer : "";
        int cursor_pos = rl_point;
        
        dash::DebugLog::logCompletion("Line buffer: '" + line_buffer + "', cursor_pos=" + std::to_string(cursor_pos));
        
        // 找到当前单词的起始位置
        int word_start = cursor_pos;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // 提取当前单词
        std::string current_word = "";
        if (word_start <= cursor_pos && static_cast<size_t>(cursor_pos) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, cursor_pos - word_start);
        }
        
        dash::DebugLog::logCompletion("Current word: '" + current_word + "', word_start=" + 
                           std::to_string(word_start) + ", cursor_pos=" + std::to_string(cursor_pos));
        
        // 分析命令行，确定当前是否在补全文件名
        bool is_file_completion = false;
        std::string cmd_prefix = "";
        
        // 查找命令的起始位置
        size_t cmd_start = line_buffer.find_first_not_of(" \t");
        if (cmd_start != std::string::npos && cmd_start < static_cast<size_t>(word_start)) {
            // 查找命令后的第一个空格
            size_t cmd_end = line_buffer.find_first_of(" \t", cmd_start);
            if (cmd_end != std::string::npos && cmd_end < static_cast<size_t>(word_start)) {
                // 当前位置在命令后面，可能是文件名补全
                is_file_completion = true;
                cmd_prefix = line_buffer.substr(cmd_start, cmd_end - cmd_start);
                dash::DebugLog::logCompletion("Command detected: '" + cmd_prefix + "', file completion mode");
            }
        }
        
        // 获取补全结果
        std::vector<std::string> matches;
        
        if (is_file_completion) {
            // 文件名补全模式
            matches = g_stdin_source->complete(current_word, word_start, cursor_pos);
            dash::DebugLog::logCompletion("File completion mode, got " + std::to_string(matches.size()) + " matches");
        } else {
            // 命令补全模式
            matches = g_stdin_source->complete(current_word, word_start, cursor_pos);
            dash::DebugLog::logCompletion("Command completion mode, got " + std::to_string(matches.size()) + " matches");
        }
        
        // 记录上次Tab按下的时间和内容
        static auto last_tab_time = std::chrono::steady_clock::now();
        static std::string last_line_buffer = "";
        static int last_cursor_pos = -1;
        
        // 获取当前时间
        auto current_time = std::chrono::steady_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_tab_time).count();
        
        // 检查是否是快速连按两次Tab（500毫秒内）
        bool is_double_tab = time_diff < 500 && 
                             line_buffer == last_line_buffer && 
                             cursor_pos == last_cursor_pos;
        
        // 更新上次Tab按下的状态
        last_tab_time = current_time;
        last_line_buffer = line_buffer;
        last_cursor_pos = cursor_pos;
        
        // 如果没有匹配项，尝试强制进行文件名补全
        if (matches.empty() && !current_word.empty()) {
            dash::DebugLog::logCompletion("No matches found, trying forced file completion");
            
            // 获取当前目录
            std::string dir_path = ".";
            std::string file_prefix = current_word;
            
            // 如果包含路径，分离目录和文件名前缀
            size_t slash_pos = current_word.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                dir_path = current_word.substr(0, slash_pos);
                file_prefix = current_word.substr(slash_pos + 1);
                
                if (dir_path.empty()) {
                    dir_path = "/";
                }
            }
            
            // 打开目录并搜索匹配项
            DIR* dir;
            if ((dir = opendir(dir_path.c_str())) != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    
                    // 跳过 . 和 .. (除非明确要求)
                    if ((name == "." || name == "..") && file_prefix != "." && file_prefix != "..") {
                        continue;
                    }
                    
                    // 检查是否匹配前缀
                    if (name.compare(0, file_prefix.length(), file_prefix) == 0) {
                        // 构造完整路径
                        std::string full_path = (dir_path == ".") ? name : (dir_path + "/" + name);
                        
                        // 检查是否是目录
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0) {
                            // 构造结果
                            std::string result;
                            if (slash_pos != std::string::npos) {
                                result = current_word.substr(0, slash_pos + 1) + name;
                            } else {
                                result = name;
                            }
                            
                            // 如果是目录，添加斜杠
                            if (S_ISDIR(st.st_mode)) {
                                result += "/";
                            }
                            
                            matches.push_back(result);
                        }
                    }
                }
                closedir(dir);
            }
            
            dash::DebugLog::logCompletion("Forced file completion found " + std::to_string(matches.size()) + " matches");
        }
        
        if (matches.empty()) {
            dash::DebugLog::logCompletion("No matches found after all attempts");
            rl_ding(); // 发出提示音
            return 0;
        }
        
        // 如果是双击Tab，显示所有匹配项
        if (is_double_tab) {
            dash::DebugLog::logCompletion("Double Tab detected, showing all matches");
            
            // 显示所有匹配项
            std::cout << std::endl;
            for (const auto& match : matches) {
                std::cout << match << "  ";
            }
            std::cout << std::endl;
            
            // 重新显示提示符和当前行
            rl_forced_update_display();
            return 0;
        }
        
        // 单击Tab的情况
        if (matches.size() == 1) {
            // 如果只有一个匹配项，直接替换当前单词
            std::string match = matches[0];
            
            // 构造新的行内容
            std::string new_line = line_buffer.substr(0, word_start) + match;
            if (static_cast<size_t>(cursor_pos) < line_buffer.length()) {
                new_line += line_buffer.substr(cursor_pos);
            }
            
            // 替换readline的行缓冲区
            rl_replace_line(new_line.c_str(), 0);
            
            // 将光标移动到合适的位置
            rl_point = word_start + match.length();
            
            // 重新显示行
            rl_redisplay();
        } else {
            // 如果有多个匹配项，尝试补全最长的公共前缀
            std::string common_prefix = matches[0];
            for (size_t i = 1; i < matches.size(); ++i) {
                size_t j = 0;
                while (j < common_prefix.length() && j < matches[i].length() && 
                       common_prefix[j] == matches[i][j]) {
                    j++;
                }
                common_prefix = common_prefix.substr(0, j);
            }
            
            // 如果公共前缀比当前单词长，则替换为公共前缀
            if (common_prefix.length() > current_word.length()) {
                std::string new_line = line_buffer.substr(0, word_start) + common_prefix;
                if (static_cast<size_t>(cursor_pos) < line_buffer.length()) {
                    new_line += line_buffer.substr(cursor_pos);
                }
                
                // 替换readline的行缓冲区
                rl_replace_line(new_line.c_str(), 0);
                
                // 将光标移动到合适的位置
                rl_point = word_start + common_prefix.length();
                
                // 重新显示行
                rl_redisplay();
            } else {
                // 如果无法进一步补全，发出提示音
                rl_ding();
            }
        }
        
        return 0;
    }

    // Readline的tab补全回调函数
    char** readline_completion(const char* text, int start, int end)
    {
        // 调试输出
        std::string debug_msg = "readline_completion called: text='";
        debug_msg += (text ? text : "NULL");
        debug_msg += "', start=" + std::to_string(start) + ", end=" + std::to_string(end);
        dash::DebugLog::logCompletion(debug_msg);
        
        // 告诉readline不要使用默认的文件名补全
        rl_attempted_completion_over = 1;
        
        // 设置替换模式
        rl_completion_suppress_append = 1;
        rl_completion_append_character = '\0';
        
        // 检查text是否为空指针
        if (!text) {
            dash::DebugLog::logCompletion("Warning: text is NULL, returning NULL");
            return nullptr;
        }
        
        // 获取当前行的内容，用于确定当前单词的起始位置
        std::string line_buffer;
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        }
        
        // 找到当前单词的起始位置
        int word_start = start;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // 提取当前单词
        std::string current_word = line_buffer.substr(word_start, end - word_start);
        dash::DebugLog::logCompletion("Current word: '" + current_word + "'");
        
        if (g_stdin_source)
        {
            // 获取补全结果
            std::vector<std::string> matches = g_stdin_source->complete(current_word, word_start, end);
            
            dash::DebugLog::logCompletion("Found " + std::to_string(matches.size()) + " matches");
            for (size_t i = 0; i < matches.size() && i < 5; ++i) {
                dash::DebugLog::logCompletion("Match " + std::to_string(i) + ": " + matches[i]);
            }
            
            if (!matches.empty())
            {
                // 将结果转换为readline期望的格式
                char** result = static_cast<char**>(malloc((matches.size() + 1) * sizeof(char*)));
                for (size_t i = 0; i < matches.size(); ++i)
                {
                    // 我们需要返回完整的匹配项，而不仅仅是匹配部分
                    // 这样readline就会用完整的匹配项替换当前单词
                    result[i] = strdup(matches[i].c_str());
                }
                result[matches.size()] = nullptr;
                
                // 关键部分：手动实现替换功能
                if (matches.size() == 1) {
                    // 如果只有一个匹配项，直接替换当前单词
                    std::string match = matches[0];
                    std::string new_line = line_buffer.substr(0, word_start) + match;
                    
                    // 如果当前单词后面还有内容，也要保留
                    if (static_cast<size_t>(end) < line_buffer.length()) {
                        new_line += line_buffer.substr(end);
                    }
                    
                    // 替换readline的行缓冲区
                    rl_replace_line(new_line.c_str(), 0);
                    
                    // 将光标移动到合适的位置
                    rl_point = word_start + match.length();
                    
                    // 重新显示行
                    rl_redisplay();
                    
                    // 返回空结果，因为我们已经手动处理了替换
                    free(result[0]);
                    result[0] = nullptr;
                }
                
                dash::DebugLog::logCompletion("Returning " + std::to_string(matches.size()) + " matches to readline");
                return result;
            }
        }
        else {
            dash::DebugLog::logCompletion("g_stdin_source is NULL");
        }
        
        // 如果没有匹配项，返回NULL让readline使用默认补全
        dash::DebugLog::logCompletion("No matches found, returning NULL");
        return nullptr;
    }
    
    // 单个匹配生成器，用于支持自定义补全
    char* readline_match_generator(const char* text, int state)
    {
        dash::DebugLog::logCompletion("readline_match_generator: text='" + std::string(text ? text : "NULL") + 
                          "', state=" + std::to_string(state));
        
        // 检查text是否为空指针
        if (!text) {
            dash::DebugLog::logCompletion("Warning: text is NULL, returning NULL");
            return nullptr;
        }
        
        // 获取当前行的内容，用于确定当前单词的起始位置
        std::string line_buffer;
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        } else {
            line_buffer = text;
        }
        
        // 找到当前单词的起始位置
        int word_start = rl_point;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // 提取当前单词
        std::string current_word = "";
        if (word_start <= rl_point && static_cast<size_t>(rl_point) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, rl_point - word_start);
        }
        
        dash::DebugLog::logCompletion("Current word in generator: '" + current_word + "'");
        
        static size_t list_index;
        static std::vector<std::string> matches;
        
        // 如果是状态0，重新获取匹配列表
        if (state == 0)
        {
            list_index = 0;
            // 首次调用时获取所有匹配项
            if (g_stdin_source) {
                matches = g_stdin_source->complete(current_word, word_start, rl_point);
                dash::DebugLog::logCompletion("Generator found " + std::to_string(matches.size()) + " matches");
            }
            else
            {
                dash::DebugLog::logCompletion("Generator: g_stdin_source is NULL");
                matches.clear();
            }
        }
        
        // 返回当前匹配项，并增加索引
        if (list_index < matches.size())
        {
            dash::DebugLog::logCompletion("Generator returning match: " + matches[list_index]);
            
            // 如果只有一个匹配项，直接替换当前单词
            if (matches.size() == 1 && state == 0) {
                std::string match = matches[0];
                std::string new_line = line_buffer.substr(0, word_start) + match;
                
                // 如果当前单词后面还有内容，也要保留
                if (static_cast<size_t>(rl_point) < line_buffer.length()) {
                    new_line += line_buffer.substr(rl_point);
                }
                
                // 替换readline的行缓冲区
                rl_replace_line(new_line.c_str(), 0);
                
                // 将光标移动到合适的位置
                rl_point = word_start + match.length();
                
                // 重新显示行
                rl_redisplay();
            }
            
            return strdup(matches[list_index++].c_str());
        }
        
        dash::DebugLog::logCompletion("Generator: no more matches");
        return nullptr;
    }
#endif

    // FileInputSource 实现

    FileInputSource::FileInputSource(const std::string &filename)
        : filename_(filename)
    {
        file_.open(filename);
        if (!file_.is_open())
        {
            throw ShellException(ExceptionType::IO, "Cannot open file: " + filename);
        }
    }

    FileInputSource::~FileInputSource()
    {
        if (file_.is_open())
        {
            file_.close();
        }
    }

    std::string FileInputSource::readLine()
    {
        std::string line;
        if (std::getline(file_, line))
        {
            return line;
        }
        return "";
    }

    bool FileInputSource::isEOF() const
    {
        return file_.eof() || !file_.good();
    }

    std::string FileInputSource::getName() const
    {
        return filename_;
    }

    // StringInputSource 实现

    StringInputSource::StringInputSource(const std::string &str, const std::string &name)
        : current_line_(0), name_(name)
    {
        std::istringstream iss(str);
        std::string line;
        while (std::getline(iss, line))
        {
            lines_.push_back(line);
        }
    }

    std::string StringInputSource::readLine()
    {
        if (current_line_ < lines_.size())
        {
            return lines_[current_line_++];
        }
        return "";
    }

    bool StringInputSource::isEOF() const
    {
        return current_line_ >= lines_.size();
    }

    std::string StringInputSource::getName() const
    {
        return name_;
    }

    // StdinInputSource 实现

    StdinInputSource::StdinInputSource(bool interactive, const std::string &prompt, bool use_readline)
        : eof_(false), interactive_(interactive), prompt_(prompt), use_readline_(use_readline), completion_func_(nullptr)
    {
        // 强制启用交互模式和readline
        interactive_ = true;
        use_readline_ = true;
        
        // 输出readline启用状态
        dash::DebugLog::logCommand("\nReadline库是否启用: " + std::string(READLINE_IS_ENABLED ? "是" : "否"));
        
#ifdef READLINE_ENABLED
        dash::DebugLog::logCommand("Readline库已编译进程序");
        dash::DebugLog::logCommand("初始化Readline库...");
        initializeReadline();
#else
        dash::DebugLog::logCommand("Readline库未编译进程序");
#endif
    }

    StdinInputSource::~StdinInputSource()
    {
#ifdef READLINE_ENABLED
        if (interactive_ && use_readline_)
        {
            cleanupReadline();
        }
#endif
    }

    void StdinInputSource::initializeReadline()
    {
#ifdef READLINE_ENABLED
        dash::DebugLog::logCompletion("Initializing readline...");
        
        // 设置全局指针，用于回调
        g_stdin_source = this;
        
        // 配置readline
        rl_readline_name = "dash";
        rl_attempted_completion_function = readline_completion;
        
        // 禁用默认的文件名补全
        rl_completion_entry_function = readline_match_generator;
        
        // 启用自动补全 - 使用我们的自定义补全函数
        rl_bind_key('\t', custom_complete);
        dash::DebugLog::logCompletion("Tab key bound to custom_complete");
        
        // 设置补全分隔符
        rl_completer_word_break_characters = const_cast<char*>(" \t\n\"\\'`@$><=;|&{(");
        
        // 设置补全替换整个单词，而不是追加
        rl_completion_suppress_append = 1;
        rl_completion_query_items = 20;  // 当匹配项超过这个数量时，询问是否显示所有匹配
        
        // 设置补全行为
        rl_variable_bind("completion-ignore-case", "on");  // 忽略大小写
        rl_variable_bind("show-all-if-ambiguous", "on");   // 如果有多个匹配项，立即显示所有
        rl_variable_bind("completion-map-case", "on");     // 允许大小写映射
        rl_variable_bind("completion-prefix-display-length", "0"); // 不显示前缀
        rl_variable_bind("completion-display-width", "0");  // 自动计算显示宽度
        
        // 关键设置：强制替换当前单词而不是追加
        rl_completer_quote_characters = const_cast<char*>("\"'");
        rl_filename_completion_desired = 1;
        rl_filename_quoting_desired = 1;
        rl_completion_append_character = '\0';  // 不自动添加空格
        
        // 读取历史记录
        std::string home_dir = getenv("HOME") ? getenv("HOME") : ".";
        std::string history_file = home_dir + "/.dash_history";
        read_history(history_file.c_str());
        
        // 设置历史记录大小
        stifle_history(1000);
        
        // 添加一个测试补全，确认补全功能正常工作
        dash::DebugLog::logCompletion("添加测试补全");
        // 直接输出一些测试补全信息
        dash::DebugLog::logCommand("\n可用命令: echo, exit, cd, pwd, jobs, fg, bg, help, debug");
        dash::DebugLog::logCommand("按Tab键可以补全命令");
        
        dash::DebugLog::logCompletion("Readline initialization complete");
#else
        dash::DebugLog::logCompletion("Readline not enabled at compile time");
#endif
    }

    void StdinInputSource::cleanupReadline()
    {
#ifdef READLINE_ENABLED
        // 保存历史记录
        std::string home_dir = getenv("HOME") ? getenv("HOME") : ".";
        std::string history_file = home_dir + "/.dash_history";
        write_history(history_file.c_str());
        
        // 清除全局指针
        if (g_stdin_source == this)
        {
            g_stdin_source = nullptr;
        }
#endif
    }

    std::string StdinInputSource::readLineWithReadline()
    {
#ifdef READLINE_ENABLED
        // 使用readline读取一行
        char* line = readline(prompt_.c_str());
        
        if (line)
        {
            std::string result(line);
            
            // 添加非空行到历史记录
            if (!result.empty())
            {
                add_history(line);
            }
            
            free(line);
            return result;
        }
        else
        {
            // readline返回NULL表示EOF
            eof_ = true;
            return "";
        }
#else
        // 如果未启用readline，则使用标准输入
        if (interactive_)
        {
            std::cout << prompt_;
            std::cout.flush();
        }
        
        std::string line;
        if (std::getline(std::cin, line))
        {
            return line;
        }
        else
        {
            eof_ = true;
            return "";
        }
#endif
    }

    std::string StdinInputSource::readLine()
    {
        if (eof_)
        {
            return "";
        }
        
        if (interactive_ && use_readline_)
        {
            return readLineWithReadline();
        }
        else
        {
            // 非交互模式或不使用readline
            if (interactive_)
            {
                std::cout << prompt_;
                std::cout.flush();
            }
            
            std::string line;
            if (std::getline(std::cin, line))
            {
                return line;
            }
            else
            {
                eof_ = true;
                return "";
            }
        }
    }

    bool StdinInputSource::isEOF() const
    {
        return eof_;
    }

    void StdinInputSource::resetEOF()
    {
        eof_ = false;
    }

    std::string StdinInputSource::getName() const
    {
        return "stdin";
    }

    void StdinInputSource::setPrompt(const std::string &prompt)
    {
        prompt_ = prompt;
    }

    void StdinInputSource::setCompletionFunction(CompletionFunc func)
    {
        completion_func_ = func;
    }

    std::vector<std::string> StdinInputSource::complete(const std::string& text, int start, int end)
    {
        dash::DebugLog::logCompletion("StdinInputSource::complete called: text='" + text + 
                           "', start=" + std::to_string(start) + ", end=" + std::to_string(end));
        
        // 获取当前行的内容，用于确定当前单词的起始位置
        std::string line_buffer;
#ifdef READLINE_ENABLED
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        } else {
            line_buffer = text;
        }
#else
        line_buffer = text;
#endif
        
        // 找到当前单词的起始位置
        int word_start = start;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // 提取当前单词
        std::string current_word = "";
        if (word_start <= end && static_cast<size_t>(end) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, end - word_start);
        }
        
        dash::DebugLog::logCompletion("Current word: '" + current_word + "'");
        
        // 分析命令行，确定当前是否在补全文件名
        bool is_file_completion = false;
        std::string cmd_prefix = "";
        
        // 查找命令的起始位置
        size_t cmd_start = line_buffer.find_first_not_of(" \t");
        if (cmd_start != std::string::npos && cmd_start < static_cast<size_t>(word_start)) {
            // 查找命令后的第一个空格
            size_t cmd_end = line_buffer.find_first_of(" \t", cmd_start);
            if (cmd_end != std::string::npos && cmd_end < static_cast<size_t>(word_start)) {
                // 当前位置在命令后面，可能是文件名补全
                is_file_completion = true;
                cmd_prefix = line_buffer.substr(cmd_start, cmd_end - cmd_start);
                dash::DebugLog::logCompletion("Command detected: '" + cmd_prefix + "', file completion mode");
            }
        }
        
        // 如果设置了自定义补全函数，则调用
        if (completion_func_)
        {
            dash::DebugLog::logCompletion("Using custom completion function");
            auto results = completion_func_(current_word, word_start, end);
            dash::DebugLog::logCompletion("Custom completion returned " + std::to_string(results.size()) + " matches");
            return results;
        }
        
        dash::DebugLog::logCompletion("Using default completion implementation");
        
        // 默认实现：命令和文件名补全
        std::vector<std::string> matches;
        
        // 如果是文件补全模式或者是命令行开始位置，尝试补全命令
        if (!is_file_completion && start == 0)
        {
            // 内置命令列表（示例）
            const std::vector<std::string> builtins = {
                "cd", "echo", "exit", "pwd", "jobs", "fg", "bg", "history", "help", "alias", "unalias", "export", "source"
            };
            
            // 匹配内置命令
            for (const auto& cmd : builtins)
            {
                if (cmd.compare(0, current_word.length(), current_word) == 0)
                {
                    matches.push_back(cmd);
                }
            }
            
            dash::DebugLog::logCompletion("Found " + std::to_string(matches.size()) + " matching builtins");
        }
        
        // 文件名补全
        if (is_file_completion || matches.empty())
        {
            dash::DebugLog::logCompletion("Performing file completion");
            
            DIR* dir;
            struct dirent* entry;
            std::string dir_path = ".";
            std::string file_prefix = current_word;
            
            // 如果包含路径，分离目录和文件名前缀
            size_t slash_pos = current_word.find_last_of("/\\");
            if (slash_pos != std::string::npos)
            {
                dir_path = current_word.substr(0, slash_pos);
                file_prefix = current_word.substr(slash_pos + 1);
                
                // 处理空目录情况
                if (dir_path.empty())
                {
                    dir_path = "/";
                }
                
                dash::DebugLog::logCompletion("Path completion: dir_path='" + dir_path + 
                                   "', file_prefix='" + file_prefix + "'");
            }
            
            // 打开目录
            if ((dir = opendir(dir_path.c_str())) != NULL)
            {
                dash::DebugLog::logCompletion("Successfully opened directory: " + dir_path);
                
                while ((entry = readdir(dir)) != NULL)
                {
                    std::string name = entry->d_name;
                    
                    // 跳过 . 和 .. 如果前缀为空
                    if (file_prefix.empty() && (name == "." || name == ".."))
                    {
                        continue;
                    }
                    
                    // 检查是否匹配前缀
                    if (name.compare(0, file_prefix.length(), file_prefix) == 0)
                    {
                        // 构造完整路径和结果
                        std::string full_path = (dir_path == ".") ? name : (dir_path + "/" + name);
                        std::string result = (slash_pos != std::string::npos) ? 
                            (current_word.substr(0, slash_pos + 1) + name) : name;
                        
                        // 检查是否是目录，如果是则在后面添加斜杠
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                        {
                            result += "/";
                        }
                        
                        matches.push_back(result);
                        dash::DebugLog::logCompletion("Added match: " + result);
                    }
                }
                closedir(dir);
                
                dash::DebugLog::logCompletion("File completion found " + std::to_string(matches.size()) + " matches");
            }
            else
            {
                dash::DebugLog::logCompletion("Failed to open directory: " + dir_path);
            }
        }
        
        dash::DebugLog::logCompletion("Returning " + std::to_string(matches.size()) + " total matches");
        return matches;
    }

    // InputHandler 实现

    InputHandler::InputHandler(Shell *shell)
        : shell_(shell)
    {
        // 初始化时将标准输入作为默认输入源
        bool interactive = shell_->isInteractive();
        
        // 强制启用交互模式和readline
        dash::DebugLog::logCommand("原始交互模式状态: " + std::string(interactive ? "启用" : "禁用"));
        interactive = true;
        dash::DebugLog::logCommand("强制启用交互模式");
        
        auto stdin_source = std::make_unique<StdinInputSource>(interactive);
        
        // 设置Tab自动补全回调函数
        stdin_source->setCompletionFunction([this](const std::string& text, int start, int end) {
            return this->tabCompletion(text, start, end);
        });
        
        input_stack_.push(std::move(stdin_source));
    }

    InputHandler::~InputHandler()
    {
        // 清空输入源栈
        while (!input_stack_.empty())
        {
            input_stack_.pop();
        }
    }

    std::string InputHandler::readLine(bool show_prompt)
    {
        if (input_stack_.empty())
        {
            return "";
        }

        // 如果当前输入源到达文件末尾，则弹出并尝试下一个
        while (!input_stack_.empty() && input_stack_.top()->isEOF())
        {
            input_stack_.pop();
        }

        if (input_stack_.empty())
        {
            return "";
        }

        // 如果是交互式标准输入并且需要显示提示符
        if (show_prompt && input_stack_.top()->getName() == "stdin")
        {
            auto *stdin_source = dynamic_cast<StdinInputSource *>(input_stack_.top().get());
            if (stdin_source)
            {
                // 可以在这里根据 shell 状态设置不同的提示符
                std::string prompt = "$ ";
                stdin_source->setPrompt(prompt);
            }
        }

        return input_stack_.top()->readLine();
    }

    bool InputHandler::pushFile(const std::string &filename, int flags)
    {
        try
        {
            // 如果设置了 IF_PUSH_FILE 标志，则保留当前输入源
            if (!(flags & IF_PUSH_FILE) && !input_stack_.empty())
            {
                input_stack_.pop();
            }

            // 尝试打开文件
            std::ifstream file(filename);
            if (!file.is_open())
            {
                if (flags & IF_NOFILE_OK)
                {
                    return false;
                }
                throw ShellException(ExceptionType::IO, "Cannot open file: " + filename);
            }
            file.close();

            input_stack_.push(std::make_unique<FileInputSource>(filename));
            return true;
        }
        catch (const ShellException &e)
        {
            if (!(flags & IF_NOFILE_OK))
            {
                throw;
            }
            return false;
        }
    }

    bool InputHandler::pushString(const std::string &str, const std::string &name)
    {
        input_stack_.push(std::make_unique<StringInputSource>(str, name));
        return true;
    }

    bool InputHandler::popFile()
    {
        if (input_stack_.empty())
        {
            return false;
        }

        input_stack_.pop();

        // 如果栈为空，则添加标准输入作为默认输入源
        if (input_stack_.empty())
        {
            bool interactive = shell_->isInteractive();
            auto stdin_source = std::make_unique<StdinInputSource>(interactive);
            
            // 设置Tab自动补全回调函数
            stdin_source->setCompletionFunction([this](const std::string& text, int start, int end) {
                return this->tabCompletion(text, start, end);
            });
            
            input_stack_.push(std::move(stdin_source));
        }

        return true;
    }

    bool InputHandler::isEOF() const
    {
        return input_stack_.empty() || input_stack_.top()->isEOF();
    }

    void InputHandler::resetEOF()
    {
        if (!input_stack_.empty() && input_stack_.top()->getName() == "stdin")
        {
            auto *stdin_source = dynamic_cast<StdinInputSource *>(input_stack_.top().get());
            if (stdin_source)
            {
                stdin_source->resetEOF();
            }
        }
    }

    std::string InputHandler::getCurrentSourceName() const
    {
        if (input_stack_.empty())
        {
            return "none";
        }

        return input_stack_.top()->getName();
    }

    void InputHandler::setPrompt(const std::string &prompt)
    {
        if (!input_stack_.empty() && input_stack_.top()->getName() == "stdin")
        {
            auto *stdin_source = dynamic_cast<StdinInputSource *>(input_stack_.top().get());
            if (stdin_source)
            {
                stdin_source->setPrompt(prompt);
            }
        }
    }
    
    std::vector<std::string> InputHandler::tabCompletion(const std::string& text, int start, int end)
    {
        dash::DebugLog::logCompletion("InputHandler::tabCompletion called: text='" + text + 
                           "', start=" + std::to_string(start) + ", end=" + std::to_string(end));
        
        std::vector<std::string> matches;
        
        // 获取当前命令行
        std::string line_buffer;
        
#ifdef READLINE_ENABLED
        // 安全获取readline行缓冲区
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        } else {
            dash::DebugLog::log("Warning: rl_line_buffer is NULL, using text as line buffer");
            line_buffer = text;  // 在测试模式下使用text作为完整命令行
        }
#else
        // 没有readline的情况下，使用text作为行缓冲区
        line_buffer = text;
#endif

        // 找到当前单词的起始位置
        int word_start = start;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // 提取当前单词
        std::string current_word = "";
        if (word_start <= end && static_cast<size_t>(end) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, end - word_start);
        }
        
        dash::DebugLog::logCompletion("Current word: '" + current_word + "'");
        
        // 内置命令列表 - 可以从Shell实例获取完整的内置命令列表
        const std::vector<std::string> builtins = {
            "cd", "echo", "exit", "pwd", "jobs", "fg", "bg", "history", "help", "debug", "alias", "unalias", "export", "source"
        };
        
        // 检查是否是cd命令
        bool is_cd_command = false;
        if (line_buffer.length() >= 3 && line_buffer.substr(0, 3) == "cd " && start > 3) {
            is_cd_command = true;
            dash::DebugLog::logCompletion("检测到cd命令，只补全目录");
        }
        
        // 如果是命令行开始位置或者前面有管道/分号等，补全命令
        bool is_command_position = false;
        if (start == 0) {
            is_command_position = true;
        } else {
            // 确保start不超出line_buffer长度
            if (start >= 0 && static_cast<size_t>(start) < line_buffer.length()) {
                std::string before_text = line_buffer.substr(0, start);
                // 去除前导空格
                before_text.erase(0, before_text.find_first_not_of(" \t"));
                // 检查最后一个非空字符
                if (!before_text.empty()) {
                    char last_char = before_text[before_text.length() - 1];
                    if (last_char == '|' || last_char == ';' || last_char == '&' || 
                        (before_text.length() >= 2 && before_text.substr(before_text.length() - 2) == "&&") ||
                        (before_text.length() >= 2 && before_text.substr(before_text.length() - 2) == "||")) {
                        is_command_position = true;
                    }
                }
            }
        }
        
        // 如果是cd命令，只补全目录
        if (is_cd_command) {
            // 只补全目录
            std::string dir_prefix = current_word;
            std::string search_dir = ".";
            
            // 如果包含路径，分离目录和前缀
            size_t slash_pos = dir_prefix.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                search_dir = dir_prefix.substr(0, slash_pos);
                dir_prefix = dir_prefix.substr(slash_pos + 1);
                
                if (search_dir.empty()) {
                    search_dir = "/";
                }
            }
            
            DIR* dir;
            if ((dir = opendir(search_dir.c_str())) != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    
                    // 对于cd命令，包含 . 和 ..
                    
                    // 检查是否匹配前缀
                    if (name.compare(0, dir_prefix.length(), dir_prefix) == 0) {
                        // 构造完整路径
                        std::string full_path = (search_dir == ".") ? name : (search_dir + "/" + name);
                        
                        // 检查是否是目录
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                            // 构造结果
                            std::string result;
                            if (slash_pos != std::string::npos) {
                                result = current_word.substr(0, slash_pos + 1) + name;
                            } else {
                                result = name;
                            }
                            
                            matches.push_back(result + "/");
                        }
                    }
                }
                closedir(dir);
            }
            
            return matches;
        }
        // 如果是命令位置，补全命令
        else if (is_command_position) {
            // 匹配内置命令
            for (const auto& cmd : builtins) {
                if (cmd.compare(0, current_word.length(), current_word) == 0) {
                    matches.push_back(cmd);
                }
            }
            
            // 匹配PATH中的可执行文件
            const char* path_env = getenv("PATH");
            if (path_env) {
                std::string path = path_env;
                std::istringstream path_stream(path);
                std::string dir_path;
                
                while (std::getline(path_stream, dir_path, ':')) {
                    DIR* dir;
                    struct dirent* entry;
                    
                    if ((dir = opendir(dir_path.c_str())) != NULL) {
                        while ((entry = readdir(dir)) != NULL) {
                            std::string name = entry->d_name;
                            
                            // 检查是否匹配前缀
                            if (name.compare(0, current_word.length(), current_word) == 0) {
                                // 构造完整路径
                                std::string full_path = dir_path + "/" + name;
                                
                                // 检查是否是可执行文件
                                struct stat st;
                                if (stat(full_path.c_str(), &st) == 0 && 
                                    (st.st_mode & S_IXUSR) && !S_ISDIR(st.st_mode)) {
                                    matches.push_back(name);
                                }
                            }
                        }
                        closedir(dir);
                    }
                }
            }
            
            return matches;
        }
        // 否则，补全文件名
        else {
            // 文件名补全
            std::string file_prefix = current_word;
            std::string search_dir = ".";
            
            // 如果包含路径，分离目录和文件名前缀
            size_t slash_pos = file_prefix.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                search_dir = file_prefix.substr(0, slash_pos);
                file_prefix = file_prefix.substr(slash_pos + 1);
                
                if (search_dir.empty()) {
                    search_dir = "/";
                }
            }
            
            DIR* dir;
            if ((dir = opendir(search_dir.c_str())) != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    
                    // 跳过 . 和 .. (除非明确要求)
                    if ((name == "." || name == "..") && file_prefix != "." && file_prefix != "..") {
                        continue;
                    }
                    
                    // 检查是否匹配前缀
                    if (name.compare(0, file_prefix.length(), file_prefix) == 0) {
                        // 构造完整路径
                        std::string full_path = (search_dir == ".") ? name : (search_dir + "/" + name);
                        
                        // 检查是否是目录
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0) {
                            // 构造结果
                            std::string result;
                            if (slash_pos != std::string::npos) {
                                result = current_word.substr(0, slash_pos + 1) + name;
                            } else {
                                result = name;
                            }
                            
                            // 如果是目录，添加斜杠
                            if (S_ISDIR(st.st_mode)) {
                                result += "/";
                            }
                            
                            matches.push_back(result);
                        }
                    }
                }
                closedir(dir);
            }
            
            return matches;
        }
    }

} // namespace dash