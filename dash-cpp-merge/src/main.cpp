/**
 * @file main.cpp
 * @brief Dash-CPP合并版Shell入口点
 */

#include "core/shell.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

/**
 * 显示帮助信息
 */
void show_help()
{
    std::cout << "用法: dash-cpp-merge [选项] [脚本文件] [参数...]" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -c <命令>   执行<命令>后退出" << std::endl;
    std::cout << "  -d, --debug 启用调试模式" << std::endl;
    std::cout << "  -h, --help  显示此帮助信息并退出" << std::endl;
}

/**
 * 检查命令行参数是否包含特定选项
 */
bool has_option(int argc, char** argv, const char* short_opt, const char* long_opt = nullptr)
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], short_opt) == 0 || (long_opt && strcmp(argv[i], long_opt) == 0)) {
            return true;
        }
    }
    return false;
}

/**
 * 程序入口点
 */
int main(int argc, char** argv)
{
    // 检查是否需要显示帮助信息
    if (has_option(argc, argv, "-h", "--help")) {
        show_help();
        return 0;
    }

    // 创建Shell实例
    dash::Shell shell;
    
    // 初始化Shell
    if (!shell.initialize(argc, argv)) {
        std::cerr << "初始化Shell失败" << std::endl;
        return 1;
    }
    
    // 运行Shell
    shell.run();
    
    // 返回Shell的退出码
    return shell.getExitCode();
} 