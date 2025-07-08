/**
 * @file alias.cpp
 * @brief 别名管理实现
 */

#include "core/alias.h"
#include "core/shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

namespace dash
{

    Alias::Alias(Shell *shell)
        : shell_(shell)
    {
    }

    Alias::~Alias()
    {
        save();
    }

    bool Alias::initialize(const std::string &filename)
    {
        // 设置别名文件名
        if (filename.empty()) {
            // 默认使用~/.dash_aliases
            const char *home = getenv("HOME");
            if (!home) {
                struct passwd *pw = getpwuid(getuid());
                if (pw) {
                    home = pw->pw_dir;
                }
            }
            
            if (home) {
                alias_file_ = std::string(home) + "/.dash_aliases";
            } else {
                alias_file_ = ".dash_aliases";
            }
        } else {
            alias_file_ = filename;
        }

        // 加载别名
        return load();
    }

    bool Alias::load()
    {
        // 检查文件是否存在
        struct stat st;
        if (stat(alias_file_.c_str(), &st) != 0) {
            // 文件不存在，创建空文件
            std::ofstream file(alias_file_);
            if (!file.is_open()) {
                std::cerr << "无法创建别名文件: " << alias_file_ << std::endl;
                return false;
            }
            file.close();
            return true;
        }

        // 打开文件
        std::ifstream file(alias_file_);
        if (!file.is_open()) {
            std::cerr << "无法打开别名文件: " << alias_file_ << std::endl;
            return false;
        }

        // 读取别名
        aliases_.clear();
        std::string line;
        while (std::getline(file, line)) {
            // 忽略空行和注释行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 移除行尾的注释
            size_t comment_pos = line.find('#');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            
            // 移除行首和行尾的空白字符
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            // 忽略空行
            if (line.empty()) {
                continue;
            }
            
            // 解析别名
            if (line.compare(0, 6, "alias ") == 0) {
                line = line.substr(6);
                
                // 查找第一个等号
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos) {
                    std::string name = line.substr(0, eq_pos);
                    std::string value = line.substr(eq_pos + 1);
                    
                    // 移除名称和值的空白字符
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    // 如果值被引号包围，移除引号
                    if ((value[0] == '"' && value[value.length() - 1] == '"') ||
                        (value[0] == '\'' && value[value.length() - 1] == '\'')) {
                        value = value.substr(1, value.length() - 2);
                    }
                    
                    // 添加别名
                    if (!name.empty()) {
                        aliases_[name] = value;
                    }
                }
            }
        }

        // 关闭文件
        file.close();

        return true;
    }

    bool Alias::save() const
    {
        // 打开文件
        std::ofstream file(alias_file_);
        if (!file.is_open()) {
            std::cerr << "无法打开别名文件进行写入: " << alias_file_ << std::endl;
            return false;
        }

        // 写入别名
        file << "# Dash Shell 别名文件" << std::endl;
        file << "# 此文件由 Dash Shell 自动生成" << std::endl;
        file << std::endl;

        for (const auto &alias : aliases_) {
            file << "alias " << alias.first << "='" << alias.second << "'" << std::endl;
        }

        // 关闭文件
        file.close();

        return true;
    }

    bool Alias::add(const std::string &name, const std::string &value)
    {
        // 检查名称是否为空
        if (name.empty()) {
            return false;
        }

        // 添加别名
        aliases_[name] = value;
        return true;
    }

    bool Alias::remove(const std::string &name)
    {
        // 检查别名是否存在
        auto it = aliases_.find(name);
        if (it == aliases_.end()) {
            return false;
        }

        // 移除别名
        aliases_.erase(it);
        return true;
    }

    bool Alias::has(const std::string &name) const
    {
        return aliases_.find(name) != aliases_.end();
    }

    std::string Alias::get(const std::string &name) const
    {
        // 检查别名是否存在
        auto it = aliases_.find(name);
        if (it == aliases_.end()) {
            return "";
        }

        return it->second;
    }

    std::map<std::string, std::string> Alias::getAll() const
    {
        return aliases_;
    }

    std::string Alias::expand(const std::string &command) const
    {
        // 解析命令
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        // 检查是否是别名
        auto it = aliases_.find(cmd);
        if (it == aliases_.end()) {
            return command;
        }

        // 获取命令的其余部分
        std::string rest;
        std::getline(iss, rest);

        // 展开别名
        return it->second + rest;
    }

} // namespace dash 