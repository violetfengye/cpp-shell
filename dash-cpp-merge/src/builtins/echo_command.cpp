/**
 * @file echo_command.cpp
 * @brief Echo命令实现
 */

#include "builtins/echo_command.h"
#include "core/shell.h"
#include <iostream>
#include <string>
#include <vector>

namespace dash
{

    EchoCommand::EchoCommand(Shell *shell)
        : BuiltinCommand(shell, "echo")
    {
    }

    EchoCommand::~EchoCommand()
    {
    }

    int EchoCommand::execute(const std::vector<std::string> &args)
    {
        bool interpret_escapes = false;
        bool no_newline = false;
        size_t arg_index = 1;

        // 解析选项
        while (arg_index < args.size() && args[arg_index].size() > 0 && args[arg_index][0] == '-') {
            if (args[arg_index] == "-n") {
                no_newline = true;
            } else if (args[arg_index] == "-e") {
                interpret_escapes = true;
            } else if (args[arg_index] == "-E") {
                interpret_escapes = false;
            } else if (args[arg_index] == "--help") {
                std::cout << getHelp() << std::endl;
                return 0;
            } else {
                // 未知选项，停止解析
                break;
            }
            arg_index++;
        }

        // 输出参数
        for (size_t i = arg_index; i < args.size(); ++i) {
            if (i > arg_index) {
                std::cout << " ";
            }
            
            if (interpret_escapes) {
                std::cout << interpretEscapes(args[i]);
            } else {
                std::cout << args[i];
            }
        }

        // 输出换行符
        if (!no_newline) {
            std::cout << std::endl;
        }

        return 0;
    }

    std::string EchoCommand::interpretEscapes(const std::string &str)
    {
        std::string result;
        size_t i = 0;

        while (i < str.size()) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                switch (str[i + 1]) {
                case 'a': result += '\a'; i += 2; break;  // 警报（铃声）
                case 'b': result += '\b'; i += 2; break;  // 退格
                case 'c': return result;                  // 不输出换行符并终止输出
                case 'e': result += '\033'; i += 2; break;// 转义字符
                case 'f': result += '\f'; i += 2; break;  // 换页
                case 'n': result += '\n'; i += 2; break;  // 换行
                case 'r': result += '\r'; i += 2; break;  // 回车
                case 't': result += '\t'; i += 2; break;  // 水平制表符
                case 'v': result += '\v'; i += 2; break;  // 垂直制表符
                case '\\': result += '\\'; i += 2; break; // 反斜杠
                case '0':  // 八进制值
                    if (i + 2 < str.size() && str[i + 2] >= '0' && str[i + 2] <= '7') {
                        if (i + 3 < str.size() && str[i + 3] >= '0' && str[i + 3] <= '7') {
                            // 最多三位八进制数
                            int val = (str[i + 2] - '0') * 64 + (str[i + 3] - '0') * 8;
                            if (i + 4 < str.size() && str[i + 4] >= '0' && str[i + 4] <= '7') {
                                val += (str[i + 4] - '0');
                                i += 5;
                            } else {
                                i += 4;
                            }
                            result += static_cast<char>(val);
                        } else {
                            // 两位八进制数
                            int val = (str[i + 2] - '0') * 8;
                            i += 3;
                            result += static_cast<char>(val);
                        }
                    } else {
                        // 单个0
                        result += '\0';
                        i += 2;
                    }
                    break;
                case 'x':  // 十六进制值
                    if (i + 2 < str.size() && isxdigit(str[i + 2])) {
                        int val = 0;
                        i += 2;
                        if (isdigit(str[i])) {
                            val = str[i] - '0';
                        } else if (str[i] >= 'a' && str[i] <= 'f') {
                            val = str[i] - 'a' + 10;
                        } else if (str[i] >= 'A' && str[i] <= 'F') {
                            val = str[i] - 'A' + 10;
                        }
                        i++;
                        
                        if (i < str.size() && isxdigit(str[i])) {
                            val = val * 16;
                            if (isdigit(str[i])) {
                                val += str[i] - '0';
                            } else if (str[i] >= 'a' && str[i] <= 'f') {
                                val += str[i] - 'a' + 10;
                            } else if (str[i] >= 'A' && str[i] <= 'F') {
                                val += str[i] - 'A' + 10;
                            }
                            i++;
                        }
                        
                        result += static_cast<char>(val);
                    } else {
                        result += "\\x";
                        i += 2;
                    }
                    break;
                default:
                    result += '\\';
                    result += str[i + 1];
                    i += 2;
                    break;
                }
            } else {
                result += str[i++];
            }
        }

        return result;
    }

    std::string EchoCommand::getHelp() const
    {
        return "echo [选项]... [字符串]...\n"
               "  将字符串输出到标准输出。\n"
               "  选项:\n"
               "    -n         不输出尾随的换行符\n"
               "    -e         解释反斜杠转义序列\n"
               "    -E         不解释反斜杠转义序列（默认）\n"
               "    --help     显示此帮助信息\n"
               "  转义序列:\n"
               "    \\a         警报（铃声）\n"
               "    \\b         退格\n"
               "    \\c         不输出换行符并终止输出\n"
               "    \\e         转义字符\n"
               "    \\f         换页\n"
               "    \\n         换行\n"
               "    \\r         回车\n"
               "    \\t         水平制表符\n"
               "    \\v         垂直制表符\n"
               "    \\\\         反斜杠\n"
               "    \\0nnn      八进制值为nnn的字符\n"
               "    \\xHH       十六进制值为HH的字符";
    }

} // namespace dash 