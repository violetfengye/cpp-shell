/**
 * @file main.cpp
 * @brief 程序入口
 */

#include <iostream>
#include <csignal>
#include "dash.h"
#include "core/debug.h"

// 如果启用了readline库
#ifdef READLINE_ENABLED
#include <readline/readline.h>
#include <readline/history.h>
#endif

/**
 * @brief 程序入口
 *
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 退出状态码
 */
int main(int argc, char *argv[])
{
    try
    {
        // 初始化调试日志
        dash::DebugLog::init();
        dash::DebugLog::log("Dash shell starting...");
        
#ifdef READLINE_ENABLED
        dash::DebugLog::log("Readline is enabled");
        // 确保readline正确处理信号
        rl_catch_signals = 0;
        rl_catch_sigwinch = 1;
        rl_set_signals();
#else
        dash::DebugLog::log("Readline is NOT enabled");
#endif

        int result = dash::createShell(argc, argv);
        
        // 关闭调试日志
        dash::DebugLog::log("Dash shell exiting with code: " + std::to_string(result));
        dash::DebugLog::close();
        
        return result;
    }
    catch (const std::exception &e)
    {
        dash::DebugLog::log("Fatal error: " + std::string(e.what()));
        dash::DebugLog::close();
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}