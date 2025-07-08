/**
 * @file history.cpp
 * @brief 历史记录管理器实现
 */

#include "utils/history.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

namespace dash
{

    History::History()
        : max_size_(1000), start_index_(1), current_index_(0)
    {
    }

    History::~History()
    {
        save();
    }

    bool History::initialize(const std::string &filename)
    {
        // 设置历史文件名
        if (filename.empty()) {
            // 默认使用~/.dash_history
            const char *home = getenv("HOME");
            if (!home) {
                struct passwd *pw = getpwuid(getuid());
                if (pw) {
                    home = pw->pw_dir;
                }
            }
            
            if (home) {
                history_file_ = std::string(home) + "/.dash_history";
            } else {
                history_file_ = ".dash_history";
            }
        } else {
            history_file_ = filename;
        }

        // 加载历史记录
        return load();
    }

    bool History::load()
    {
        // 检查文件是否存在
        struct stat st;
        if (stat(history_file_.c_str(), &st) != 0) {
            // 文件不存在，创建空文件
            std::ofstream file(history_file_);
            if (!file.is_open()) {
                std::cerr << "无法创建历史文件: " << history_file_ << std::endl;
                return false;
            }
            file.close();
            return true;
        }

        // 打开文件
        std::ifstream file(history_file_);
        if (!file.is_open()) {
            std::cerr << "无法打开历史文件: " << history_file_ << std::endl;
            return false;
        }

        // 读取历史记录
        entries_.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                entries_.push_back(line);
            }
        }

        // 关闭文件
        file.close();

        // 如果历史记录超过最大大小，截断
        if (entries_.size() > max_size_) {
            entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - max_size_));
        }

        // 设置当前索引
        current_index_ = entries_.size();

        return true;
    }

    bool History::save() const
    {
        // 打开文件
        std::ofstream file(history_file_);
        if (!file.is_open()) {
            std::cerr << "无法打开历史文件进行写入: " << history_file_ << std::endl;
            return false;
        }

        // 写入历史记录
        for (const auto &entry : entries_) {
            file << entry << std::endl;
        }

        // 关闭文件
        file.close();

        return true;
    }

    void History::add(const std::string &entry)
    {
        // 忽略空条目
        if (entry.empty()) {
            return;
        }

        // 忽略与上一条相同的条目
        if (!entries_.empty() && entries_.back() == entry) {
            return;
        }

        // 添加条目
        entries_.push_back(entry);

        // 如果历史记录超过最大大小，移除最旧的条目
        if (entries_.size() > max_size_) {
            entries_.erase(entries_.begin());
        }

        // 更新当前索引
        current_index_ = entries_.size();
    }

    void History::clear()
    {
        entries_.clear();
        current_index_ = 0;
        save();
    }

    std::string History::getPrevious()
    {
        if (entries_.empty() || current_index_ <= 0) {
            return "";
        }

        current_index_--;
        return entries_[current_index_];
    }

    std::string History::getNext()
    {
        if (entries_.empty() || current_index_ >= entries_.size()) {
            return "";
        }

        current_index_++;
        if (current_index_ == entries_.size()) {
            return "";
        }

        return entries_[current_index_];
    }

    std::vector<std::string> History::getEntries() const
    {
        return entries_;
    }

    void History::setMaxSize(size_t size)
    {
        max_size_ = size;

        // 如果当前历史记录超过新的最大大小，截断
        if (entries_.size() > max_size_) {
            entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - max_size_));
            current_index_ = std::min(current_index_, entries_.size());
        }
    }

    size_t History::getMaxSize() const
    {
        return max_size_;
    }

    int History::getStartIndex() const
    {
        return start_index_;
    }

    void History::setStartIndex(int index)
    {
        start_index_ = index;
    }

    std::string History::search(const std::string &prefix) const
    {
        // 从当前位置向后搜索
        for (size_t i = current_index_; i > 0; --i) {
            if (entries_[i - 1].compare(0, prefix.size(), prefix) == 0) {
                return entries_[i - 1];
            }
        }

        return "";
    }

    std::string History::searchReverse(const std::string &prefix) const
    {
        // 从当前位置向前搜索
        for (size_t i = current_index_ + 1; i <= entries_.size(); ++i) {
            if (entries_[i - 1].compare(0, prefix.size(), prefix) == 0) {
                return entries_[i - 1];
            }
        }

        return "";
    }

} // namespace dash 