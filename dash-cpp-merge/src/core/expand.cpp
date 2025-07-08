/**
 * @file expand.cpp
 * @brief 变量扩展和命令替换实现
 */

#include "core/expand.h"
#include "core/shell.h"
#include "core/node.h"
#include "variable/variable_manager.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glob.h>
#include <vector>
#include <string>
#include <algorithm>
#include <regex>

namespace dash
{

    Expand::Expand(Shell *shell)
        : shell_(shell)
    {
    }

    Expand::~Expand()
    {
    }

    bool Expand::initialize()
    {
        return true;
    }

    bool Expand::expandCommand(Node *node, std::vector<std::string> &expanded_args)
    {
        if (!node) {
            return false;
        }

        // 清空结果
        expanded_args.clear();

        // 获取命令参数
        std::vector<std::string> args = node->getArgs();
        if (args.empty()) {
            return true;
        }

        // 展开每个参数
        for (const auto &arg : args) {
            std::vector<std::string> expanded;
            if (!expandWord(arg, expanded)) {
                return false;
            }

            // 添加展开后的参数
            expanded_args.insert(expanded_args.end(), expanded.begin(), expanded.end());
        }

        return true;
    }

    bool Expand::expandWord(const std::string &word, std::vector<std::string> &result)
    {
        // 清空结果
        result.clear();

        // 如果是空字符串，直接返回
        if (word.empty()) {
            result.push_back("");
            return true;
        }

        // 检查是否是引号字符串
        if (word[0] == '"' && word[word.length() - 1] == '"') {
            // 双引号字符串，展开变量和命令替换，但不进行通配符展开
            std::string expanded = expandDoubleQuoted(word.substr(1, word.length() - 2));
            result.push_back(expanded);
            return true;
        } else if (word[0] == '\'' && word[word.length() - 1] == '\'') {
            // 单引号字符串，不进行任何展开
            result.push_back(word.substr(1, word.length() - 2));
            return true;
        } else if (word[0] == '`' && word[word.length() - 1] == '`') {
            // 反引号字符串，进行命令替换
            std::string command = word.substr(1, word.length() - 2);
            std::string output = executeCommand(command);
            
            // 按空白字符分割输出
            std::istringstream iss(output);
            std::string token;
            while (iss >> token) {
                result.push_back(token);
            }
            
            return true;
        }

        // 展开变量和命令替换
        std::string expanded = expandVariablesAndCommands(word);

        // 展开通配符
        if (containsWildcards(expanded)) {
            std::vector<std::string> glob_results;
            if (expandGlob(expanded, glob_results)) {
                if (!glob_results.empty()) {
                    result = glob_results;
                    return true;
                }
            }
        }

        // 如果没有匹配的通配符，或者不包含通配符，返回展开后的字符串
        result.push_back(expanded);
        return true;
    }

