/**
 * @file history.h
 * @brief 命令历史记录
 */

#ifndef DASH_HISTORY_H
#define DASH_HISTORY_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../dash.h"

namespace dash {

/**
 * @brief 历史记录条目
 */
struct HistoryEntry {
    int index;              // 历史记录索引
    std::string command;    // 命令内容
    time_t timestamp;       // 时间戳
};

/**
 * @brief 历史记录管理类
 */
class History {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     * @param maxSize 最大历史记录数量
     */
    explicit History(Shell& shell, size_t maxSize = 1000);

    /**
     * @brief 析构函数
     */
    ~History();

    /**
     * @brief 添加命令到历史记录
     * 
     * @param command 命令字符串
     */
    void addCommand(const std::string& command);

    /**
     * @brief 获取指定索引的历史记录
     * 
     * @param index 历史记录索引
     * @return const HistoryEntry* 历史记录条目，如果不存在则返回nullptr
     */
    const HistoryEntry* getCommand(int index) const;

    /**
     * @brief 获取最近的N条历史记录
     * 
     * @param count 要获取的历史记录数量
     * @return std::vector<HistoryEntry> 历史记录列表
     */
    std::vector<HistoryEntry> getRecentCommands(size_t count) const;

    /**
     * @brief 获取所有历史记录
     * 
     * @return const std::vector<HistoryEntry>& 历史记录列表
     */
    const std::vector<HistoryEntry>& getAllCommands() const;

    /**
     * @brief 清除历史记录
     */
    void clear();

    /**
     * @brief 从文件加载历史记录
     * 
     * @param filename 历史记录文件名
     * @return bool 是否成功加载
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief 保存历史记录到文件
     * 
     * @param filename 历史记录文件名
     * @return bool 是否成功保存
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief 搜索历史记录
     * 
     * @param pattern 搜索模式
     * @return std::vector<HistoryEntry> 匹配的历史记录
     */
    std::vector<HistoryEntry> searchCommands(const std::string& pattern) const;

private:
    Shell& shell_;                  // Shell实例引用
    std::vector<HistoryEntry> history_;  // 历史记录列表
    size_t maxSize_;                // 最大历史记录数量
    int nextIndex_;                 // 下一条历史记录的索引
};

} // namespace dash

#endif // DASH_HISTORY_H 