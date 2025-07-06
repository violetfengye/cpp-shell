/**
 * @file signal_handler.cpp
 * @brief 信号处理实现
 */

#include "../../include/core/signal_handler.h"
#include "../../include/core/shell.h"
#include <unistd.h>
#include <signal.h>
#include <map>
#include <cstring>

namespace dash {

// 静态成员初始化
SignalHandler* SignalHandler::instance_ = nullptr;

// 信号名称映射表
static const std::map<int, const char*> signalNames = {
    {SIGHUP, "SIGHUP"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGILL, "SIGILL"},
    {SIGTRAP, "SIGTRAP"},
    {SIGABRT, "SIGABRT"},
    {SIGFPE, "SIGFPE"},
    {SIGKILL, "SIGKILL"},
    {SIGSEGV, "SIGSEGV"},
    {SIGPIPE, "SIGPIPE"},
    {SIGALRM, "SIGALRM"},
    {SIGTERM, "SIGTERM"},
    {SIGUSR1, "SIGUSR1"},
    {SIGUSR2, "SIGUSR2"},
    {SIGCHLD, "SIGCHLD"},
    {SIGCONT, "SIGCONT"},
    {SIGSTOP, "SIGSTOP"},
    {SIGTSTP, "SIGTSTP"},
    {SIGTTIN, "SIGTTIN"},
    {SIGTTOU, "SIGTTOU"},
    {SIGBUS, "SIGBUS"},
    {SIGPOLL, "SIGPOLL"},
    {SIGPROF, "SIGPROF"},
    {SIGSYS, "SIGSYS"},
    {SIGTRAP, "SIGTRAP"},
    {SIGURG, "SIGURG"},
    {SIGVTALRM, "SIGVTALRM"},
    {SIGXCPU, "SIGXCPU"},
    {SIGXFSZ, "SIGXFSZ"}
};

SignalHandler::SignalHandler(Shell& shell) : shell_(shell) {
    // 保存实例指针，用于静态方法中访问
    instance_ = this;
}

SignalHandler::~SignalHandler() {
    // 恢复默认信号处理
    for (const auto& handler : handlers_) {
        signal(handler.first, SIG_DFL);
    }
    
    // 清除实例指针
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void SignalHandler::initialize() {
    // 设置默认的信号处理器
    setHandler(SIGINT, [this](int signum) {
        // 处理Ctrl+C
        // 在实际实现中，需要中断当前命令执行
    });
    
    setHandler(SIGCHLD, [this](int signum) {
        // 处理子进程状态变化
        // 在实际实现中，需要回收子进程
    });
    
    // 忽略一些信号
    ignoreSignal(SIGPIPE);
    ignoreSignal(SIGTTOU);
    ignoreSignal(SIGTTIN);
}

bool SignalHandler::setHandler(int signum, SignalCallback handler) {
    handlers_[signum] = handler;
    
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &SignalHandler::signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    return sigaction(signum, &sa, nullptr) == 0;
}

bool SignalHandler::setDefaultHandler(int signum) {
    handlers_.erase(signum);
    return signal(signum, SIG_DFL) != SIG_ERR;
}

bool SignalHandler::ignoreSignal(int signum) {
    handlers_.erase(signum);
    return signal(signum, SIG_IGN) != SIG_ERR;
}

bool SignalHandler::blockSignal(int signum) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signum);
    return sigprocmask(SIG_BLOCK, &set, nullptr) == 0;
}

bool SignalHandler::unblockSignal(int signum) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signum);
    return sigprocmask(SIG_UNBLOCK, &set, nullptr) == 0;
}

std::string SignalHandler::getSignalName(int signum) {
    auto it = signalNames.find(signum);
    if (it != signalNames.end()) {
        return it->second;
    }
    return "UNKNOWN_SIGNAL(" + std::to_string(signum) + ")";
}

int SignalHandler::getSignalNumber(const std::string& name) {
    for (const auto& pair : signalNames) {
        if (name == pair.second) {
            return pair.first;
        }
    }
    return -1;
}

void SignalHandler::signalHandler(int signum) {
    if (instance_) {
        auto it = instance_->handlers_.find(signum);
        if (it != instance_->handlers_.end()) {
            it->second(signum);
        }
    }
}

} // namespace dash 