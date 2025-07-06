/**
 * @file expand.cpp
 * @brief 路径和变量扩展实现
 */

#include "../../include/core/expand.h"
#include "../../include/core/shell.h"
#include "../../include/variable/variable_manager.h"
#include "../../include/core/arithmetic.h"
#include <glob.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex>
#include <cstring>
#include <sstream>
#include <array>

namespace dash {

Expand::Expand(Shell& shell) : shell_(shell) {
    // 初始化
}

Expand::~Expand() {
    // 清理资源
}

ExpandResult Expand::expandWord(const std::string& word) {
    ExpandResult result;
    result.success = true;
    
    try {
        // 处理引号
        std::string unquoted = handleQuotes(word);
        
        // 处理波浪线扩展
        unquoted = expandTilde(unquoted);
        
        // 处理变量扩展
        unquoted = expandVariable(unquoted);
        
        // 处理命令扩展
        // 这里需要检测 $() 和 `` 格式的命令替换
        // 实现较复杂，暂时留空，后续完善
        
        // 处理算术扩展
        // 这里需要检测 $(()) 格式的算术扩展
        // 实现较复杂，暂时留空，后续完善
        
        // 处理路径名扩展
        std::vector<std::string> expanded = expandPathname(unquoted);
        
        if (expanded.empty()) {
            // 如果没有匹配的路径，则使用原始单词
            result.words.push_back(unquoted);
        } else {
            result.words = std::move(expanded);
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    
    return result;
}

std::vector<std::string> Expand::expandPathname(const std::string& pattern) {
    std::vector<std::string> result;
    
    // 检查是否包含通配符
    if (pattern.find_first_of("*?[") == std::string::npos) {
        // 没有通配符，直接返回原始模式
        result.push_back(pattern);
        return result;
    }
    
    glob_t globbuf;
    int flags = GLOB_NOCHECK;
    
    if (glob(pattern.c_str(), flags, nullptr, &globbuf) == 0) {
        for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
            result.push_back(globbuf.gl_pathv[i]);
        }
        globfree(&globbuf);
    }
    
    return result;
}

std::string Expand::expandTilde(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    
    // 提取用户名（如果有）
    size_t slashPos = path.find('/');
    std::string username;
    
    if (slashPos == 1) {
        // ~/ 格式，使用当前用户的主目录
        username = "";
    } else if (slashPos == std::string::npos) {
        // ~ 或 ~username 格式，没有后续路径
        username = (path.size() > 1) ? path.substr(1) : "";
    } else {
        // ~username/ 格式
        username = path.substr(1, slashPos - 1);
    }
    
    // 获取用户的主目录
    std::string homedir;
    if (username.empty()) {
        // 当前用户的主目录
        const char* home = getenv("HOME");
        if (home) {
            homedir = home;
        } else {
            // 如果环境变量不可用，则使用passwd文件
            struct passwd* pw = getpwuid(getuid());
            if (pw) {
                homedir = pw->pw_dir;
            } else {
                // 无法确定主目录，返回原始路径
                return path;
            }
        }
    } else {
        // 指定用户的主目录
        struct passwd* pw = getpwnam(username.c_str());
        if (pw) {
            homedir = pw->pw_dir;
        } else {
            // 用户不存在，返回原始路径
            return path;
        }
    }
    
    // 替换波浪线部分为主目录
    if (slashPos == std::string::npos) {
        return homedir;
    } else {
        return homedir + path.substr(slashPos);
    }
}

std::string Expand::expandVariable(const std::string& str) {
    std::string result;
    size_t pos = 0;
    
    while (pos < str.size()) {
        size_t dollarPos = str.find('$', pos);
        
        if (dollarPos == std::string::npos) {
            // 没有更多的变量引用
            result += str.substr(pos);
            break;
        }
        
        // 添加变量前的部分
        result += str.substr(pos, dollarPos - pos);
        
        // 跳过 $ 字符
        pos = dollarPos + 1;
        
        if (pos >= str.size()) {
            // $ 在字符串末尾，直接添加
            result += '$';
            break;
        }
        
        if (str[pos] == '{') {
            // ${VAR} 格式
            size_t endPos = str.find('}', pos + 1);
            if (endPos == std::string::npos) {
                // 未闭合的 ${ ，保留原始文本
                result += "${";
                pos += 1;
            } else {
                std::string varName = str.substr(pos + 1, endPos - pos - 1);
                result += shell_.getVariableManager()->get(varName);
                pos = endPos + 1;
            }
        } else if (std::isalpha(str[pos]) || str[pos] == '_') {
            // $VAR 格式
            size_t endPos = pos;
            while (endPos < str.size() && (std::isalnum(str[endPos]) || str[endPos] == '_')) {
                ++endPos;
            }
            std::string varName = str.substr(pos, endPos - pos);
            result += shell_.getVariableManager()->get(varName);
            pos = endPos;
        } else if (str[pos] == '$') {
            // $$ 格式，表示进程ID
            result += std::to_string(getpid());
            pos += 1;
        } else if (str[pos] == '?') {
            // $? 格式，表示上一个命令的退出状态
            result += std::to_string(shell_.getExitStatus());
            pos += 1;
        } else {
            // 其他格式，保留原始 $ 字符
            result += '$';
        }
    }
    
    return result;
}

std::string Expand::expandCommand(const std::string& command) {
    // 执行命令并获取输出
    // 这是一个简单的实现，实际应使用管道和fork/exec
    std::array<char, 4096> buffer;
    std::string result;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("无法执行命令: " + command);
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    // 去除结尾的换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    pclose(pipe);
    return result;
}

std::string Expand::expandArithmetic(const std::string& expression) {
    // 使用算术处理类计算表达式
    long result = 0;
    try {
        // 在实际实现中，应该调用Arithmetic类来计算表达式
        // Arithmetic类尚未完全实现，这里使用临时占位代码
        result = 0;  // 临时占位
        
        // 简单处理表达式以消除未使用参数警告
        if (!expression.empty()) {
            // 尝试将表达式转换为数字，如果可能的话
            try {
                result = std::stol(expression);
            } catch (...) {
                // 忽略转换错误
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("算术表达式错误: " + std::string(e.what()));
    }
    
    return std::to_string(result);
}

std::string Expand::handleQuotes(const std::string& str) {
    std::string result;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        
        if (c == '\\' && !inSingleQuote) {
            // 处理转义字符
            if (i + 1 < str.size()) {
                result += str[i + 1];
                ++i;
            }
        } else if (c == '\'' && !inDoubleQuote) {
            // 切换单引号状态
            inSingleQuote = !inSingleQuote;
        } else if (c == '"' && !inSingleQuote) {
            // 切换双引号状态
            inDoubleQuote = !inDoubleQuote;
        } else {
            // 添加普通字符
            result += c;
        }
    }
    
    // 未闭合的引号视为错误
    if (inSingleQuote || inDoubleQuote) {
        throw std::runtime_error("未闭合的引号");
    }
    
    return result;
}

bool Expand::matchPattern(const std::string& pattern, const std::string& str) {
    // 简单的通配符匹配实现
    // 实际实现应该使用更健壮的方法，如正则表达式或fnmatch
    
    // 这里添加一个简单的实现以消除未使用参数警告
    if (pattern.empty() && str.empty()) {
        return true;
    } else if (pattern == "*") {
        return true;  // 星号匹配任何内容
    } else if (pattern == "?" && str.length() == 1) {
        return true;  // 问号匹配单个字符
    } else if (pattern == str) {
        return true;  // 精确匹配
    }
    
    // 此处仅作为占位符，实际应该实现完整的通配符匹配逻辑
    return false;
}

std::vector<std::string> Expand::splitWords(const std::string& str) {
    std::vector<std::string> words;
    std::string word;
    bool inWord = false;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    
    for (char c : str) {
        if (c == '\'' && !inDoubleQuote) {
            // 切换单引号状态
            inSingleQuote = !inSingleQuote;
            inWord = true;
            word += c;
        } else if (c == '"' && !inSingleQuote) {
            // 切换双引号状态
            inDoubleQuote = !inDoubleQuote;
            inWord = true;
            word += c;
        } else if (std::isspace(c) && !inSingleQuote && !inDoubleQuote) {
            // 遇到空白字符，结束当前单词
            if (inWord) {
                words.push_back(word);
                word.clear();
                inWord = false;
            }
        } else {
            // 添加普通字符
            inWord = true;
            word += c;
        }
    }
    
    // 添加最后一个单词
    if (inWord) {
        words.push_back(word);
    }
    
    return words;
}

} // namespace dash 