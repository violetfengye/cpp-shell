/**
 * @file debug.h
 * @brief 调试工具头文件
 */

#ifndef DASH_DEBUG_H
#define DASH_DEBUG_H

#include <string>
#include <iostream>
#include <sstream>

namespace dash
{

    /**
     * @brief 调试级别
     */
    enum class DebugLevel
    {
        NONE,    ///< 不输出任何调试信息
        ERROR,   ///< 只输出错误信息
        WARNING, ///< 输出警告和错误信息
        INFO,    ///< 输出一般信息、警告和错误信息
        DEBUG,   ///< 输出调试信息、一般信息、警告和错误信息
        TRACE    ///< 输出跟踪信息、调试信息、一般信息、警告和错误信息
    };

    /**
     * @brief 调试工具类
     */
    class Debug
    {
    public:
        /**
         * @brief 获取调试工具实例
         * @return 调试工具实例
         */
        static Debug &getInstance();

        /**
         * @brief 设置调试级别
         * @param level 调试级别
         */
        void setLevel(DebugLevel level);

        /**
         * @brief 获取调试级别
         * @return 调试级别
         */
        DebugLevel getLevel() const;

        /**
         * @brief 设置是否启用调试
         * @param enabled 是否启用
         */
        void setEnabled(bool enabled);

        /**
         * @brief 检查是否启用调试
         * @return 是否启用
         */
        bool isEnabled() const;

        /**
         * @brief 输出错误信息
         * @param msg 错误信息
         */
        void error(const std::string &msg);

        /**
         * @brief 输出警告信息
         * @param msg 警告信息
         */
        void warning(const std::string &msg);

        /**
         * @brief 输出一般信息
         * @param msg 一般信息
         */
        void info(const std::string &msg);

        /**
         * @brief 输出调试信息
         * @param msg 调试信息
         */
        void debug(const std::string &msg);

        /**
         * @brief 输出跟踪信息
         * @param msg 跟踪信息
         */
        void trace(const std::string &msg);

    private:
        /**
         * @brief 构造函数
         */
        Debug();

        /**
         * @brief 析构函数
         */
        ~Debug();

        /**
         * @brief 输出调试信息
         * @param level 调试级别
         * @param msg 调试信息
         */
        void log(DebugLevel level, const std::string &msg);

    private:
        DebugLevel level_; ///< 调试级别
        bool enabled_;     ///< 是否启用调试
    };

    /**
     * @brief 调试宏
     */
    #define DEBUG_ERROR(msg) dash::Debug::getInstance().error(msg)
    #define DEBUG_WARNING(msg) dash::Debug::getInstance().warning(msg)
    #define DEBUG_INFO(msg) dash::Debug::getInstance().info(msg)
    #define DEBUG_DEBUG(msg) dash::Debug::getInstance().debug(msg)
    #define DEBUG_TRACE(msg) dash::Debug::getInstance().trace(msg)

} // namespace dash

#endif // DASH_DEBUG_H 