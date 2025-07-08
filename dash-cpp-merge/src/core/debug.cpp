/**
 * @file debug.cpp
 * @brief 调试工具实现
 */

#include "core/debug.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace dash
{

    Debug::Debug()
        : level_(DebugLevel::NONE), enabled_(false)
    {
    }

    Debug::~Debug()
    {
    }

    Debug &Debug::getInstance()
    {
        static Debug instance;
        return instance;
    }

    void Debug::setLevel(DebugLevel level)
    {
        level_ = level;
    }

    DebugLevel Debug::getLevel() const
    {
        return level_;
    }

    void Debug::setEnabled(bool enabled)
    {
        enabled_ = enabled;
    }

    bool Debug::isEnabled() const
    {
        return enabled_;
    }

    void Debug::error(const std::string &msg)
    {
        log(DebugLevel::ERROR, msg);
    }

    void Debug::warning(const std::string &msg)
    {
        log(DebugLevel::WARNING, msg);
    }

    void Debug::info(const std::string &msg)
    {
        log(DebugLevel::INFO, msg);
    }

    void Debug::debug(const std::string &msg)
    {
        log(DebugLevel::DEBUG, msg);
    }

    void Debug::trace(const std::string &msg)
    {
        log(DebugLevel::TRACE, msg);
    }

    void Debug::log(DebugLevel level, const std::string &msg)
    {
        // 检查是否启用调试
        if (!enabled_) {
            return;
        }

        // 检查调试级别
        if (level > level_) {
            return;
        }

        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &now_c);
#else
        localtime_r(&now_c, &tm);
#endif

        // 输出时间戳
        std::cerr << "[" << std::setfill('0')
                  << std::setw(4) << (tm.tm_year + 1900) << "-"
                  << std::setw(2) << (tm.tm_mon + 1) << "-"
                  << std::setw(2) << tm.tm_mday << " "
                  << std::setw(2) << tm.tm_hour << ":"
                  << std::setw(2) << tm.tm_min << ":"
                  << std::setw(2) << tm.tm_sec << "."
                  << std::setw(3) << now_ms.count() << "] ";

        // 输出调试级别
        switch (level) {
        case DebugLevel::ERROR:
            std::cerr << "[错误] ";
            break;
        case DebugLevel::WARNING:
            std::cerr << "[警告] ";
            break;
        case DebugLevel::INFO:
            std::cerr << "[信息] ";
            break;
        case DebugLevel::DEBUG:
            std::cerr << "[调试] ";
            break;
        case DebugLevel::TRACE:
            std::cerr << "[跟踪] ";
            break;
        default:
            break;
        }

        // 输出调试信息
        std::cerr << msg << std::endl;
    }

} // namespace dash 