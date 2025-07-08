/**
 * @file history.h
 * @brief 历史记录管理器头文件
 */

#ifndef DASH_HISTORY_H
#define DASH_HISTORY_H

#include <string>
#include <vector>

namespace dash
{
    /**
     * @brief 历史记录管理器类
     */
    class History
    {
    public:
        /**
         * @brief 构造函数
         */
        History();

        /**
         * @brief 析构函数
         */
        ~History();

        /**
         * @brief 初始化历史记录管理器
         * @param filename 历史记录文件名，为空则使用默认文件
         * @return 是否初始化成功
         */
        bool initialize(const std::string &filename = "");

        /**
         * @brief 加载历史记录
         * @return 是否加载成功
         */
        bool load();

        /**
         * @brief 保存历史记录
         * @return 是否保存成功
         */
        bool save() const;

        /**
         * @brief 添加历史记录条目
         * @param entry 历史记录条目
         */
        void add(const std::string &entry);

        /**
         * @brief 清除历史记录
         */
        void clear();

        /**
         * @brief 获取上一条历史记录
         * @return 上一条历史记录，如果没有则返回空字符串
         */
        std::string getPrevious();

        /**
         * @brief 获取下一条历史记录
         * @return 下一条历史记录，如果没有则返回空字符串
         */
        std::string getNext();

        /**
         * @brief 获取所有历史记录条目
         * @return 历史记录条目列表
         */
        std::vector<std::string> getEntries() const;

        /**
         * @brief 设置历史记录最大大小
         * @param size 最大大小
         */
        void setMaxSize(size_t size);

        /**
         * @brief 获取历史记录最大大小
         * @return 最大大小
         */
        size_t getMaxSize() const;

        /**
         * @brief 获取历史记录起始索引
         * @return 起始索引
         */
        int getStartIndex() const;

        /**
         * @brief 设置历史记录起始索引
         * @param index 起始索引
         */
        void setStartIndex(int index);

        /**
         * @brief 搜索历史记录
         * @param prefix 前缀
         * @return 匹配的历史记录条目，如果没有则返回空字符串
         */
        std::string search(const std::string &prefix) const;

        /**
         * @brief 反向搜索历史记录
         * @param prefix 前缀
         * @return 匹配的历史记录条目，如果没有则返回空字符串
         */
        std::string searchReverse(const std::string &prefix) const;

    private:
        std::string history_file_;      ///< 历史记录文件名
        std::vector<std::string> entries_; ///< 历史记录条目
        size_t max_size_;               ///< 历史记录最大大小
        int start_index_;               ///< 历史记录起始索引
        size_t current_index_;          ///< 当前索引
    };

} // namespace dash

#endif // DASH_HISTORY_H 