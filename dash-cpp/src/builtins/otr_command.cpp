/**
 * @file otr_command.cpp
 * @brief Otr命令类实现
 */

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <getopt.h>
#include <vector>
#include "builtins/otr_command.h"
#include "core/shell.h"
#include "utils/error.h"

namespace dash
{

    OtrCommand::OtrCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int OtrCommand::execute(const std::vector<std::string> &args)
    {
        std::string cmd; //获取执行的命令
        if (args.size() <= 1){
            std::cerr << "otr: 缺少参数" << std::endl;
            return 1;
        }
        for (size_t i = 1; i < args.size(); ++i) {
            cmd += args[i] + " ";
        }
        Transaction::addSpecialCommand(cmd);
        Transaction::setInputType(InputType::special); //设置输入类型为特殊命令
        return 0;
    }

    std::string OtrCommand::getName() const
    {
        return "otr";
    }

    std::string OtrCommand::getHelp() const
    {
        return "otr [cmd] - 外部命令";
    }

} // namespace dash