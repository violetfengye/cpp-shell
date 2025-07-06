/**
 * @file history.cpp
 * @brief 命令历史记录实现
 */

#include "../../include/utils/history.h"
#include "../../include/core/shell.h"
#include <fstream>
#include <algorithm>
#include <ctime>
#include <regex>

namespace dash {

History::History(Shell& shell, size_t maxSize)
    : shell_(shell), maxSize_(maxSize), nextIndex_(1) {
    // 初始化
}

History::~History() {
    // 清理资源
}

void History::addCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }
    
    // 避免重复连续的命令
    if (!history_.empty() && history_.back().command == command) {
        return;
    }
    
    // 添加新的历史记录
    HistoryEntry entry;
    entry.index = nextIndex_++;
    entry.command = command;
    entry.timestamp = std::time(nullptr);
    
    history_.push_back(entry);
    
    // 如果超过最大记录数，删除最旧的记录
    if (history_.size() > maxSize_) {
        history_.erase(history_.begin());
    }
}

const HistoryEntry* History::getCommand(int index) const {
    for (const auto& entry : history_) {
        if (entry.index == index) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<HistoryEntry> History::getRecentCommands(size_t count) const {
    std::vector<HistoryEntry> result;
    size_t size = history_.size();
    
    if (count > size) {
        count = size;
    }
    
    result.reserve(count);
    auto startIter = history_.end() - count;
    
    for (auto it = startIter; it != history_.end(); ++it) {
        result.push_back(*it);
    }
    
    return result;
}

const std::vector<HistoryEntry>& History::getAllCommands() const {
    return history_;
}

void History::clear() {
    history_.clear();
    nextIndex_ = 1;
}

bool History::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        return false;
    }
    
    clear();
    
    std::string line;
    while (std::getline(file, line)) {
        // 简单格式：时间戳 命令
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            HistoryEntry entry;
            entry.index = nextIndex_++;
            entry.timestamp = std::stoll(line.substr(0, spacePos));
            entry.command = line.substr(spacePos + 1);
            history_.push_back(entry);
        }
    }
    
    return true;
}

bool History::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        return false;
    }
    
    for (const auto& entry : history_) {
        file << entry.timestamp << ' ' << entry.command << '\n';
    }
    
    return file.good();
}

std::vector<HistoryEntry> History::searchCommands(const std::string& pattern) const {
    std::vector<HistoryEntry> result;
    
    try {
        std::regex regex(pattern);
        for (const auto& entry : history_) {
            if (std::regex_search(entry.command, regex)) {
                result.push_back(entry);
            }
        }
    } catch (const std::regex_error&) {
        // 正则表达式错误，使用普通的子串搜索
        for (const auto& entry : history_) {
            if (entry.command.find(pattern) != std::string::npos) {
                result.push_back(entry);
            }
        }
    }
    
    return result;
}

} // namespace dash 