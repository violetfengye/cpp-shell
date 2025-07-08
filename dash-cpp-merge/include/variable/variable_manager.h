/**
 * @file variable_manager.h
 * @brief 变量管理器头文件
 */

#ifndef DASH_VARIABLE_MANAGER_H
#define DASH_VARIABLE_MANAGER_H

#include <string>
#include <map>
#include <vector>

namespace dash
{
    // 前向声明
    class Shell;

    /**
     * @brief 变量管理器类
     */
    class VariableManager
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit VariableManager(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~VariableManager();

        /**
         * @brief 初始化变量管理器
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 检查变量是否存在
         * @param name 变量名
         * @return 变量是否存在
         */
        bool hasVariable(const std::string &name) const;

        /**
         * @brief 获取变量值
         * @param name 变量名
         * @return 变量值，如果变量不存在则返回空字符串
         */
        std::string getVariable(const std::string &name) const;

        /**
         * @brief 设置变量
         * @param name 变量名
         * @param value 变量值
         */
        void setVariable(const std::string &name, const std::string &value);

        /**
         * @brief 设置环境变量
         * @param name 变量名
         * @param value 变量值
         */
        void setEnvironmentVariable(const std::string &name, const std::string &value);

        /**
         * @brief 删除变量
         * @param name 变量名
         */
        void unsetVariable(const std::string &name);

        /**
         * @brief 导出变量为环境变量
         * @param name 变量名
         */
        void exportVariable(const std::string &name);

        /**
         * @brief 设置位置参数
         * @param params 位置参数
         */
        void setPositionalParameters(const std::vector<std::string> &params);

        /**
         * @brief 获取所有位置参数
         * @return 位置参数列表
         */
        std::vector<std::string> getPositionalParameters() const;

        /**
         * @brief 获取指定位置参数
         * @param index 参数索引
         * @return 参数值，如果索引无效则返回空字符串
         */
        std::string getPositionalParameter(int index) const;

        /**
         * @brief 获取所有位置参数的字符串表示
         * @return 所有位置参数，用空格分隔
         */
        std::string getAllPositionalParameters() const;

        /**
         * @brief 获取所有变量
         * @return 变量名到变量值的映射
         */
        std::map<std::string, std::string> getAllVariables() const;

    private:
        /**
         * @brief 初始化环境变量
         */
        void initializeEnvironmentVariables();

        /**
         * @brief 初始化特殊变量
         */
        void initializeSpecialVariables();

    private:
        Shell *shell_;                                        ///< Shell实例指针
        std::map<std::string, std::string> variables_;        ///< 本地变量
        std::map<std::string, std::string> environment_variables_; ///< 环境变量
        std::vector<std::string> positional_parameters_;      ///< 位置参数
    };

} // namespace dash

#endif // DASH_VARIABLE_MANAGER_H 