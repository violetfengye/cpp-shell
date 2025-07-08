/**
 * @file history_command.cpp
 * @brief History命令实现
 */

#include "builtins/history_command.h"
#include "core/shell.h"
#include "utils/history.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdlib>

namespace dash
{

    HistoryCommand::HistoryCommand(Shell *shell)
        : BuiltinCommand(shell, "history")
    {
    }

    HistoryCommand::~HistoryCommand()
    {
    }

    int HistoryCommand::execute(const std::vector<std::string> &args)
    {
        // 获取历史记录管理器
        History *history = shell_->getHistory();
        if (!history) {
            std::cerr << "history: 无法获取历史记录管理器" << std::endl;
            return 1;
        }

        // 解析选项和参数
        bool clear = false;
        int count = -1;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "-c" || args[i] == "--clear") {
                clear = true;
            } else if (args[i] == "-h" || args[i] == "--help") {
                std::cout << getHelp() << std::endl;
                return 0;
            } else if (args[i][0] != '-') {
                // 尝试解析为数字
                try {
                    count = std::stoi(args[i]);
                    if (count < 0) {
                        std::cerr << "history: 无效的数量: " << args[i] << std::endl;
                        return 1;
                    }
                } catch (const std::exception &) {
                    std::cerr << "history: 无效的参数: " << args[i] << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "history: 未知选项: " << args[i] << std::endl;
                std::cerr << "尝试 'history --help' 获取更多信息。" << std::endl;
                return 1;
            }
        }

        // 处理清除历史记录
        if (clear) {
            history->clear();
            return 0;
        }

        // 获取历史记录
        std::vector<std::string> history_entries = history->getEntries();
        
        // 如果指定了数量，只显示最后count条记录
        if (count > 0 && count < static_cast<int>(history_entries.size())) {
            history_entries.erase(history_entries.begin(), history_entries.end() - count);
        }

        // 显示历史记录
        int start_index = history->getStartIndex();
        for (size_t i = 0; i < history_entries.size(); ++i) {
            std::cout << std::setw(5) << (start_index + i) << "  " << history_entries[i] << std::endl;
        }

        return 0;
    }

    std::string HistoryCommand::getHelp() const
    {
        return "history [选项] [数量]\n"
               "  显示命令历史记录。\n"
               "  选项:\n"
               "    -c, --clear   清除历史记录\n"
               "    -h, --help    显示此帮助信息\n"
               "  数量:\n"
               "    指定要显示的最近命令的数量。";
    }

} // namespace dash 