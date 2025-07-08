/**
 * @file transaction.h
 * @brief 事务集合处理，Transaction Scripting Language
 */

#ifndef DASH_TRANSACTION_H
#define DASH_TRANSACTION_H

#include <string>
#include <vector>
#include <map>
#include <memory>


namespace dash
{
    // 输入类型
    enum class InputType
    {
        normal,         // 普通输入
        transaction,    // 事务输入
        record,         // 记录输入
    };
    /**
     * @brief Transaction 事务处理类
     */
    class Transaction
    {
    private:
        /**
         * @brief 输入类型
        */
        static InputType type_;

        /**
         * @brief 事务集合
         */
        static std::map<std::string,std::unique_ptr<Transaction>> transaction_map_;

        /**
         * @brief 事务名
         * */
        std::string transaction_name_;

        /**
         * @brief 事务命令集合
         * */
        std::vector<std::string> command_list_;

        /**
         * @brief 初始化一个事物
         * */
        static void init() ;

        /**
         * @brief 初始化事务集合
         * */
        static void initTransactionMap() ;

        /**
         * @brief 初始化事务完成标识
         * */
        static bool init_flag_;

        /**
         * @brief 当前处理的事务的名称
         * */
        static std::string current_transaction_name_;

        /**
         * @brief 当前处理的事务的命令集合
         * */
        static std::vector<std::string> current_command_list_;

        /**
         * @brief 当前事务命令索引
         * */
        static int current_command_index;

        /**
         * @brief 是否自动执行
         * */
        static bool auto_run_;

    public:
        /**
         * @brief 构造函数
         *
         * @note 默认构造函数
         */
        Transaction()
        {
            if(init_flag_ == false){
                initTransactionMap();
            }
        }

        /**
         * @brief 获取输入类型
         *
         * @return InputType 输入类型
         */
        static InputType getInputType() ;

        /**
         * @brief 修改输入类型
         *
         * @param type 输入类型
         * 
         * @return Int 是否成功
         */
        static int setInputType(InputType type) ;

        /**
         * @brief 返回命令字符串
         *
         * @return std::string 异常类型字符串
         */
        static std::string getCommandString() ;

        /**
         * @brief 添加命令字符串
         *
         * @param command 命令字符串
         * 
         * @return Int 是否成功
         */
        static int addCommandString(const std::string& command) ;

        /**
         * @brief 开始事务
         * 
         * @param transaction_name 事务名
         * */
        static void transactionStart(const std::string& transaction_name) ;

        /**
         * @brief 中断当前事务
         * 
         * */
        static void transactionInterrupt() ;
        
        /**
         * @brief 记录事务
         * 
         * @param transaction_name 事务名
         * */
        static void transactionRecord(const std::string& transaction_name) ;

        /**
         * @brief 事务完成
         * */
        static void transactionComplete() ;

        /**
         * @brief 删除一个事务
         * 
         * @param transaction_name 事务名
         * */
        static void transactionDelete(const std::string& transaction_name) ;

        /**
         * @brief 输出内存中所有事务信息
         */
        static void outputTransactionInfo() ;

        /**
         * @brief 处理运行中的事务
         * 
         * @var is_first 是否是第一次运行
         * 
         * @return Int 是否结束事务
         * */
        static int transactionRun(bool is_first = true) ;

        /**
         * @brief 设为自动执行
         * 
         * @param auto_run 是否自动执行
         * */
        static void setAutoRun(bool auto_run = false) ;

        /**
         * @brief 获取自动执行状态
         * 
         * @return bool 是否自动执行
         * 
         * */
        static bool getAutoRun() ;
    };
} // namespace dash

#endif // DASH_TRANSACTION_H