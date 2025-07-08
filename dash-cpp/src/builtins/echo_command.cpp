/**
 * @file echo_command.cpp
 * @brief Echo命令类实现
 */

#include <iostream>
#include <sstream>
#include "builtins/echo_command.h"
#include "core/shell.h"

namespace dash
{

    EchoCommand::EchoCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int EchoCommand::execute(const std::vector<std::string> &args)
    {
        bool interpret_escapes = false;
        bool no_newline = false;
        size_t i = 1;

        // 处理选项
        while (i < args.size() && args[i].size() > 0 && args[i][0] == '-')
        {
            if (args[i] == "-n")
            {
                no_newline = true;
            }
            else if (args[i] == "-e")
            {
                interpret_escapes = true;
            }
            else if (args[i] == "-E")
            {
                interpret_escapes = false;
            }
            else
            {
                // 不是有效选项，当作普通参数
                break;
            }
            i++;
        }

        // 输出参数
        bool first = true;
        for (; i < args.size(); i++)
        {
            if (!first)
            {
                std::cout << " ";
            }
            first = false;

            if (interpret_escapes)
            {
                std::cout << processEscapes(args[i]);
            }
            else
            {
                std::cout << args[i];
            }
        }

        if (!no_newline)
        {
            std::cout << std::endl;
        }
        else
        {
            std::cout.flush();
        }

        return 0;
    }

    std::string EchoCommand::getName() const
    {
        return "echo";
    }

    std::string EchoCommand::getHelp() const
    {
        return "echo [-neE] [arg ...] - 显示一行文本";
    }

    std::string EchoCommand::processEscapes(const std::string &str)
    {
        std::ostringstream result;
        bool in_escape = false;

        for (size_t i = 0; i < str.size(); i++)
        {
            if (in_escape)
            {
                in_escape = false;
                switch (str[i])
                {
                case 'a':
                    result << '\a';
                    break; // 警告（响铃）
                case 'b':
                    result << '\b';
                    break; // 退格
                case 'c':  // 不输出更多字符
                    return result.str();
                case 'e':
                    result << '\033';
                    break; // 转义字符
                case 'f':
                    result << '\f';
                    break; // 换页
                case 'n':
                    result << '\n';
                    break; // 换行
                case 'r':
                    result << '\r';
                    break; // 回车
                case 't':
                    result << '\t';
                    break; // 水平制表符
                case 'v':
                    result << '\v';
                    break; // 垂直制表符
                case '\\':
                    result << '\\';
                    break; // 反斜杠
                default:
                    result << '\\' << str[i];
                    break;
                }
            }
            else if (str[i] == '\\')
            {
                in_escape = true;
            }
            else
            {
                result << str[i];
            }
        }

        // 如果字符串以反斜杠结尾，则添加它
        if (in_escape)
        {
            result << '\\';
        }

        return result.str();
    }

} // namespace dash