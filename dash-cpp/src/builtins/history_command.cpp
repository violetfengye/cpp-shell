/**
 * @file history_command.cpp
 * @brief History命令类实现
 */

#include <iostream>
#include <cstdlib> // 用于atoi
#include <string>
#include <vector>
#include <fstream>
#include "builtins/history_command.h"
#include "core/shell.h"
#include "utils/history.h"

namespace dash
{

    HistoryCommand::HistoryCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int HistoryCommand::execute(const std::vector<std::string> &args)
    {
        // 如果没有参数，显示所有历史记录
        if (args.size() == 1) {
            displayHistory();
            return 0;
        }

        // 处理命令选项
        if (args[1] == "-c" || args[1] == "--clear") {
            // 清除历史记录
            clearHistory();
            return 0;
        }
        else if (args[1] == "-h" || args[1] == "--help") {
            // 显示帮助信息
            displayHelp();
            return 0;
        }
        else if (args[1] == "-s" || args[1] == "--save") {
            // 保存历史记录到文件
            if (args.size() < 3) {
                std::cerr << "history: 缺少文件名参数" << std::endl;
                return 1;
            }
            saveHistory(args[2]);
            return 0;
        }
        else if (args[1] == "-l" || args[1] == "--load") {
            // 从文件加载历史记录
            if (args.size() < 3) {
                std::cerr << "history: 缺少文件名参数" << std::endl;
                return 1;
            }
            loadHistory(args[2]);
            return 0;
        }
        else if (args[1][0] == '-') {
            // 无效选项
            std::cerr << "history: 无效选项: " << args[1] << std::endl;
            displayHelp();
            return 1;
        }
        else {
            // 尝试将参数解析为数字
            try {
                size_t count = std::stoul(args[1]);
                displayHistory(count);
                return 0;
            }
            catch (const std::exception &) {
                std::cerr << "history: 无效参数: " << args[1] << std::endl;
                displayHelp();
                return 1;
            }
        }
    }

    std::string HistoryCommand::getName() const
    {
        return "history";
    }

    std::string HistoryCommand::getHelp() const
    {
        return "history [n] [-c] [-h] [-s filename] [-l filename] - 显示或管理命令历史记录";
    }

    void HistoryCommand::displayHelp()
    {
        std::cout << "用法: history [选项] [参数]" << std::endl;
        std::cout << "选项:" << std::endl;
        std::cout << "  [n]             显示最近n条历史记录" << std::endl;
        std::cout << "  -c, --clear     清除历史记录" << std::endl;
        std::cout << "  -h, --help      显示此帮助信息" << std::endl;
        std::cout << "  -s, --save      将历史记录保存到指定文件" << std::endl;
        std::cout << "  -l, --load      从指定文件加载历史记录" << std::endl;
    }

    void HistoryCommand::displayHistory(size_t count)
    {
        History* history = shell_->getHistory();
        if (!history) {
            std::cerr << "历史记录功能不可用" << std::endl;
            return;
        }

        const std::vector<HistoryEntry>& entries = history->getAllCommands();
        
        // 计算要显示的记录数量
        size_t start = 0;
        if (count > 0 && count < entries.size()) {
            start = entries.size() - count;
        }
        
        // 显示历史记录
        for (size_t i = start; i < entries.size(); ++i) {
            const auto& entry = entries[i];
            std::cout << entry.index << "  " << entry.command << std::endl;
        }
    }

    void HistoryCommand::clearHistory()
    {
        History* history = shell_->getHistory();
        if (history) {
            history->clear();
            std::cout << "历史记录已清除" << std::endl;
        }
    }

    void HistoryCommand::saveHistory(const std::string &filename)
    {
        History* history = shell_->getHistory();
        if (!history) {
            std::cerr << "历史记录功能不可用" << std::endl;
            return;
        }
        
        if (history->saveToFile(filename)) {
            std::cout << "历史记录已保存到 " << filename << std::endl;
        } else {
            std::cerr << "无法保存历史记录到 " << filename << std::endl;
        }
    }

    void HistoryCommand::loadHistory(const std::string &filename)
    {
        History* history = shell_->getHistory();
        if (!history) {
            std::cerr << "历史记录功能不可用" << std::endl;
            return;
        }
        
        if (history->loadFromFile(filename)) {
            std::cout << "已从 " << filename << " 加载历史记录" << std::endl;
        } else {
            std::cerr << "无法从 " << filename << " 加载历史记录" << std::endl;
        }
    }

} // namespace dash 