/**
 * @file error.cpp
 * @brief 异常处理类实现
 */

#include "utils/error.h"
#include <iostream>

namespace dash
{
    void exitShell(int exit_code)
    {
        throw ShellException(ExceptionType::EXIT, "Shell exit requested with code " + std::to_string(exit_code));
    }

    void errorShell(const std::string &message, int exit_code)
    {
        std::cerr << "Error: " << message << std::endl;
        throw ShellException(ExceptionType::ERROR, message);
    }

    void warnShell(const std::string &message)
    {
        std::cerr << "Warning: " << message << std::endl;
    }
} // namespace dash 