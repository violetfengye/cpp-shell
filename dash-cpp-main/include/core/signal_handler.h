/**
 * @file signal_handler.h
 * @brief 信号处理
 */

#ifndef DASH_SIGNAL_HANDLER_H
#define DASH_SIGNAL_HANDLER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <csignal>
#include "../dash.h"

namespace dash {

/**
 * @brief 信号处理类
 */
class SignalHandler {
public:
    /**
     * @brief 信号处理器回调函数类型
     */
    using SignalCallback = std::function<void(int)>;

    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     */
    explicit SignalHandler(Shell& shell);

    /**
     * @brief 析构函数
     */
    ~SignalHandler();

    /**
     * @brief 初始化信号处理
     */
    void initialize();

    /**
     * @brief 设置信号处理器
     * 
     * @param signum 信号编号
     * @param handler 处理函数
     * @return bool 是否成功设置
     */
    bool setHandler(int signum, SignalCallback handler);

    /**
     * @brief 设置默认信号处理器
     * 
     * @param signum 信号编号
     * @return bool 是否成功设置
     */
    bool setDefaultHandler(int signum);

    /**
     * @brief 忽略信号
     * 
     * @param signum 信号编号
     * @return bool 是否成功设置
     */
    bool ignoreSignal(int signum);

    /**
     * @brief 阻塞信号
     * 
     * @param signum 信号编号
     * @return bool 是否成功阻塞
     */
    bool blockSignal(int signum);

    /**
     * @brief 解除阻塞信号
     * 
     * @param signum 信号编号
     * @return bool 是否成功解除阻塞
     */
    bool unblockSignal(int signum);

    /**
     * @brief 获取信号名称
     * 
     * @param signum 信号编号
     * @return std::string 信号名称
     */
    static std::string getSignalName(int signum);

    /**
     * @brief 获取信号编号
     * 
     * @param name 信号名称
     * @return int 信号编号，如果不存在则返回-1
     */
    static int getSignalNumber(const std::string& name);

private:
    Shell& shell_;  // Shell实例引用
    std::unordered_map<int, SignalCallback> handlers_;  // 信号处理器映射表

    // 静态方法用于处理信号
    static void signalHandler(int signum);
    
    // 信号处理器实例指针
    static SignalHandler* instance_;
};

} // namespace dash

#endif // DASH_SIGNAL_HANDLER_H 