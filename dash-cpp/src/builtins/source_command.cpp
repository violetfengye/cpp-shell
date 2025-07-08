/**
 * @file source_command.cpp
 * @brief Source命令类实现
 */

#include <iostream>
#include <fstream>
#include <string>
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

        // 读取文件内容
        std::string script_content;
        std::string line;
        while (std::getline(file, line))
        {
            script_content += line + "\n";
        }
        
        // 关闭文件
        file.close();

        // 创建词法分析器和解析器
        Lexer lexer;
        Parser parser(shell_);
        
        int status = 0;
        
        try
        {
            // 对脚本内容进行词法分析
            lexer.setInput(script_content);
            
            // 解析并执行脚本中的每个命令
            while (!lexer.isEof())
            {
                // 解析一个命令
                std::unique_ptr<Node> node = parser.parse(lexer);
                
                if (node)
                {
                    // 执行命令
                    status = shell_->getExecutor()->execute(node.get());
                }
            }
        }
        catch (const ShellException &e)
        {
            std::cerr << "source: " << script_path << ": " << e.what() << std::endl;
            return 1;
        }
        
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