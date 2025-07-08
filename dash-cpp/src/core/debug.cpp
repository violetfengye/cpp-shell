/**
 * @file debug.cpp
 * @brief 调试功能实现
 */

#include "debug.h"

namespace dash {
    // 初始化静态成员变量
    std::ofstream DebugLog::log_file;
    bool DebugLog::initialized = false;
} // namespace dash 