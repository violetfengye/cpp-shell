

/**
 * @file transaction.cpp
 * @brief 事务集合处理
 */

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <curses.h>
#include <map>
#include <stdio.h>  
#include <termios.h>  
#include "../../include/utils/transaction.h"

#define filePath "./etc/dash/transaction/"


namespace dash
{
    // 读取一个字符
    
    char get1char(void)  
    {  
        struct termios stored_settings;  
        struct termios new_settings;  
        tcgetattr (0, &stored_settings);  
        new_settings = stored_settings;  
        new_settings.c_lflag &= (~ICANON);  
        new_settings.c_cc[VTIME] = 0;  
        new_settings.c_cc[VMIN] = 1;  
        tcsetattr (0, TCSANOW, &new_settings);  
    
        int ret = 0;  
        char c;  
    
        c = getchar();  
        putchar('\b'); // 删除回显  
    
        //printf("input:  [%c]\n", c);  
        tcsetattr (0, TCSANOW, &stored_settings); // 恢复终端参数  
    
        return c;   
    }  
    // 读取文件夹中的所有文件，并将每行存储到 vector<string> 中
    std::vector<std::string> readLinesFromFile(std::string fileName) {
        std::vector<std::string> lines;
        std::ifstream file(filePath + fileName);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
            file.close();
        } else {
            std::cerr << "无法打开文件: " << filePath + fileName << std::endl;
        }
        return lines;
    }
    // 将 vector<string> 写回文件，每行一个字符串
    void writeLinesToFile(std::vector<std::string> lines, std::string fileName) {
        //std::cout<<"写入文件"<<std::endl;
        // 如果文件存在，则先删除文件
        if (std::filesystem::exists(filePath + fileName)) {
            std::filesystem::remove(filePath + fileName);
        }
        std::ofstream file(filePath + fileName);
        if (file.is_open()) {
            for (const auto& line : lines) {
                file << line << std::endl;
            }
            file.close();
        } else {
            std::cerr << "无法打开文件: " << filePath + fileName << std::endl;
        }
    }

    InputType Transaction::type_ = InputType::normal; // 默认为普通输入
    std::map<std::string,std::unique_ptr<Transaction>> Transaction::transaction_map_;
    void Transaction::init() {
        std::unique_ptr<Transaction> transaction(new Transaction());
        transaction->transaction_name_ = Transaction::current_transaction_name_;
        transaction->command_list_ = Transaction::current_command_list_;
        //std::cout<<"初始化事物"<<std::endl;
        try
        {
            writeLinesToFile(transaction->current_command_list_,transaction->transaction_name_);
            transaction_map_[transaction->transaction_name_] = std::move(transaction);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    int Transaction::addCommandString(const std::string& command){
        Transaction::current_command_list_.push_back(command);
        return 0;
    }
    void Transaction::transactionStart(const std::string &transaction_name){
        //std::cout<<"事务开始"<<std::endl;
        if (transaction_map_.find(transaction_name) != transaction_map_.end()) {
            Transaction::current_transaction_name_ = transaction_name;
            Transaction::current_command_list_ = transaction_map_[transaction_name]->command_list_;
            Transaction::current_command_index = 0;
            setInputType(InputType::transaction); // 设置为事务开始
            std::cout<<"开始事务："<<transaction_name<<std::endl;
            setAutoRun(false); // 设置为单步运行
        }else{
            std::cerr<< transaction_name <<"事务不存在"<<std::endl;
        }

    }
    void Transaction::transactionRecord(const std::string &transaction_name){
        Transaction::current_transaction_name_ = transaction_name;
        Transaction::current_command_list_.clear();
        setInputType(InputType::record); // 设置为记录输入
        std::cout<<"开始记录事务："<< transaction_name <<std::endl;
    }

    void Transaction::transactionComplete() {
        //std::cout<<"事务名称: "<<Transaction::current_transaction_name_<<std::endl;
        if (Transaction::current_command_list_.size()<=1){
            std::cerr<<"事务命令列表为空"<<std::endl;
            return;
        }
        //std::cout<<"事务命令列表: "<<Transaction::current_command_list_[0]<<std::endl;
        Transaction::current_command_list_.pop_back();
        init(); // 初始化该事物
        Transaction::current_transaction_name_ = ""; // 清空当前事务名称
        Transaction::current_command_list_.clear(); // 清空当前事务命令列表
        setInputType(InputType::normal); // 恢复为普通输入
        std::cout<<"事务记录结束"<<std::endl;
    }

    void Transaction::transactionDelete(const std::string &transaction_name){
        if (transaction_map_.find(transaction_name) != transaction_map_.end()) {
            if(current_transaction_name_ == transaction_name){
                std::cerr<<"当前事务不能删除"<<std::endl;
                return;
            }
            transaction_map_.erase(transaction_name);
            std::filesystem::remove(filePath + transaction_name);
            std::cout<<"已删除事务："<< transaction_name <<std::endl;
        }else{
            std::cerr<< transaction_name <<"事务不存在"<<std::endl;
        }
    }

    namespace fs = std::filesystem;
    void Transaction::initTransactionMap() {
        if (!fs::exists(filePath)) {
            fs::create_directories(filePath);
            //std::cout << "创建路径: " << filePath << std::endl;
        } else {
            //std::cout << "路径已存在: " << filePath << std::endl;
        }
        init_flag_ = true;
        transaction_map_.clear();
        // 遍历文件夹中的所有文件
        for (const auto& entry : fs::directory_iterator(filePath)) {
            const auto& path = entry.path();
            if (fs::is_regular_file(path)) {
                std::ifstream file(path);
                std::unique_ptr<Transaction> transaction(new Transaction());
                transaction->transaction_name_ = path.stem().string(); // 获取文件名（不包含扩展名）
                //std::cout<<"transaction name: "<<transaction->transaction_name_<<std::endl;
                transaction->command_list_ = readLinesFromFile(path.filename().string());
                //std::cout<<"transaction command list: "<<transaction->command_list_[0]<<std::endl;
                transaction_map_[transaction->transaction_name_] = std::move(transaction);
            }
        }
    }

    bool Transaction::init_flag_ = false;

    std::string Transaction::current_transaction_name_; // 当前事务名称
    std::vector<std::string> Transaction::current_command_list_; // 当前事务命令列表
    int Transaction::current_command_index = 0; // 当前事务命令索引

    
    InputType Transaction::getInputType(){ return type_; }

    int Transaction::setInputType(InputType type){ type_ = type; return 1;}
    std::string Transaction::getCommandString() {
        if (current_command_index < current_command_list_.size()) {
            if (current_command_index == current_command_list_.size() - 1) {
                //事务结束
                writeLinesToFile(current_command_list_, current_transaction_name_);
                transaction_map_[current_transaction_name_]->command_list_ = current_command_list_;
                setInputType(InputType::normal);
                current_transaction_name_ = "";//必须重置
                std::cout<<"事务结束，命令输出："<<std::endl;
            }
            return current_command_list_[current_command_index++];
        } else {
            writeLinesToFile(current_command_list_, current_transaction_name_);
            transaction_map_[current_transaction_name_]->command_list_ = current_command_list_;
            setInputType(InputType::normal);
            current_transaction_name_ = "";//必须重置
            std::cout<<"事务结束"<<std::endl;
            return "";
        }
    };
    void Transaction::outputTransactionInfo() {
        std::cout << "编号" << "\t" << "事务名称" << "\t" << "命令数" << std::endl;
        int index = 0;
        for(auto &transaction : transaction_map_){
            std::cout<<index++<<"\t"<<transaction.first<<"\t"<<transaction.second->command_list_.size()<<std::endl;
        }
    }
    void Transaction::transactionInterrupt(){
        Transaction::current_transaction_name_ = "";
        Transaction::current_command_index = 0;
        setInputType(InputType::normal);
    }

    int Transaction::transactionRun(bool is_first){
        if(is_first)
            std::cout<<std::endl;
        std::cout<<"第 "<< current_command_index + 1<<" 条命令："<<Transaction::current_command_list_[current_command_index]<<std::endl;
        // 建立一个读取一个字符的shell输入
        // a add d delete m modify t to end q quit j jump b back
        if(auto_run_){
            return 0;
        }
        char c = get1char();
        std::string command;
        
        switch (c)
        {
        case 'a':
            printf("\033[%dA", 1);
            printf("\033[K");
            fflush(stdout);
            std::cout<<"请输入命令："<<std::endl;
            // 从标准输入读取一行命令
            std::getline(std::cin, command);
            current_command_list_.insert(current_command_list_.begin() + current_command_index, command);
            break;
        case 'b':
            if(current_command_index > 0){
                current_command_index--;
                // ANSI转义序列：光标上移指定行数
                printf("\033[%dA", 1);
                printf("\033[K");
                fflush(stdout);
                return transactionRun(false);
            }else{
                std::cerr<<"已经是第一条命令"<<std::endl;
                return transactionRun(false);
            }
            break;
        case 'd':
            if(current_command_index + 1 < Transaction::current_command_list_.size()){
                current_command_list_.erase(current_command_list_.begin() + current_command_index);
                printf("\033[%dA", 1);
                printf("\033[K");
                fflush(stdout);
                return transactionRun(false);
            }else{
                std::cout<<"这是最后一条命令"<<std::endl;
                //强制加一，跳过获取字符串
                current_command_index++;
                return 1;
            }
            
            break;
        case 'm':
            fflush(stdout);
            // 从标准输入读取一行命令
            std::getline(std::cin, command);
            // 修改当前命令
            current_command_list_.erase(current_command_list_.begin() + current_command_index);
            current_command_list_.insert(current_command_list_.begin() + current_command_index, command);
            break;
        case 'j':
            if(current_command_index + 1 < Transaction::current_command_list_.size()){
                current_command_index++;
                // ANSI转义序列：光标上移指定行数
                printf("\033[%dA", 1);
                printf("\033[K");
                fflush(stdout);
                return transactionRun(false);
            }else{
                std::cout<<"已经最后一条命令"<<std::endl;
                //强制加一，跳过获取字符串
                current_command_index++;
                return 1;
            }
            break;
        case 't':
            setAutoRun(true);
            break;
        case 'q':
            Transaction::transactionInterrupt();
            break;
        default:
            break;
        }
        return 0;
    }

    bool Transaction::auto_run_ = false;
    void Transaction::setAutoRun(bool is_auto_run){
        auto_run_ = is_auto_run;
    }
    bool Transaction::getAutoRun(){
        return auto_run_;
    }

} // namespace dash