/**
 * @file history.cpp
 * @brief 命令历史记录实现
 */

#include "../../include/utils/history.h"
#include "../../include/core/shell.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <regex>
#include "../core/debug.h"

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
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        
        // 尝试解析格式：时间戳 命令
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            try {
                // 尝试将第一部分解析为时间戳
                std::string timestampStr = line.substr(0, spacePos);
                time_t timestamp = std::stoll(timestampStr);
                
                // 如果成功，创建带时间戳的历史条目
                HistoryEntry entry;
                entry.index = nextIndex_++;
                entry.timestamp = timestamp;
                entry.command = line.substr(spacePos + 1);
                history_.push_back(entry);
            } catch (const std::exception& e) {
                // 如果解析时间戳失败，将整行视为命令
                DebugLog::logCommand("警告: 解析历史记录行失败: " + line + " (" + e.what() + ")");
                DebugLog::logCommand("尝试将整行作为命令添加到历史记录");
                
                HistoryEntry entry;
                entry.index = nextIndex_++;
                entry.timestamp = std::time(nullptr);  // 使用当前时间作为时间戳
                entry.command = line;
                history_.push_back(entry);
            }
        } else {
            // 如果没有空格，将整行视为命令
            HistoryEntry entry;
            entry.index = nextIndex_++;
            entry.timestamp = std::time(nullptr);  // 使用当前时间作为时间戳
            entry.command = line;
            history_.push_back(entry);
        }
    }
    
    return true;
}

bool History::saveToFile(const std::string& filename) const {
    // 以截断模式打开文件，确保文件为空
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (!file) {
        return false;
    }
    
    for (const auto& entry : history_) {
        // 确保时间戳是有效的
        time_t timestamp = entry.timestamp;
        if (timestamp <= 0) {
            timestamp = std::time(nullptr);  // 如果时间戳无效，使用当前时间
        }
        
        // 保存格式：时间戳 命令
        file << timestamp << ' ' << entry.command << '\n';
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