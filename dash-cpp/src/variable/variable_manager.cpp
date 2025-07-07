/**
 * @file variable_manager.cpp
 * @brief 变量管理器类实现
 */

#include <iostream>
#include <cstdlib>
#include <regex>
#include <unistd.h>
#include <sys/wait.h>
#include "variable/variable_manager.h"
#include "core/shell.h"
#include "utils/error.h"
#include "variable/prompt_string.h"

extern char **environ;

namespace dash
{

    // Variable 实现

    Variable::Variable(const std::string &name, const std::string &value, int flags)
        : name_(name), value_(value), flags_(flags)
    {
    }

    bool Variable::setValue(const std::string &value)
    {
        // 如果变量是只读的，则不能修改
        if (hasFlag(VAR_READONLY))
        {
            return false;
        }

        value_ = value;
        return true;
    }

    // VariableManager 实现

    VariableManager::VariableManager(Shell *shell)
        : shell_(shell)
    {
        initialize();
    }

    VariableManager::~VariableManager()
    {
        // 清空变量表
        variables_.clear();
    }

    void VariableManager::initialize()
    {
        // 导入环境变量
        for (char **env = environ; env && *env; ++env)
        {
            std::string entry = *env;
            size_t pos = entry.find('=');
            if (pos != std::string::npos)
            {
                std::string name = entry.substr(0, pos);
                std::string value = entry.substr(pos + 1);
                set(name, value, Variable::VAR_EXPORT);
            }
        }

        // 设置一些默认变量
        set("PS1", "$ ", Variable::VAR_READONLY | Variable::VAR_UPDATE_ON_READ);// 修改为只读，且在读时需要更新属性
        set("FPS1", "$ ", Variable::VAR_READONLY | Variable::VAR_UPDATE_ON_READ);// 修改为只读，且在读时需要更新属性
        setUpdateValueFunc("PS1", prompt_string::getRawPrompt);
        setUpdateValueFunc("FPS1", prompt_string::getFormattedPrompt);
        set("PS2", "> ", Variable::VAR_NONE);
        set("IFS", " \t\n", Variable::VAR_NONE);

        // 设置特殊变量
        set("?", "0", Variable::VAR_SPECIAL);
        set("$", std::to_string(getpid()), Variable::VAR_SPECIAL);

        // 设置 PATH 如果不存在
        if (!exists("PATH"))
        {
            set("PATH", "/usr/local/bin:/usr/bin:/bin", Variable::VAR_EXPORT);
        }

        // 设置 HOME 如果不存在
        if (!exists("HOME"))
        {
            const char *home = getenv("HOME");
            if (home)
            {
                set("HOME", home, Variable::VAR_EXPORT);
            }
            else
            {
                set("HOME", "/", Variable::VAR_EXPORT);
            }
        }
    }

    bool VariableManager::set(const std::string &name, const std::string &value, int flags)
    {
        // 检查变量名是否有效
        if (name.empty())
        {
            return false;
        }

        // 检查是否是特殊变量
        if (name == "?" || name == "$" || name == "#" || name == "0")
        {
            flags |= Variable::VAR_SPECIAL;
        }

        // 如果变量已存在
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // 如果变量是只读的，则不能修改
            if (it->second->hasFlag(Variable::VAR_READONLY))
            {
                return false;
            }

            // 更新变量值和标志
            it->second->setValue(value);

            // 保留原有标志，添加新标志
            int new_flags = it->second->getFlags() | flags;
            it->second->setFlags(new_flags);

            // 如果变量是导出的，则更新环境变量
            if (it->second->hasFlag(Variable::VAR_EXPORT))
            {
                setenv(name.c_str(), value.c_str(), 1);
            }
        }
        else
        {
            // 创建新变量
            variables_[name] = std::make_unique<Variable>(name, value, flags);

            // 如果变量是导出的，则设置环境变量
            if (flags & Variable::VAR_EXPORT)
            {
                setenv(name.c_str(), value.c_str(), 1);
            }
        }

