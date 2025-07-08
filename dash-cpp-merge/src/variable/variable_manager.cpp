/**
 * @file variable_manager.cpp
 * @brief 变量管理器实现
 */

#include "variable/variable_manager.h"
#include "core/shell.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <limits.h>
#include <regex>

namespace dash
{

    // Variable implementation
    Variable::Variable(const std::string &name, const std::string &value, int flags)
        : name_(name), value_(value), flags_(flags)
    {
    }

    bool Variable::setValue(const std::string &value)
    {
        // If variable is readonly, it cannot be modified
        if (hasFlag(VAR_READONLY))
        {
            return false;
        }

        value_ = value;
        return true;
    }

    // VariableManager implementation

    VariableManager::VariableManager(Shell *shell)
        : shell_(shell)
    {
        initialize();
    }

    VariableManager::~VariableManager()
    {
        // Clear variable table
        variables_.clear();
    }

    bool VariableManager::initialize()
    {
        // Import environment variables
        for (char **env = environ; env && *env; ++env)
        {
            std::string entry = *env;
            size_t pos = entry.find('=');
            if (pos != std::string::npos)
            {
                std::string name = entry.substr(0, pos);
                std::string value = entry.substr(pos + 1);
                set(name, value, Variable::VAR_EXPORT);
            }
        }

        // Set default variables
        set("PS1", "$ ", Variable::VAR_NONE);
        set("PS2", "> ", Variable::VAR_NONE);
        set("IFS", " \t\n", Variable::VAR_NONE);

        // Set special variables
        set("?", "0", Variable::VAR_SPECIAL);
        set("$", std::to_string(getpid()), Variable::VAR_SPECIAL);

        // Set PATH if it doesn't exist
        if (!exists("PATH"))
        {
            set("PATH", "/usr/local/bin:/usr/bin:/bin", Variable::VAR_EXPORT);
        }

        // Set HOME if it doesn't exist
        if (!exists("HOME"))
        {
            const char *home = getenv("HOME");
            if (home)
            {
                set("HOME", home, Variable::VAR_EXPORT);
            }
            else
            {
                set("HOME", "/", Variable::VAR_EXPORT);
            }
        }

        return true;
    }

    void VariableManager::initializeEnvironmentVariables()
    {
        // Get environment variables
        extern char **environ;
        for (char **env = environ; *env != nullptr; ++env) {
            std::string env_str = *env;
            size_t pos = env_str.find('=');
            if (pos != std::string::npos) {
                std::string name = env_str.substr(0, pos);
                std::string value = env_str.substr(pos + 1);
                environment_variables_[name] = value;
            }
        }
    }

    void VariableManager::initializeSpecialVariables()
    {
        // Get current working directory
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            setVariable("PWD", cwd);
        } else {
            setVariable("PWD", ".");
        }

