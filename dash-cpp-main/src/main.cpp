/**
 * @file main.cpp
 * @brief 程序入口
 */

#include <iostream>
#include "dash.h"

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
        return dash::createShell(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}