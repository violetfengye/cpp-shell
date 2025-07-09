/**
 * @file dash.h
 * @brief Dash-CPP 主头文件
 */

#ifndef DASH_H
#define DASH_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>

// 版本信息
#define DASH_VERSION "0.1.0"
#define DASH_VERSION_MAJOR 0
#define DASH_VERSION_MINOR 1
#define DASH_VERSION_PATCH 0

// 异常类型
enum class ExceptionType
{
    EXIT,      // 正常退出
    INTERRUPT, // 中断信号
    ERROR,     // 一般错误
    SYNTAX,    // 语法错误
    MEMORY,    // 内存错误
    IO         // 输入输出错误
};
// 输入类型
enum class InputType
{
    normal,         // 普通输入
    transaction,    // 事务输入
    recall,         // 命令历史
    special         // 特殊事务
};
// 节点类型
enum class NodeType
{
    COMMAND, // 简单命令
    PIPE,    // 管道
    LIST,    // 命令列表
    IF,      // if 语句
    FOR,     // for 循环
    WHILE,   // while/until 循环
    CASE,    // case 语句
    SUBSHELL // 子 shell
};

// 词法单元类型
enum class TokenType
{
    WORD,        // 单词
    OPERATOR,    // 操作符
    ASSIGNMENT,  // 赋值
    IO_NUMBER,   // IO 编号
    NEWLINE,     // 换行符
    END_OF_INPUT // 输入结束
};

// 前向声明
namespace dash
{
    class Shell;
    class Node;
    class Parser;
    class Executor;
    class InputHandler;
    class JobControl;
    class VariableManager;
    class BuiltinCommand;
    class ShellException;
    class Transaction;
    
    /**
     * @brief 创建 Shell 实例
     *
     * @param argc 参数数量
     * @param argv 参数数组
     * @return int 退出状态码
     */
    int createShell(int argc, char *argv[]);
}

/**
 * @brief 词法单元类
 */
class Token
{
private:
    TokenType type_;
    std::string value_;

public:
    /**
     * @brief 构造函数
     *
     * @param type 词法单元类型
     * @param value 词法单元值
     */
    Token(TokenType type, const std::string &value)
        : type_(type), value_(value)
    {
    }

    /**
     * @brief 获取词法单元类型
     *
     * @return TokenType 词法单元类型
     */
    TokenType getType() const { return type_; }

    /**
     * @brief 获取词法单元值
     *
     * @return const std::string& 词法单元值
     */
    const std::string &getValue() const { return value_; }
};

#endif // DASH_H