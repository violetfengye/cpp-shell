/**
 * @file alias.h
 * @brief 命令别名处理
 */

#ifndef DASH_ALIAS_H
#define DASH_ALIAS_H

#include <string>
#include <unordered_map>
#include <memory>
#include "../dash.h"

namespace dash {

/**
 * @brief 别名管理类
 */
class AliasManager {
public:
    /**
     * @brief 构造函数
     * 
     * @param shell Shell实例引用
     */
    explicit AliasManager(Shell& shell);

    /**
     * @brief 析构函数
     */
    ~AliasManager();

    /**
     * @brief 设置别名
     * 
     * @param name 别名名称
     * @param value 别名值
     */
    void setAlias(const std::string& name, const std::string& value);

    /**
     * @brief 获取别名
     * 
     * @param name 别名名称
     * @return std::string 别名值，如果别名不存在则返回空字符串
     */
    std::string getAlias(const std::string& name) const;

    /**
     * @brief 删除别名
     * 
     * @param name 别名名称
     * @return bool 是否成功删除
     */
    bool removeAlias(const std::string& name);

    /**
     * @brief 检查别名是否存在
     * 
     * @param name 别名名称
     * @return bool 别名是否存在
     */
    bool hasAlias(const std::string& name) const;

    /**
     * @brief 获取所有别名
     * 
     * @return const std::unordered_map<std::string, std::string>& 所有别名的映射表
     */
    const std::unordered_map<std::string, std::string>& getAllAliases() const;

    /**
     * @brief 清除所有别名
     */
    void clear();

private:
    Shell& shell_;  // Shell实例引用
    std::unordered_map<std::string, std::string> aliases_;  // 别名映射表
};

} // namespace dash

#endif // DASH_ALIAS_H 