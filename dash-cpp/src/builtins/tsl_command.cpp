/**
 * @file tsl_command.cpp
 * @brief tsl命令类实现
 */

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <getopt.h>
#include <vector>
#include "builtins/tsl_command.h"
#include "core/shell.h"
#include "utils/error.h"
#include "utils/transaction.h"
#include <termios.h>
#include <pwd.h>
#include <shadow.h>

namespace dash {
    TslCommand::TslCommand(Shell *shell)
    : BuiltinCommand(shell)
    {
    }

    int TslCommand::execute(const std::vector<std::string> &args){
        if (args.size() < 2) {
            std::cout << "tsl: 缺少参数" << std::endl;
            return 1;
        }else if(args.size() == 2){
            if (args[1] == "-a") {
                Transaction::outputTransactionInfo();
            } else if (args[1] == "-c") {
                std::cout << "tsl: 缺少事务名称" << std::endl;
                return 1;
            } else if (args[1] == "-d") {
                std::cout << "tsl: 缺少事务名称" << std::endl;
                return 1;
            } else if (args[1] == "-e") {
                Transaction::transactionComplete();
            } else{
                Transaction::transactionStart(args[1]);
            }
        }else if (args.size() == 3) {
            if (args[1] == "-a") {
                std::cout << "tsl: 无效的参数" << std::endl;
                return 1;
            }else if (args[1] == "-c") {
                Transaction::transactionRecord(args[2]);
            }else if (args[1] == "-d") {
                Transaction::transactionDelete(args[2]);
            }else if (args[1] == "-e") {
                Transaction::transactionComplete();
            }else if (args[1] == "-r") {
                Transaction::transactionStart(args[2]);
            }else{
                std::cout << "tsl: 无效的参数" << std::endl;
                return 1;
            }
        }
        return 0;
    }

    std::string TslCommand::getName() const
    {
        return "tsl";
    }

    std::string TslCommand::getHelp() const
    {
        return "tsl [-a|-c|-d|-e|-r] [transaction_name] - 管理事务。\n";
    }

}