/**
 * @file source_command.cpp
 * @brief Source命令实现
 */

#include "builtins/source_command.h"
#include "core/shell.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <memory>
#include <sys/wait.h>

namespace dash
{

    SourceCommand::SourceCommand(Shell *shell)
        : BuiltinCommand(shell, "source")
    {
    }

    SourceCommand::~SourceCommand()
    {
    }

    int SourceCommand::execute(const std::vector<std::string> &args)
    {
        if (args.size() < 2) {
            std::cerr << "source: 缺少文件名参数" << std::endl;
            std::cerr << "尝试 'source --help' 获取更多信息。" << std::endl;
            return 1;
        }

        if (args[1] == "--help") {
            std::cout << getHelp() << std::endl;
            return 0;
        }

        // 获取文件名
        std::string filename = args[1];

        // 检查文件是否存在
        if (access(filename.c_str(), R_OK) != 0) {
            std::cerr << "source: " << filename << ": " << strerror(errno) << std::endl;
            return 1;
        }

        // 打开文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "source: 无法打开文件: " << filename << ": " << strerror(errno) << std::endl;
            return 1;
        }

        // 读取文件内容
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(file, line)) {
            // 忽略空行和注释行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 移除行尾的注释
            size_t comment_pos = line.find('#');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            
            // 移除行首和行尾的空白字符
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            // 忽略空行
            if (line.empty()) {
                continue;
            }
            
            lines.push_back(line);
        }
        
        // 关闭文件
        file.close();

        // 由于我们无法直接访问 Shell::executeCommand，我们将使用一种替代方法：
        // 创建一个临时脚本文件，然后让 Shell 执行这个文件
        
        // 创建临时文件
        char temp_filename[] = "/tmp/source_cmd_XXXXXX";
        int temp_fd = mkstemp(temp_filename);
        if (temp_fd == -1) {
            std::cerr << "source: 无法创建临时文件: " << strerror(errno) << std::endl;
            return 1;
        }
        
        // 将命令写入临时文件
        FILE* temp_file = fdopen(temp_fd, "w");
        if (!temp_file) {
            close(temp_fd);
            unlink(temp_filename);
            std::cerr << "source: 无法打开临时文件: " << strerror(errno) << std::endl;
            return 1;
        }
        
        for (const auto& cmd : lines) {
            fprintf(temp_file, "%s\n", cmd.c_str());
        }
        
        fclose(temp_file);
        
        // 构建执行临时脚本的命令
        std::vector<std::string> exec_args = {"sh", temp_filename};
        
        // 创建一个新的 Shell 进程来执行临时脚本
        pid_t pid = fork();
        if (pid == -1) {
            unlink(temp_filename);
            std::cerr << "source: 无法创建子进程: " << strerror(errno) << std::endl;
            return 1;
        } else if (pid == 0) {
            // 子进程
            execl("/bin/sh", "sh", temp_filename, NULL);
            // 如果 execl 返回，说明出错了
            std::cerr << "source: 无法执行脚本: " << strerror(errno) << std::endl;
            exit(1);
        }
        
        // 父进程等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        
        // 删除临时文件
        unlink(temp_filename);
        
        // 返回子进程的退出状态
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return 1;
        }
    }

    std::string SourceCommand::getHelp() const
    {
        return "source 文件名 [参数...]\n"
               "  从指定文件读取并执行命令。\n"
               "  文件中的每一行都将被作为命令执行。\n"
               "  忽略空行和以#开头的注释行。";
    }

} // namespace dash 