    std::string Expand::expandVariablesAndCommands(const std::string &str)
    {
        std::string result;
        size_t pos = 0;
        
        while (pos < str.length()) {
            if (str[pos] == '\\') {
                // 处理转义字符
                if (pos + 1 < str.length()) {
                    result += str[pos + 1];
                    pos += 2;
                } else {
                    result += '\\';
                    pos++;
                }
            } else if (str[pos] == '$') {
                // 处理变量或命令替换
                size_t var_start = pos;
                pos++;
                
                if (pos < str.length()) {
                    if (str[pos] == '{') {
                        // ${VAR} 形式的变量
                        size_t var_end = str.find('}', pos);
                        if (var_end != std::string::npos) {
                            std::string var_name = str.substr(pos + 1, var_end - pos - 1);
                            result += expandVariable(var_name);
                            pos = var_end + 1;
                        } else {
                            // 未闭合的 ${，保留原样
                            result += str.substr(var_start, 2);
                            pos = var_start + 2;
                        }
                    } else if (str[pos] == '(') {
                        // $(...) 形式的命令替换
                        size_t cmd_end = findClosingBracket(str, pos, '(', ')');
                        if (cmd_end != std::string::npos) {
                            std::string cmd = str.substr(pos + 1, cmd_end - pos - 1);
                            result += executeCommand(cmd);
                            pos = cmd_end + 1;
                        } else {
                            // 未闭合的 $(，保留原样
                            result += str.substr(var_start, 2);
                            pos = var_start + 2;
                        }
                    } else if (isalpha(str[pos]) || str[pos] == '_') {
                        // $VAR 形式的变量
                        size_t var_end = pos;
                        while (var_end < str.length() && (isalnum(str[var_end]) || str[var_end] == '_')) {
                            var_end++;
                        }
                        std::string var_name = str.substr(pos, var_end - pos);
                        result += expandVariable(var_name);
                        pos = var_end;
                    } else if (isdigit(str[pos]) || str[pos] == '*' || str[pos] == '@' || 
                              str[pos] == '#' || str[pos] == '?' || str[pos] == '-' || 
                              str[pos] == '$' || str[pos] == '!') {
                        // $1, $*, $@, $#, $?, $-, $$, $! 等特殊参数
                        std::string var_name(1, str[pos]);
                        result += expandVariable(var_name);
                        pos++;
                    } else {
                        // 单独的 $，保留原样
                        result += '$';
                    }
                } else {
                    // 字符串末尾的 $，保留原样
                    result += '$';
                }
            } else if (str[pos] == '`') {
                // 处理反引号命令替换
                size_t cmd_end = str.find('`', pos + 1);
                if (cmd_end != std::string::npos) {
                    std::string cmd = str.substr(pos + 1, cmd_end - pos - 1);
                    result += executeCommand(cmd);
                    pos = cmd_end + 1;
                } else {
                    // 未闭合的 `，保留原样
                    result += '`';
                    pos++;
                }
            } else {
                // 普通字符
                result += str[pos];
                pos++;
            }
        }
        
        return result;
    }

    std::string Expand::expandDoubleQuoted(const std::string &str)
    {
        std::string result;
        size_t pos = 0;
        
        while (pos < str.length()) {
            if (str[pos] == '\\') {
                // 处理转义字符
                if (pos + 1 < str.length()) {
                    char next = str[pos + 1];
                    if (next == '$' || next == '`' || next == '"' || next == '\\' || next == '\n') {
                        // 这些字符在双引号中可以被转义
                        if (next == '\n') {
                            // 转义的换行符被忽略
                        } else {
                            result += next;
                        }
                        pos += 2;
                    } else {
                        // 其他转义序列保留原样
                        result += '\\';
                        result += next;
                        pos += 2;
                    }
                } else {
                    // 字符串末尾的 \，保留原样
                    result += '\\';
                    pos++;
                }
            } else if (str[pos] == '$') {
                // 处理变量替换
                size_t var_start = pos;
                pos++;
                
                if (pos < str.length()) {
                    if (str[pos] == '{') {
                        // ${VAR} 形式的变量
                        size_t var_end = str.find('}', pos);
                        if (var_end != std::string::npos) {
                            std::string var_name = str.substr(pos + 1, var_end - pos - 1);
                            result += expandVariable(var_name);
                            pos = var_end + 1;
                        } else {
                            // 未闭合的 ${，保留原样
                            result += str.substr(var_start, 2);
                            pos = var_start + 2;
                        }
                    } else if (str[pos] == '(') {
                        // $(...) 形式的命令替换
                        size_t cmd_end = findClosingBracket(str, pos, '(', ')');
                        if (cmd_end != std::string::npos) {
                            std::string cmd = str.substr(pos + 1, cmd_end - pos - 1);
                            result += executeCommand(cmd);
                            pos = cmd_end + 1;
                        } else {
                            // 未闭合的 $(，保留原样
                            result += str.substr(var_start, 2);
                            pos = var_start + 2;
                        }
                    } else if (isalpha(str[pos]) || str[pos] == '_') {
                        // $VAR 形式的变量
                        size_t var_end = pos;
                        while (var_end < str.length() && (isalnum(str[var_end]) || str[var_end] == '_')) {
                            var_end++;
                        }
                        std::string var_name = str.substr(pos, var_end - pos);
                        result += expandVariable(var_name);
                        pos = var_end;
                    } else if (isdigit(str[pos]) || str[pos] == '*' || str[pos] == '@' || 
                              str[pos] == '#' || str[pos] == '?' || str[pos] == '-' || 
                              str[pos] == '$' || str[pos] == '!') {
                        // $1, $*, $@, $#, $?, $-, $$, $! 等特殊参数
                        std::string var_name(1, str[pos]);
                        result += expandVariable(var_name);
                        pos++;
                    } else {
                        // 单独的 $，保留原样
                        result += '$';
                    }
                } else {
                    // 字符串末尾的 $，保留原样
                    result += '$';
                }
            } else if (str[pos] == '`') {
                // 处理反引号命令替换
                size_t cmd_end = str.find('`', pos + 1);
                if (cmd_end != std::string::npos) {
                    std::string cmd = str.substr(pos + 1, cmd_end - pos - 1);
                    result += executeCommand(cmd);
                    pos = cmd_end + 1;
                } else {
                    // 未闭合的 `，保留原样
                    result += '`';
                    pos++;
                }
            } else {
                // 普通字符
                result += str[pos];
                pos++;
            }
        }
        
        return result;
    }

