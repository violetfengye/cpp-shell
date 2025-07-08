/**
 * @file su_command.cpp
 * @brief Su命令类实现，未完善，暂不更新，用作测试，Rikyon
 */

#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <getopt.h>
#include <vector>
#include "builtins/su_command.h"
#include "core/shell.h"
#include "variable/prompt_string.h"
#include "utils/error.h"
#include "utils/transaction.h"
#include <termios.h>
#include <pwd.h>
#include <shadow.h>

namespace dash
{
    std::string promptPassword(std::string);

    SuCommand::SuCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int SuCommand::execute(const std::vector<std::string> &args)
    {
        //std::cout<<promptPassword()<<std::endl;
        int mode = 0; // 用于辨识选项，方便后续命令处理
        std::string userName = "root";
        std::string commandText = "";
        static const char *optString = "cl";
        // 1 : -l
        // 2 : -c
        // 手动解析选项，避免使用 getopt
        if (args.size() == 1) {
            Transaction::setInputType(InputType::normal);
            mode = 1;
        }else if (args.size() == 2) {
            Transaction::transactionStart(args[1]);
            if (args[1] == "-c") {
                std::cerr << "su: 缺少参数: 用户名" << std::endl;
                std::cerr << "su: 用法: su [-c|-l] [username]" << std::endl;
                return 1;
            }
            mode = 1;
            if (args[1] == "-l" || args[1] == "-") {
                ;
            } else {
                userName = args[1];
            }
        }else if (args.size() == 3) {
            Transaction::setInputType(InputType::transaction);
            if (args[1] == "-c") {
                mode = 2;
                commandText = args[2];
            } else if (args[1] == "-l") {
                mode = 1;
                userName = args[2];
            } else {
                std::cerr << "su: 无效选项: -" << args[1][1] << std::endl;
            }
        }else if (args.size() == 4) {
            Transaction::transactionRecord( args[1] );
            if(args[1] == "-c") {
                mode = 2;
                userName = args[3];
                commandText = args[2];
            }else{
                std::cerr << "su: 无效选项: -" << args[1][1] << std::endl;
                std::cerr << "su: 用法: su [-c|-l] [username]" << std::endl;
                return 1;
            }
        }else {
            Transaction::transactionComplete();
            std::cerr << "su: 无效参数: " << args[1] << std::endl;
            std::cerr << "su: 用法: su [-c|-l] [username]" << std::endl;
            return 1;
        }
        if (mode == 1) {
        }else{
            Transaction::setInputType(InputType::transaction);
        }
        /*
        //该功能暂时未完善
        struct spwd *sp = getspnam("root");
        if(sp==NULL){
            std::cout<<"su weikong"<<std::endl;
        }else{
            if(sp->sp_pwdp==NULL){
                std::cout<<"su weikong1"<<std::endl;
            }else
            std::cout<<sp->sp_pwdp<<std::endl;

        }
        std::string password = promptPassword(userName);
        */
        //验证密码
        /*
        if (strcmp(getpassphrase(userName.c_str()), password.c_str()) != 0) {
            std::cerr << "su: " << userName << ": 密码错误" << std::endl;
            return 1;
        }
        if (mode == 1) {
            if (setuid(getpwnam(userName.c_str())->pw_uid) == -1) {
                std::cerr << "su: " << userName << ": 权限不够" << std::endl;
                return 1;
            }
        }else{
            if (setuid(getpwnam(userName.c_str())->pw_uid) == -1) {
                std::cerr << "su: " << userName << ": 权限不够" << std::endl;
                return 1;
            }
            if (execlp(commandText.c_str(), commandText.c_str(), NULL) == -1) {
                std::cerr << "su: " << commandText << ": 找不到命令" << std::endl;
                return 1;
            }
        }*/
        return 0;
    }

    std::string SuCommand::getName() const
    {
        return "su";
    }

    std::string SuCommand::getHelp() const
    {
        return "su [-c|-l] [username] - 以指定用户运行。\n"; 
    }

    std::string promptPassword(std::string userName) {
        std::string password;
        std::cout << "请输入用户 "+ userName +" 的密码: ";

        // 禁用Echo，隐藏密码输入
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        std::getline(std::cin, password);

        // 恢复终端设置
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << std::endl;

        return password;
    }

} // namespace dash