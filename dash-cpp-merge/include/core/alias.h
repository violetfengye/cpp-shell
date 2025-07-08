/**
 * @file alias.h
 * @brief 别名管理头文件
 */

#ifndef DASH_ALIAS_H
#define DASH_ALIAS_H

#include <string>
#include <map>

namespace dash
{
    // 前向声明
    class Shell;

    /**
     * @brief 别名管理类
     */
    class Alias
    {
    public:
        /**
         * @brief 构造函数
         * @param shell Shell实例指针
         */
        explicit Alias(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~Alias();

        /**
         * @brief 初始化别名管理器
         * @param filename 别名文件名，为空则使用默认文件
         * @return 是否初始化成功
         */
        bool initialize(const std::string &filename = "");

        /**
         * @brief 加载别名
         * @return 是否加载成功
         */
        bool load();

        /**
         * @brief 保存别名
         * @return 是否保存成功
         */
        bool save() const;

        /**
         * @brief 添加别名
         * @param name 别名名称
         * @param value 别名值
         * @return 是否添加成功
         */
        bool add(const std::string &name, const std::string &value);

        /**
         * @brief 移除别名
         * @param name 别名名称
         * @return 是否移除成功
         */
        bool remove(const std::string &name);

        /**
         * @brief 检查别名是否存在
         * @param name 别名名称
         * @return 是否存在
         */
        bool has(const std::string &name) const;

        /**
         * @brief 获取别名值
         * @param name 别名名称
         * @return 别名值，如果不存在则返回空字符串
         */
        std::string get(const std::string &name) const;

        /**
         * @brief 获取所有别名
         * @return 别名名称到别名值的映射
         */
        std::map<std::string, std::string> getAll() const;

        /**
         * @brief 展开命令中的别名
         * @param command 命令
         * @return 展开后的命令
         */
        std::string expand(const std::string &command) const;

    private:
        Shell *shell_;                               ///< Shell实例指针
        std::string alias_file_;                     ///< 别名文件名
        std::map<std::string, std::string> aliases_; ///< 别名
    };

} // namespace dash

#endif // DASH_ALIAS_H 