        return true;
    }

    void VariableManager::setUpdateValueFunc(const std::string &name, std::string (*func)()) {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            it->second->setUpdateValueFunc(func);
        }
    }

    std::string VariableManager::get(const std::string &name) const
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {

            return it->second->getValue();
        }

        return "";
    }

    bool VariableManager::exists(const std::string &name) const
    {
        return variables_.find(name) != variables_.end();
    }

    bool VariableManager::unset(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // 如果变量是只读的或特殊的，则不能删除
            if (it->second->hasFlag(Variable::VAR_READONLY) ||
                it->second->hasFlag(Variable::VAR_SPECIAL))
            {
                return false;
            }

            // 如果变量是导出的，则从环境中删除
            if (it->second->hasFlag(Variable::VAR_EXPORT))
            {
                unsetenv(name.c_str());
            }

            // 从变量表中删除
            variables_.erase(it);
            return true;
        }

        return false;
    }

    bool VariableManager::exportVar(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // 添加导出标志
            it->second->addFlag(Variable::VAR_EXPORT);

            // 设置环境变量
            setenv(name.c_str(), it->second->getValue().c_str(), 1);
            return true;
        }

        return false;
    }

    bool VariableManager::setReadOnly(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // 添加只读标志
            it->second->addFlag(Variable::VAR_READONLY);
            return true;
        }

        return false;
    }

    std::vector<std::string> VariableManager::getAllNames() const
    {
        std::vector<std::string> names;
        names.reserve(variables_.size());

        for (const auto &pair : variables_)
        {
            names.push_back(pair.first);
        }

        return names;
    }

    std::vector<std::pair<std::string, std::string>> VariableManager::getExportVars() const
    {
        std::vector<std::pair<std::string, std::string>> exports;

        for (const auto &pair : variables_)
        {
            if (pair.second->hasFlag(Variable::VAR_EXPORT))
            {
                exports.emplace_back(pair.first, pair.second->getValue());
            }
        }

        return exports;
    }

    std::vector<std::string> VariableManager::getEnvironment() const
    {
        std::vector<std::string> env;

        for (const auto &pair : variables_)
        {
            if (pair.second->hasFlag(Variable::VAR_EXPORT))
            {
                env.push_back(pair.first + "=" + pair.second->getValue());
            }
        }

        return env;
    }

    std::string VariableManager::expand(const std::string &str) const
    {
        std::string result;
        size_t i = 0;
        
        while (i < str.length())
        {
            // 处理命令替换 $(command)
            if (i + 1 < str.length() && str[i] == '$' && str[i + 1] == '(')
            {
                i += 2; // 跳过 $(
                size_t start = i;
                int paren_count = 1;
                
                // 查找匹配的右括号
                while (i < str.length() && paren_count > 0)
                {
                    if (str[i] == '(')
                    {
                        paren_count++;
                    }
                    else if (str[i] == ')')
                    {
                        paren_count--;
                    }
                    
                    if (paren_count > 0)
                    {
                        i++;
                    }
                }
                
                if (i < str.length() && paren_count == 0)
                {
                    // 提取命令
                    std::string cmd = str.substr(start, i - start);
                    i++; // 跳过 )
                    
                    // 执行命令并获取输出
                    std::string output = executeCommandSubstitution(cmd);
                    
                    // 移除尾部的换行符
                    if (!output.empty() && output[output.length() - 1] == '\n')
                    {
                        output.pop_back();
                    }
                    
                    // 添加到结果
                    result += output;
                }
                else
                {
                    // 未闭合的括号，保持原样
                    result += "$(" + str.substr(start, i - start);
                }
            }
            // 处理命令替换 `command`
            else if (str[i] == '`')
            {
                i++; // 跳过开始的 `
                size_t start = i;
                
                // 查找匹配的反引号
                while (i < str.length() && str[i] != '`')
                {
                    i++;
                }
                
                if (i < str.length() && str[i] == '`')
                {
                    // 提取命令
                    std::string cmd = str.substr(start, i - start);
                    i++; // 跳过结束的 `
                    
                    // 执行命令并获取输出
                    std::string output = executeCommandSubstitution(cmd);
                    
                    // 移除尾部的换行符
                    if (!output.empty() && output[output.length() - 1] == '\n')
                    {
                        output.pop_back();
                    }
                    
                    // 添加到结果
                    result += output;
                }
                else
                {
                    // 未闭合的反引号，保持原样
                    result += "`" + str.substr(start, i - start);
                }
            }
            // 处理变量引用
            else if (str[i] == '$' && i + 1 < str.length())
            {
                // 找到变量引用
                size_t start = i;
                i++; // 跳过 $
                
                // 处理 ${name} 形式
                if (str[i] == '{')
                {
                    i++; // 跳过 {
                    size_t name_start = i;
                    while (i < str.length() && str[i] != '}')
                    {
                        i++;
                    }
                    
                    if (i < str.length() && str[i] == '}')
                    {
                        std::string var_name = str.substr(name_start, i - name_start);
                        result += get(var_name);
                        i++; // 跳过 }
                    }
                    else
                    {
                        // 未闭合的 ${，保持原样
                        result += str.substr(start, i - start);
                    }
                }
                // 处理特殊变量
                else if (str[i] == '$' || str[i] == '?' || str[i] == '#' || (str[i] >= '0' && str[i] <= '9'))
                {
                    std::string var_name = str.substr(i, 1);
                    result += get(var_name);
                    i++;
                }
                // 处理普通变量
                else if (isalpha(str[i]) || str[i] == '_')
                {
                    size_t name_start = i;
                    while (i < str.length() && (isalnum(str[i]) || str[i] == '_'))
                    {
                        i++;
                    }
                    
                    std::string var_name = str.substr(name_start, i - name_start);
                    result += get(var_name);
                }
                else
                {
                    // 单独的 $，保持原样
                    result += '$';
                }
            }
            else
            {
                result += str[i];
                i++;
            }
        }
        
        return result;
    }

    // 执行命令替换并返回输出
    std::string VariableManager::executeCommandSubstitution(const std::string &cmd) const
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            return "";
        }
        
        pid_t pid = fork();
        if (pid == -1)
        {
            close(pipefd[0]);
            close(pipefd[1]);
            return "";
        }
        
        if (pid == 0)
        {
            // 子进程
            close(pipefd[0]); // 关闭读端
            
            // 重定向标准输出到管道写端
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            
            // 执行命令
            execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
            
            // 如果 execl 返回，则表示执行失败
            exit(1);
        }
        
        // 父进程
        close(pipefd[1]); // 关闭写端
        
        // 从管道读取输出
        std::string output;
        char buffer[4096];
        ssize_t n;
        
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[n] = '\0';
            output += buffer;
        }
        
        close(pipefd[0]);
        
        // 等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        
        return output;
    }

    void VariableManager::updateSpecialVars(int exit_status)
    {
        // 更新 $? (上一个命令的退出状态)
        set("?", std::to_string(exit_status), Variable::VAR_SPECIAL);

        // 更新 $$ (当前进程ID)
        set("$", std::to_string(getpid()), Variable::VAR_SPECIAL);

        // TODO: 更新 $# (位置参数数量) 和 $0 (脚本名称) 等其他特殊变量
    }

} // namespace dash