    std::string Expand::expandVariable(const std::string &name)
    {
        // 获取变量管理器
        VariableManager *var_manager = shell_->getVariableManager();
        if (!var_manager) {
            return "";
        }

        // 获取变量值
        return var_manager->getVariable(name);
    }

    std::string Expand::executeCommand(const std::string &command)
    {
        // 创建管道
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            std::cerr << "创建管道失败: " << strerror(errno) << std::endl;
            return "";
        }

        // 创建子进程
        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "创建子进程失败: " << strerror(errno) << std::endl;
            close(pipefd[0]);
            close(pipefd[1]);
            return "";
        }

        if (pid == 0) {
            // 子进程
            // 关闭读端
            close(pipefd[0]);

            // 将标准输出重定向到管道写端
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // 执行命令
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);

            // 如果execl失败
            std::cerr << "执行命令失败: " << strerror(errno) << std::endl;
            _exit(1);
        } else {
            // 父进程
            // 关闭写端
            close(pipefd[1]);

            // 从管道读取输出
            std::string output;
            char buffer[4096];
            ssize_t count;

            while ((count = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[count] = '\0';
                output += buffer;
            }

            // 关闭读端
            close(pipefd[0]);

            // 等待子进程结束
            int status;
            waitpid(pid, &status, 0);

            // 移除末尾的换行符
            while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
                output.pop_back();
            }

            return output;
        }

        return "";
    }

    bool Expand::containsWildcards(const std::string &str)
    {
        for (char c : str) {
            if (c == '*' || c == '?' || c == '[') {
                return true;
            }
        }
        return false;
    }

    bool Expand::expandGlob(const std::string &pattern, std::vector<std::string> &result)
    {
        glob_t glob_result;
        int ret = glob(pattern.c_str(), GLOB_NOSORT, nullptr, &glob_result);

        if (ret == 0) {
            // 成功匹配
            for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
                result.push_back(glob_result.gl_pathv[i]);
            }
            globfree(&glob_result);
            return true;
        } else if (ret == GLOB_NOMATCH) {
            // 没有匹配项
            return false;
        } else {
            // 发生错误
            std::cerr << "通配符展开失败: " << strerror(errno) << std::endl;
            globfree(&glob_result);
            return false;
        }
    }

    size_t Expand::findClosingBracket(const std::string &str, size_t start_pos, char open_bracket, char close_bracket)
    {
        int nesting = 1;
        size_t pos = start_pos + 1;

        while (pos < str.length() && nesting > 0) {
            if (str[pos] == open_bracket) {
                nesting++;
            } else if (str[pos] == close_bracket) {
                nesting--;
            }
            pos++;
        }

        if (nesting == 0) {
            return pos - 1;
        } else {
            return std::string::npos;
        }
    }

} // namespace dash 