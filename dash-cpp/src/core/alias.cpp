/**
 * @file alias.cpp
 * @brief 命令别名处理实现
 */

#include "../../include/core/alias.h"
#include "../../include/core/shell.h"

namespace dash {

AliasManager::AliasManager(Shell& shell) : shell_(shell) {
    // 初始化
}

AliasManager::~AliasManager() {
    // 清理资源
}

void AliasManager::setAlias(const std::string& name, const std::string& value) {
    if (name.empty()) {
        return;
    }
    
    aliases_[name] = value;
}

std::string AliasManager::getAlias(const std::string& name) const {
    auto it = aliases_.find(name);
    if (it != aliases_.end()) {
        return it->second;
    }
    return "";
}

bool AliasManager::removeAlias(const std::string& name) {
    return aliases_.erase(name) > 0;
}

bool AliasManager::hasAlias(const std::string& name) const {
    return aliases_.find(name) != aliases_.end();
}

const std::unordered_map<std::string, std::string>& AliasManager::getAllAliases() const {
    return aliases_;
}

void AliasManager::clear() {
    aliases_.clear();
}

} // namespace dash 