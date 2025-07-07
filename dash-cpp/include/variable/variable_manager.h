/**
 * @file variable_manager.h
 * @brief 变量管理器类定义
 */

#ifndef DASH_VARIABLE_MANAGER_H
#define DASH_VARIABLE_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 变量类
     */
    class Variable
    {
    public:
        /**
         * @brief 变量标志
         */
        enum Flags
        {
            VAR_NONE = 0,
            VAR_EXPORT = 1,         // 导出到环境变量
            VAR_READONLY = 2,       // 只读变量
            VAR_SPECIAL = 4,        // 特殊变量（如 $?, $#, $0 等）
            VAR_UPDATE_ON_READ = 8  // 在读取时更新值
        };

    private:
        std::string name_;
        std::string value_;
        int flags_;
        // 函数指针，用于更新变量值
        std::string (*updateValueFunc_)();

    public:
        /**
         * @brief 构造函数
         *
         * @param name 变量名
         * @param value 变量值
         * @param flags 变量标志
         */
        Variable(const std::string &name, const std::string &value, int flags = VAR_NONE);

        /**
         * @brief 获取变量名
         *
         * @return const std::string& 变量名
         */
        const std::string &getName() const { return name_; }

        /**
         * @brief 获取变量值
         *
         * @return const std::string& 变量值
         */
        const std::string &getValue() { 
            //修改：如果变量是在读时需要更新的，则更新值
            if (hasFlag(VAR_UPDATE_ON_READ)) {
                value_ = updateValueFunc_();
            }
            return value_; 
        }

        /**
         * @brief 设置更新值函数
         * 
         */
        void setUpdateValueFunc(std::string (*func)()) {updateValueFunc_ = func; }
        /**
         * @brief 设置变量值
         *
         * @param value 变量值
         * @return true 设置成功
         * @return false 设置失败（如变量是只读的）
         */
        bool setValue(const std::string &value);

        /**
         * @brief 获取变量标志
         *
         * @return int 变量标志
         */
        int getFlags() const { return flags_; }

        /**
         * @brief 设置变量标志
         *
         * @param flags 变量标志
         */
        void setFlags(int flags) { flags_ = flags; }

        /**
         * @brief 检查变量是否有指定标志
         *
         * @param flag 要检查的标志
         * @return true 有指定标志
         * @return false 没有指定标志
         */
        bool hasFlag(int flag) const { return (flags_ & flag) != 0; }

        /**
         * @brief 添加变量标志
         *
         * @param flag 要添加的标志
         */
        void addFlag(int flag) { flags_ |= flag; }
    };

    /**
     * @brief 变量管理器类
     *
     * 负责管理 shell 变量和环境变量。
     */
    class VariableManager
    {
    private:
        Shell *shell_;
        std::unordered_map<std::string, std::unique_ptr<Variable>> variables_;

        /**
         * @brief 执行命令替换并返回输出
         * 
         * @param cmd 要执行的命令
         * @return std::string 命令的输出
         */
        std::string executeCommandSubstitution(const std::string &cmd) const;

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit VariableManager(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~VariableManager();

        /**
         * @brief 初始化变量
         *
         * 从环境中导入变量，设置特殊变量等。
         */
        void initialize();

        /**
         * @brief 设置变量
         *
         * @param name 变量名
         * @param value 变量值
         * @param flags 变量标志
         * @return true 设置成功
         * @return false 设置失败（如变量是只读的）
         */
        bool set(const std::string &name, const std::string &value, int flags = Variable::VAR_NONE);

        /**
         * @brief 设置更新值函数
         * 
         * @param name 变量名
         * @param func 函数指针
         */
        void setUpdateValueFunc(const std::string &name, std::string (*func)());

        /**
         * @brief 获取变量值
         *
         * @param name 变量名
         * @return std::string 变量值，如果变量不存在则返回空字符串
         */
        std::string get(const std::string &name) const;

        /**
         * @brief 检查变量是否存在
         *
         * @param name 变量名
         * @return true 变量存在
         * @return false 变量不存在
         */
        bool exists(const std::string &name) const;

        /**
         * @brief 删除变量
         *
         * @param name 变量名
         * @return true 删除成功
         * @return false 删除失败（如变量是只读的或不存在）
         */
        bool unset(const std::string &name);

        /**
         * @brief 导出变量到环境
         *
         * @param name 变量名
         * @return true 导出成功
         * @return false 导出失败（如变量不存在）
         */
        bool exportVar(const std::string &name);

        /**
         * @brief 设置变量为只读
         *
         * @param name 变量名
         * @return true 设置成功
         * @return false 设置失败（如变量不存在）
         */
        bool setReadOnly(const std::string &name);

        /**
         * @brief 获取所有变量名
         *
         * @return std::vector<std::string> 变量名列表
         */
        std::vector<std::string> getAllNames() const;

        /**
         * @brief 获取所有导出变量
         *
         * @return std::vector<std::pair<std::string, std::string>> 变量名和值的列表
         */
        std::vector<std::pair<std::string, std::string>> getExportVars() const;

        /**
         * @brief 获取环境变量数组
         *
         * 用于 execve 系统调用。
         *
         * @return std::vector<std::string> 环境变量数组
         */
        std::vector<std::string> getEnvironment() const;

        /**
         * @brief 展开字符串中的变量
         *
         * @param str 包含变量的字符串
         * @return std::string 展开后的字符串
         */
        std::string expand(const std::string &str) const;

        /**
         * @brief 更新特殊变量
         *
         * 更新如 $?, $#, $0 等特殊变量。
         *
         * @param exit_status 上一个命令的退出状态
         */
        void updateSpecialVars(int exit_status);
    };

} // namespace dash

#endif // DASH_VARIABLE_MANAGER_H