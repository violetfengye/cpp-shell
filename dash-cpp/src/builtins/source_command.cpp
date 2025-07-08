/**
 * @file source_command.cpp
 * @brief Source命令类实现
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "builtins/source_command.h"
#include "core/shell.h"
#include "core/parser.h"
#include "core/lexer.h"
#include "core/executor.h"
#include "utils/error.h"

namespace dash
{

    SourceCommand::SourceCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int SourceCommand::execute(const std::vector<std::string> &args)
    {
        // 检查参数
        if (args.size() < 2)
        {
            std::cerr << "source: 用法: source 文件名" << std::endl;
            return 1;
        }

        // 获取脚本文件路径
        std::string script_path = args[1];
        
        // 打开脚本文件
        std::ifstream file(script_path);
        if (!file)
        {
            std::cerr << "source: " << script_path << ": 没有那个文件或目录" << std::endl;
            return 1;
        }

        // 创建解析器
        Parser parser(shell_);
        
        int status = 0;
        std::string line;
        int line_number = 0;
        
        try
        {
            // 逐行读取和执行脚本
            while (std::getline(file, line))
            {
                line_number++;
                
                // 跳过空行和注释行
                if (line.empty() || line[0] == '#')
                {
                    continue;
                }
                
                // 去除行尾的回车符
                if (!line.empty() && line[line.length() - 1] == '\r')
                {
                    line.erase(line.length() - 1);
                }
                
                try
                {
                    // 解析当前行
                    std::unique_ptr<Node> node = parser.parse(line);
                    
                    if (node)
                    {
                        // 执行解析后的命令树
                        status = shell_->getExecutor()->execute(node.get());
                    }
                }
                catch (const ShellException &e)
                {
                    std::cerr << "source: " << script_path << ":" << line_number 
                              << ": " << e.what() << std::endl;
                    // 继续执行下一行，而不是立即退出
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "source: " << script_path << ": " << e.what() << std::endl;
            file.close();
            return 1;
        }
        
        file.close();
        return status;
    }

    std::string SourceCommand::getName() const
    {
        return "source";
    }

    std::string SourceCommand::getHelp() const
    {
        return "source 文件名 - 在当前shell中执行指定的脚本文件";
    }

} // namespace dash 