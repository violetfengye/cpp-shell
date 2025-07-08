/**
 * @file sprf_command.cpp
 * @brief Sprf命令类实现
 */

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <getopt.h>
#include <vector>
#include "builtins/sprf_command.h"
#include "core/shell.h"
#include "variable/prompt_string.h"
#include "utils/error.h"

namespace dash
{

    SprfCommand::SprfCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int SprfCommand::execute(const std::vector<std::string> &args)
    {
        int mode = 0; // 统一结算

        // 手动解析选项，避免使用 getopt
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string &arg = args[i];
            if (arg[0] == '-') {
                for (size_t j = 1; j < arg.size(); ++j) {
                    switch (arg[j]) {
                    case 's':
                        mode |= prompt_string::formatShort;
                        break;
                    case 'l':
                        mode |= prompt_string::formatLong;
                        break;
                    case 'n':
                        mode |= prompt_string::noCme;
                        break;
                    case 'c':
                        mode |= prompt_string::color;
                        break;
                    default:
                        std::cerr << "sprf: 无效选项: -" << arg[j] << std::endl;
                        std::cerr << "sprf: 用法: sprf [-n|-c] [-l|-s]" << std::endl;
                        return 1;
                    }
                }
            } else {
                    std::cerr << "sprf: 无效参数: " << arg << std::endl;
                    std::cerr << "sprf: 用法: sprf [-n|-c] [-l|-s]" << std::endl;
                return 1;
            }
        }

        // 设置提示符模式
        prompt_string::setPromptMode(mode);
        return 0;
    }

    std::string SprfCommand::getName() const
    {
        return "sprf";
    }

    std::string SprfCommand::getHelp() const
    {
        return "sprf [-n|-c] [-l|-s] - 设置命令提示符的格式。\n"; //[-(n|c)[s|l]|-(s|l)[n|c]]
    }

} // namespace dash