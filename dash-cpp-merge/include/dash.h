/**
 * @file dash.h
 * @brief Dash Shell 主头文件
 */

#ifndef DASH_H
#define DASH_H

#include <string>
#include <vector>

namespace dash
{
    /**
     * @brief Dash Shell 版本号
     */
    const std::string VERSION = "1.0.0";

    /**
     * @brief 帮助文本
     */
    const std::string HELP_TEXT = R"(
Dash Shell 使用说明:
    dash [选项]

选项:
    -c <命令>       执行指定命令
    -h, --help      显示帮助信息
    -v, --version   显示版本信息

内置命令:
    bg [作业ID]     在后台继续运行已停止的作业
    cd [目录]       改变当前目录
    debug [on|off]  启用或禁用调试信息
    echo [参数...]  显示参数
    exit [状态码]   退出shell
    fg [作业ID]     将作业移到前台并继续运行
    help            显示帮助信息
    history         显示命令历史
    jobs [-p]       显示活动作业
    kill [参数...]  发送信号给进程
    pwd             显示当前工作目录
    source 文件     执行脚本文件中的命令
    wait [作业ID]   等待后台作业完成
    )";

    /**
     * @brief 初始化shell
     * 
     * @param argc 参数数量
     * @param argv 参数数组
     * @return int 退出状态码
     */
    int initialize(int argc, char **argv);

    /**
     * @brief 运行shell
     * 
     * @return int 退出状态码
     */
    int run();

    /**
     * @brief 执行命令
     * 
     * @param command 命令字符串
     * @return int 命令的退出状态码
     */
    int executeCommand(const std::string &command);

    /**
     * @brief 执行脚本文件
     * 
     * @param filename 脚本文件路径
     * @return int 脚本的退出状态码
     */
    int executeScript(const std::string &filename);

    /**
     * @brief 清理资源
     */
    void cleanup();
    
} // namespace dash

#endif // DASH_H 