        // Get HOME directory
        const char *home = getenv("HOME");
        if (home) {
            setVariable("HOME", home);
        } else {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                setVariable("HOME", pw->pw_dir);
            }
        }

        // Get username
        const char *user = getenv("USER");
        if (user) {
            setVariable("USER", user);
        } else {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                setVariable("USER", pw->pw_name);
            }
        }

        // Set SHELL variable
        setVariable("SHELL", "dash-cpp-merge");

        // Set PATH variable
        if (!hasVariable("PATH")) {
            setVariable("PATH", "/usr/local/bin:/usr/bin:/bin");
        }

        // Set PS1 variable (prompt)
        if (!hasVariable("PS1")) {
            setVariable("PS1", "\\u@\\h:\\w\\$ ");
        }

        // Set PS2 variable (secondary prompt)
        if (!hasVariable("PS2")) {
            setVariable("PS2", "> ");
        }

        // Set IFS variable (internal field separator)
        if (!hasVariable("IFS")) {
            setVariable("IFS", " \t\n");
        }
    }

    bool VariableManager::set(const std::string &name, const std::string &value, int flags)
    {
        // Check if variable name is valid
        if (name.empty())
        {
            return false;
        }

        // Check if it's a special variable
        if (name == "?" || name == "$" || name == "#" || name == "0")
        {
            flags |= Variable::VAR_SPECIAL;
        }

        // If variable already exists
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // If variable is readonly, it cannot be modified
            if (it->second->hasFlag(Variable::VAR_READONLY))
            {
                return false;
            }

            // Update variable value and flags
            it->second->setValue(value);

            // Keep original flags, add new flags
            int new_flags = it->second->getFlags() | flags;
            it->second->setFlags(new_flags);

            // If variable is exported, update environment variable
            if (it->second->hasFlag(Variable::VAR_EXPORT))
            {
                setenv(name.c_str(), value.c_str(), 1);
            }
        }
        else
        {
            // Create new variable
            variables_[name] = std::make_unique<Variable>(name, value, flags);

            // If variable is exported, set environment variable
            if (flags & Variable::VAR_EXPORT)
            {
                setenv(name.c_str(), value.c_str(), 1);
            }
        }

        return true;
    }

    std::string VariableManager::get(const std::string &name) const
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            return it->second->getValue();
        }

        return "";
    }

    bool VariableManager::exists(const std::string &name) const
    {
        return variables_.find(name) != variables_.end();
    }

    bool VariableManager::unset(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // If variable is readonly or special, it cannot be deleted
            if (it->second->hasFlag(Variable::VAR_READONLY) ||
                it->second->hasFlag(Variable::VAR_SPECIAL))
            {
                return false;
            }

            // If variable is exported, remove it from environment
            if (it->second->hasFlag(Variable::VAR_EXPORT))
            {
                unsetenv(name.c_str());
            }

            // Remove from variable table
            variables_.erase(it);
            return true;
        }

        return false;
    }

    bool VariableManager::exportVar(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // Add export flag
            it->second->addFlag(Variable::VAR_EXPORT);

            // Set environment variable
            setenv(name.c_str(), it->second->getValue().c_str(), 1);
            return true;
        }

        return false;
    }

    bool VariableManager::makeReadOnly(const std::string &name)
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            // Add readonly flag
            it->second->addFlag(Variable::VAR_READONLY);
            return true;
        }

        return false;
    }

    int VariableManager::getFlags(const std::string &name) const
    {
        auto it = variables_.find(name);
        if (it != variables_.end())
        {
            return it->second->getFlags();
        }

        return Variable::VAR_NONE;
    }

    std::map<std::string, std::string> VariableManager::getAllVars() const
    {
        std::map<std::string, std::string> result;
        
        for (const auto &pair : variables_)
        {
            result[pair.first] = pair.second->getValue();
        }

        return result;
    }

    std::map<std::string, std::string> VariableManager::getExportedVars() const
    {
        std::map<std::string, std::string> result;
        
        for (const auto &pair : variables_)
        {
            if (pair.second->hasFlag(Variable::VAR_EXPORT))
            {
                result[pair.first] = pair.second->getValue();
            }
        }

        return result;
    }

    bool VariableManager::hasVariable(const std::string &name) const
    {
        return variables_.find(name) != variables_.end() ||
               environment_variables_.find(name) != environment_variables_.end();
    }

    std::string VariableManager::getVariable(const std::string &name) const
    {
        // First check local variables
        auto it = variables_.find(name);
        if (it != variables_.end()) {
            return it->second->getValue();
        }

        // Then check environment variables
        auto env_it = environment_variables_.find(name);
        if (env_it != environment_variables_.end()) {
            return env_it->second;
        }

        // If it's a special variable, handle specially
        if (name == "?") {
            return std::to_string(shell_->getExitCode());
        } else if (name == "$") {
            return std::to_string(getpid());
        } else if (name == "#") {
            return std::to_string(positional_parameters_.size());
        }

        // Finally check positional parameters
        try {
            int index = std::stoi(name);
            if (index >= 0 && index < static_cast<int>(positional_parameters_.size())) {
                return positional_parameters_[index];
            }
        } catch (const std::exception &) {
            // Not a number, ignore
        }

        // Variable doesn't exist
        return "";
    }

    void VariableManager::setVariable(const std::string &name, const std::string &value)
    {
        set(name, value, Variable::VAR_NONE);
    }

    void VariableManager::setEnvironmentVariable(const std::string &name, const std::string &value)
    {
        set(name, value, Variable::VAR_EXPORT);
    }

    void VariableManager::unsetVariable(const std::string &name)
    {
        unset(name);
    }

    void VariableManager::exportVariable(const std::string &name)
    {
        exportVar(name);
    }

    void VariableManager::setPositionalParameters(const std::vector<std::string> &params)
    {
        positional_parameters_ = params;
    }

    std::vector<std::string> VariableManager::getPositionalParameters() const
    {
        return positional_parameters_;
    }

    std::string VariableManager::getPositionalParameter(int index) const
    {
        if (index >= 0 && index < static_cast<int>(positional_parameters_.size())) {
            return positional_parameters_[index];
        }
        return "";
    }

    std::string VariableManager::getAllPositionalParameters() const
    {
        std::string result;
        for (const auto &param : positional_parameters_) {
            if (!result.empty()) {
                result += " ";
            }
            result += param;
        }
        return result;
    }

    std::map<std::string, std::string> VariableManager::getAllVariables() const
    {
        std::map<std::string, std::string> all_vars;

        // Add all local variables
        for (const auto &pair : variables_)
        {
            all_vars[pair.first] = pair.second->getValue();
        }

        // Add all environment variables
        all_vars.insert(environment_variables_.begin(), environment_variables_.end());

        return all_vars;
    }

    void VariableManager::updateSpecialVars(int exit_status)
    {
        // Update exit status variable
        set("?", std::to_string(exit_status), Variable::VAR_SPECIAL);
        
        // Update process ID variable
        set("$", std::to_string(getpid()), Variable::VAR_SPECIAL);
        
        // Update positional parameters count
        set("#", std::to_string(positional_parameters_.size()), Variable::VAR_SPECIAL);
    }

    std::string VariableManager::expandVariables(const std::string &str)
    {
        if (str.empty())
        {
            return str;
        }

        // Regular expression to match variable references
        // ${name}, $name, $?
        std::regex var_regex("\\$(\\{([^}]+)\\}|([a-zA-Z0-9_?$#]+))");
        std::string result = str;
        std::smatch match;
        std::string::const_iterator search_start(str.cbegin());

        // Copy the string and replace variables
        std::string temp = str;
        while (std::regex_search(search_start, str.cend(), match, var_regex))
        {
            std::string var_name;
            if (match[2].matched) // ${name}
            {
                var_name = match[2].str();
            }
            else // $name
            {
                var_name = match[3].str();
            }

            // Get variable value
            std::string var_value = getVariable(var_name);

            // Replace in the original string
            size_t pos = match.position();
            size_t len = match.length();
            result.replace(pos, len, var_value);

            // Move search position
            search_start = match.suffix().first;
        }

        return result;
    }

} // namespace dash 