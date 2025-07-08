#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <iomanip>
#include "include/core/input.h"
#include "include/core/shell.h"
#include "include/utils/error.h"
#include "src/core/debug.h"

/**
 * @brief 测试dash-cpp shell的补全功能
 * 
 * 这个程序直接测试shell的补全功能，无需通过交互式shell
 */
int main() {
    std::cout << "====== Dash-CPP Tab补全功能测试 ======\n" << std::endl;
    
    // 初始化调试日志
    dash::DebugLog::init();
    
    // 创建Shell实例
    dash::Shell shell;
    
    // 创建InputHandler实例
    dash::InputHandler input_handler(&shell);
    
    // 测试用例结构
    struct TestCase {
        std::string input;     // 输入文本
        int cursor_pos;        // 光标位置
        std::string expected;  // 期望的补全结果描述
    };
    
    // 定义测试用例
    std::vector<TestCase> test_cases = {
        {"ec", 2, "应该补全为'echo'"},
        {"cd ", 3, "应该列出当前目录"},
        {"cd /", 4, "应该列出根目录"},
        {"cat ", 4, "应该列出当前目录文件"},
        {"cat /etc/ho", 10, "应该补全为'/etc/hosts'或类似文件"},
        {"echo hello | g", 13, "应该补全为'grep'"},
        {"ls -l | grep | w", 15, "应该补全为'wc'或类似命令"}
    };
    
    // 执行测试
    std::cout << "正在执行补全测试...\n" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto& test = test_cases[i];
        
        std::cout << "测试 #" << (i + 1) << ": 输入 '" << test.input 
                  << "', 光标位置 " << test.cursor_pos << std::endl;
        std::cout << "期望: " << test.expected << std::endl;
        
        // 调用tabCompletion函数
        std::vector<std::string> completions;
        try {
            // 使用public方法调用tabCompletion
            dash::DebugLog::logCompletion("调用testTabCompletion: text='" + test.input + "', pos=" + std::to_string(test.cursor_pos));
            completions = input_handler.testTabCompletion(test.input, test.cursor_pos, test.cursor_pos);
            
            // 输出结果
            std::cout << "实际结果: 找到 " << completions.size() << " 个匹配项" << std::endl;
            dash::DebugLog::logCompletion("找到 " + std::to_string(completions.size()) + " 个匹配项");
            
            for (size_t j = 0; j < completions.size() && j < 5; ++j) {
                std::cout << "  - " << completions[j] << std::endl;
                dash::DebugLog::logCompletion("  - " + completions[j]);
            }
            if (completions.size() > 5) {
                std::cout << "  - ... (还有 " << (completions.size() - 5) << " 个结果)" << std::endl;
            }
            
            // 判断测试是否通过
            bool test_passed = !completions.empty();
            if (test_passed) {
                std::cout << "状态: " << "\033[32m通过\033[0m" << std::endl;
                passed++;
            } else {
                std::cout << "状态: " << "\033[31m失败\033[0m (没有找到匹配项)" << std::endl;
                failed++;
            }
        } catch (const std::exception& e) {
            std::cerr << "错误: " << e.what() << std::endl;
            dash::DebugLog::logCompletion("异常: " + std::string(e.what()));
            std::cout << "状态: " << "\033[31m失败\033[0m (发生异常)" << std::endl;
            failed++;
        } catch (...) {
            std::cerr << "错误: 未知异常" << std::endl;
            dash::DebugLog::logCompletion("捕获到未知异常");
            std::cout << "状态: " << "\033[31m失败\033[0m (发生未知异常)" << std::endl;
            failed++;
        }
        
        std::cout << std::endl;
    }
    
    // 输出测试结果汇总
    std::cout << "\n====== 测试结果汇总 ======" << std::endl;
    std::cout << "总测试: " << test_cases.size() << std::endl;
    std::cout << "通过: " << passed << std::endl;
    std::cout << "失败: " << failed << std::endl;
    std::cout << "通过率: " << std::fixed << std::setprecision(2) 
              << (passed * 100.0 / test_cases.size()) << "%" << std::endl;
    
    std::cout << "\n====== 测试完成 ======" << std::endl;
    
    // 关闭调试日志
    dash::DebugLog::logCompletion("测试程序正常结束");
    dash::DebugLog::close();
    
    return (failed == 0) ? 0 : 1;
} 