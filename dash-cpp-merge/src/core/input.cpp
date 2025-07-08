/**
 * @file input.cpp
 * @brief Input processor class implementation
 */

// Check if readline is correctly enabled
#ifdef READLINE_ENABLED
#define READLINE_IS_ENABLED "yes"
#else
#define READLINE_IS_ENABLED "no"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include "../../include/core/input.h"
#include "../../include/core/shell.h"
#include "../../include/utils/error.h"

// If readline library is enabled
#ifdef READLINE_ENABLED
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace dash
{
    // Readline library global callbacks and related variables
#ifdef READLINE_ENABLED
    // Global StdinInputSource pointer for callbacks
    static StdinInputSource* g_stdin_source = nullptr;

    // Custom Tab key handler function
    int custom_complete(int count, int key)
    {
        if (!g_stdin_source) {
            return rl_complete(count, key);
        }
        
        // Get current line and cursor position
        std::string line_buffer = rl_line_buffer ? rl_line_buffer : "";
        int cursor_pos = rl_point;
        
        // Find the start position of the current word
        int word_start = cursor_pos;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // Extract current word
        std::string current_word = "";
        if (word_start <= cursor_pos && static_cast<size_t>(cursor_pos) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, cursor_pos - word_start);
        }
        
        // Analyze command line to determine if completing a filename
        bool is_file_completion = false;
        std::string cmd_prefix = "";
        
        // Find command start position
        size_t cmd_start = line_buffer.find_first_not_of(" \t");
        if (cmd_start != std::string::npos && cmd_start < static_cast<size_t>(word_start)) {
            // Find first space after command
            size_t cmd_end = line_buffer.find_first_of(" \t", cmd_start);
            if (cmd_end != std::string::npos && cmd_end < static_cast<size_t>(word_start)) {
                // Current position is after command, might be file completion
                is_file_completion = true;
                cmd_prefix = line_buffer.substr(cmd_start, cmd_end - cmd_start);
            }
        }
        
        // Get completion results
        std::vector<std::string> matches;
        
        if (is_file_completion) {
            // File completion mode
            matches = g_stdin_source->complete(current_word, word_start, cursor_pos);
        } else {
            // Command completion mode
            matches = g_stdin_source->complete(current_word, word_start, cursor_pos);
        }
        
        // Record last Tab press time and content
        static auto last_tab_time = std::chrono::steady_clock::now();
        static std::string last_line_buffer = "";
        static int last_cursor_pos = -1;
        
        // Get current time
        auto current_time = std::chrono::steady_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_tab_time).count();
        
        // Check if double-tapped Tab quickly (within 500ms)
        bool is_double_tab = time_diff < 500 && 
                             line_buffer == last_line_buffer && 
                             cursor_pos == last_cursor_pos;
        
        // Update last Tab press state
        last_tab_time = current_time;
        last_line_buffer = line_buffer;
        last_cursor_pos = cursor_pos;
        
        // If no matches, try forced file name completion
        if (matches.empty() && !current_word.empty()) {
            // Get current directory
            std::string dir_path = ".";
            std::string file_prefix = current_word;
            
            // If path contains directory, separate directory and file prefix
            size_t slash_pos = current_word.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                dir_path = current_word.substr(0, slash_pos);
                file_prefix = current_word.substr(slash_pos + 1);
                
                if (dir_path.empty()) {
                    dir_path = "/";
                }
            }
            
            // Open directory and search for matches
            DIR* dir;
            if ((dir = opendir(dir_path.c_str())) != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string name = entry->d_name;
                    
                    // Skip . and .. (unless explicitly requested)
                    if ((name == "." || name == "..") && file_prefix != "." && file_prefix != "..") {
                        continue;
                    }
                    
                    // Check if matches prefix
                    if (name.compare(0, file_prefix.length(), file_prefix) == 0) {
                        // Construct full path
                        std::string full_path = (dir_path == ".") ? name : (dir_path + "/" + name);
                        
                        // Check if directory
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0) {
                            // Construct result
                            std::string result;
                            if (slash_pos != std::string::npos) {
                                result = current_word.substr(0, slash_pos + 1) + name;
                            } else {
                                result = name;
                            }
                            
                            // If directory, add slash
                            if (S_ISDIR(st.st_mode)) {
                                result += "/";
                            }
                            
                            matches.push_back(result);
                        }
                    }
                }
                closedir(dir);
            }
        }
        
        if (matches.empty()) {
            rl_ding(); // Sound alert
            return 0;
        }
        
        // If double-tapped Tab, show all matches
        if (is_double_tab) {
            // Display all matches
            std::cout << std::endl;
            for (const auto& match : matches) {
                std::cout << match << "  ";
            }
            std::cout << std::endl;
            
            // Re-display prompt and current line
            rl_forced_update_display();
            return 0;
        }
        
        // Single Tab case
        if (matches.size() == 1) {
            // If only one match, directly replace current word
            std::string match = matches[0];
            
            // Construct new line content
            std::string new_line = line_buffer.substr(0, word_start) + match;
            if (static_cast<size_t>(cursor_pos) < line_buffer.length()) {
                new_line += line_buffer.substr(cursor_pos);
            }
            
            // Replace readline's line buffer
            rl_replace_line(new_line.c_str(), 0);
            
            // Move cursor to appropriate position
            rl_point = word_start + match.length();
            
            // Re-display line
            rl_redisplay();
        } else {
            // If multiple matches, try to complete the longest common prefix
            std::string common_prefix = matches[0];
            for (size_t i = 1; i < matches.size(); ++i) {
                size_t j = 0;
                while (j < common_prefix.length() && j < matches[i].length() && 
                       common_prefix[j] == matches[i][j]) {
                    j++;
                }
                common_prefix = common_prefix.substr(0, j);
            }
            
            // If common prefix is longer than current word, replace it
            if (common_prefix.length() > current_word.length()) {
                std::string new_line = line_buffer.substr(0, word_start) + common_prefix;
                if (static_cast<size_t>(cursor_pos) < line_buffer.length()) {
                    new_line += line_buffer.substr(cursor_pos);
                }
                
                // Replace readline's line buffer
                rl_replace_line(new_line.c_str(), 0);
                
                // Move cursor to appropriate position
                rl_point = word_start + common_prefix.length();
                
                // Re-display line
                rl_redisplay();
            } else {
                // If cannot complete further, sound alert
                rl_ding();
            }
        }
        
        return 0;
    }

    // Readline's tab completion callback function
    char** readline_completion(const char* text, int start, int end)
    {
        // Tell readline not to use default filename completion
        rl_attempted_completion_over = 1;
        
        // Set replacement mode
        rl_completion_suppress_append = 1;
        rl_completion_append_character = '\0';
        
        // Check if text is null pointer
        if (!text) {
            return nullptr;
        }
        
        // Get current line content
        std::string line_buffer;
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        }
        
        // Find start position of current word
        int word_start = start;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // Extract current word
        std::string current_word = line_buffer.substr(word_start, end - word_start);
        
        if (g_stdin_source)
        {
            // Get completion results
            std::vector<std::string> matches = g_stdin_source->complete(current_word, word_start, end);
            
            if (!matches.empty())
            {
                // Convert results to readline expected format
                char** result = static_cast<char**>(malloc((matches.size() + 1) * sizeof(char*)));
                for (size_t i = 0; i < matches.size(); ++i)
                {
                    result[i] = strdup(matches[i].c_str());
                }
                result[matches.size()] = nullptr;
                
                // Key part: manually implement replacement
                if (matches.size() == 1) {
                    // If only one match, directly replace current word
                    std::string match = matches[0];
                    std::string new_line = line_buffer.substr(0, word_start) + match;
                    
                    // Preserve content after current word
                    if (static_cast<size_t>(end) < line_buffer.length()) {
                        new_line += line_buffer.substr(end);
                    }
                    
                    // Replace readline's line buffer
                    rl_replace_line(new_line.c_str(), 0);
                    
                    // Move cursor to appropriate position
                    rl_point = word_start + match.length();
                    
                    // Re-display line
                    rl_redisplay();
                    
                    // Return empty result, because we handled replacement manually
                    free(result[0]);
                    result[0] = nullptr;
                }
                
                return result;
            }
        }
        
        // If no matches, return NULL to let readline use default completion
        return nullptr;
    }
    
    // Single match generator, for custom completion support
    char* readline_match_generator(const char* text, int state)
    {
        // Check if text is null pointer
        if (!text) {
            return nullptr;
        }
        
        // Get current line content
        std::string line_buffer;
        if (rl_line_buffer) {
            line_buffer = rl_line_buffer;
        } else {
            line_buffer = text;
        }
        
        // Find start position of current word
        int word_start = rl_point;
        while (word_start > 0 && !strchr(" \t\n\"\\'`@$><=;|&{(", line_buffer[word_start - 1])) {
            word_start--;
        }
        
        // Extract current word
        std::string current_word = "";
        if (word_start <= rl_point && static_cast<size_t>(rl_point) <= line_buffer.length()) {
            current_word = line_buffer.substr(word_start, rl_point - word_start);
        }
        
        static size_t list_index;
        static std::vector<std::string> matches;
        
        // If state is 0, get match list again
        if (state == 0)
        {
            list_index = 0;
            // Get all matches on first call
            if (g_stdin_source) {
                matches = g_stdin_source->complete(current_word, word_start, rl_point);
            }
            else
            {
                matches.clear();
            }
        }
        
        // Return current match and increment index
        if (list_index < matches.size())
        {
            // If only one match, directly replace current word
            if (matches.size() == 1 && state == 0) {
                std::string match = matches[0];
                std::string new_line = line_buffer.substr(0, word_start) + match;
                
                // Preserve content after current word
                if (static_cast<size_t>(rl_point) < line_buffer.length()) {
                    new_line += line_buffer.substr(rl_point);
                }
                
                // Replace readline's line buffer
                rl_replace_line(new_line.c_str(), 0);
                
                // Move cursor to appropriate position
                rl_point = word_start + match.length();
                
                // Re-display line
                rl_redisplay();
            }
            
            return strdup(matches[list_index++].c_str());
        }
        
        return nullptr;
    }
#endif

    // FileInputSource implementation

    FileInputSource::FileInputSource(const std::string &filename)
        : filename_(filename)
    {
        file_.open(filename);
        if (!file_.is_open())
        {
            throw ShellException(ExceptionType::IO, "Cannot open file: " + filename);
        }
    }

    FileInputSource::~FileInputSource()
    {
        if (file_.is_open())
        {
            file_.close();
        }
    }

    std::string FileInputSource::readLine()
    {
        std::string line;
        if (std::getline(file_, line))
        {
            return line;
        }
        return "";
    }

    bool FileInputSource::isEOF() const
    {
        return file_.eof() || !file_.good();
    }

    std::string FileInputSource::getName() const
    {
        return filename_;
    }

    // StringInputSource implementation

    StringInputSource::StringInputSource(const std::string &str, const std::string &name)
        : current_line_(0), name_(name)
    {
        std::istringstream iss(str);
        std::string line;
        while (std::getline(iss, line))
        {
            lines_.push_back(line);
        }
    }

    std::string StringInputSource::readLine()
    {
        if (current_line_ < lines_.size())
        {
            return lines_[current_line_++];
        }
        return "";
    }

    bool StringInputSource::isEOF() const
    {
        return current_line_ >= lines_.size();
    }

    std::string StringInputSource::getName() const
    {
        return name_;
    }

    // StdinInputSource implementation

    StdinInputSource::StdinInputSource(bool interactive, const std::string &prompt, bool use_readline)
        : interactive_(interactive), prompt_(prompt), use_readline_(use_readline),
          completion_func_(nullptr)
    {
#ifdef READLINE_ENABLED
        // Set the global pointer for readline callbacks
        if (use_readline_ && interactive_)
        {
            g_stdin_source = this;

            // Set up tab completion
            rl_attempted_completion_function = readline_completion;
            rl_completion_entry_function = readline_match_generator;
            
            // Bind Tab key to our custom completion function
            rl_bind_key('\t', custom_complete);
            
            // Disable readline's default filename completion
            rl_attempted_completion_over = 1;
        }
#endif
    }

    StdinInputSource::~StdinInputSource()
    {
#ifdef READLINE_ENABLED
        if (g_stdin_source == this)
        {
            g_stdin_source = nullptr;
        }
#endif
    }

    std::string StdinInputSource::readLine()
    {
        if (!interactive_)
        {
            // Non-interactive mode, use std::getline
            std::string line;
            if (std::getline(std::cin, line))
            {
                return line;
            }
            return "";
        }
        else
        {
            // Interactive mode
#ifdef READLINE_ENABLED
            if (use_readline_)
            {
                // Use readline library
                char *line = readline(prompt_.c_str());
                if (line)
                {
                    // Add to history if non-empty
                    if (*line)
                    {
                        add_history(line);
                    }
                    
                    std::string result(line);
                    free(line);
                    return result;
                }
                return "";
            }
            else
#endif
            {
                // Fallback to stdio
                std::cout << prompt_;
                std::cout.flush();
                
                std::string line;
                if (std::getline(std::cin, line))
                {
                    return line;
                }
                return "";
            }
        }
    }

    bool StdinInputSource::isEOF() const
    {
        return std::cin.eof();
    }

    std::string StdinInputSource::getName() const
    {
        return "stdin";
    }

    void StdinInputSource::setCompletionFunction(CompletionFunc func)
    {
        completion_func_ = func;
    }

    std::vector<std::string> StdinInputSource::complete(const std::string& text, int start, int end)
    {
        if (completion_func_)
        {
            return completion_func_(text, start, end);
        }
        
        return {};
    }

    // InputHandler implementation

    InputHandler::InputHandler(Shell *shell)
        : shell_(shell)
    {
        // Create the initial stdin source
        stdin_source = std::make_unique<StdinInputSource>(shell->isInteractive(), 
            shell->getPrompt(), true);
        
        // Set the current source to stdin
        sources_.push_back(stdin_source.get());
        
        // Set completion function for stdin source
        stdin_source->setCompletionFunction([this](const std::string& text, int start, int end) {
            return this->tabCompletion(text, start, end);
        });
    }

    InputHandler::~InputHandler()
    {
        // Clear all sources except stdin
        while (!sources_.empty() && sources_.back() != stdin_source.get())
        {
            sources_.pop_back();
        }
    }

    std::string InputHandler::readLine(bool show_prompt)
    {
        // Check if there are any sources
        if (sources_.empty())
        {
            return "";
        }
        
        // Get the current source
        InputSource *source = sources_.back();
        
        // Read a line from the source
        std::string line = source->readLine();
        
        // If EOF and not stdin, pop the source and try again
        if (source->isEOF() && source != stdin_source.get())
        {
            sources_.pop_back();
            return readLine(show_prompt);
        }
        
        return line;
    }

    bool InputHandler::pushFile(const std::string &filename, int flags)
    {
        try
        {
            // Create a new file source
            auto file_source = std::make_unique<FileInputSource>(filename);
            
            // Store in sources list and add to sources stack
            file_sources_.push_back(std::move(file_source));
            sources_.push_back(file_sources_.back().get());
            
            return true;
        }
        catch (const ShellException &e)
        {
            // Error opening file
            return false;
        }
    }

    bool InputHandler::pushString(const std::string &str, const std::string &name)
    {
        // Create a new string source
        auto string_source = std::make_unique<StringInputSource>(str, name);
        
        // Store in sources list and add to sources stack
        string_sources_.push_back(std::move(string_source));
        sources_.push_back(string_sources_.back().get());
        
        return true;
    }

    void InputHandler::setCompletionFunction(CompletionFunc func)
    {
        if (stdin_source)
        {
            stdin_source->setCompletionFunction([this, func](const std::string& text, int start, int end) {
                // Try custom completion first
                auto matches = func(text, start, end);
                
                // If no matches, try default completion
                if (matches.empty())
                {
                    matches = this->tabCompletion(text, start, end);
                }
                
                return matches;
            });
        }
    }

    std::vector<std::string> InputHandler::tabCompletion(const std::string& text, int start, int end)
    {
        std::vector<std::string> matches;
        
        // Check if text is empty
        if (text.empty())
        {
            return matches;
        }
        
        // Get shell instance
        Shell *shell = shell_;
        if (!shell)
        {
            return matches;
        }
        
        // Find if command or argument completion
        bool is_first_word = true;
        std::string command = "";
        
        // Check if we're completing the first word (command)
        if (shell->getInputHandler().getCurrentSource() && 
            shell->getInputHandler().getCurrentSource()->getName() == "stdin")
        {
            // Get current line
            std::string line_buffer;
            
            // Get line from readline if available
#ifdef READLINE_ENABLED
            if (rl_line_buffer) {
                line_buffer = rl_line_buffer;
            }
#endif
            
            // Find first non-whitespace character
            size_t cmd_start = line_buffer.find_first_not_of(" \t");
            if (cmd_start != std::string::npos && cmd_start < static_cast<size_t>(start)) {
                // Find first whitespace after command
                size_t cmd_end = line_buffer.find_first_of(" \t", cmd_start);
                if (cmd_end != std::string::npos && cmd_end < static_cast<size_t>(start)) {
                    // Not the first word, extract command
                    is_first_word = false;
                    command = line_buffer.substr(cmd_start, cmd_end - cmd_start);
                }
            }
        }
        
        if (is_first_word)
        {
            // Command completion
            
            // Add builtin commands that match
            for (const auto& cmd : shell->getExecutor().getBuiltinCommands())
            {
                if (cmd.compare(0, text.length(), text) == 0)
                {
                    matches.push_back(cmd);
                }
            }
            
            // Add executables from PATH
            std::string path = shell->getVariableManager().get("PATH");
            if (!path.empty())
            {
                // Tokenize PATH
                std::string delimiter = ":";
                size_t pos = 0;
                std::string token;
                
                while ((pos = path.find(delimiter)) != std::string::npos)
                {
                    token = path.substr(0, pos);
                    
                    // Search this directory
                    DIR* dir;
                    if ((dir = opendir(token.c_str())) != NULL)
                    {
                        struct dirent* entry;
                        while ((entry = readdir(dir)) != NULL)
                        {
                            std::string name = entry->d_name;
                            
                            // Check if matches prefix
                            if (name.compare(0, text.length(), text) == 0)
                            {
                                // Check if executable
                                std::string full_path = token + "/" + name;
                                struct stat st;
                                if (stat(full_path.c_str(), &st) == 0)
                                {
                                    // Check if file and executable
                                    if (S_ISREG(st.st_mode) && 
                                        (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH))
                                    {
                                        // Check if already in matches
                                        if (std::find(matches.begin(), matches.end(), name) == matches.end())
                                        {
                                            matches.push_back(name);
                                        }
                                    }
                                }
                            }
                        }
                        closedir(dir);
                    }
                    
                    path.erase(0, pos + delimiter.length());
                }
                
                // Last directory in PATH
                if (!path.empty())
                {
                    DIR* dir;
                    if ((dir = opendir(path.c_str())) != NULL)
                    {
                        struct dirent* entry;
                        while ((entry = readdir(dir)) != NULL)
                        {
                            std::string name = entry->d_name;
                            
                            // Check if matches prefix
                            if (name.compare(0, text.length(), text) == 0)
                            {
                                // Check if executable
                                std::string full_path = path + "/" + name;
                                struct stat st;
                                if (stat(full_path.c_str(), &st) == 0)
                                {
                                    // Check if file and executable
                                    if (S_ISREG(st.st_mode) && 
                                        (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH))
                                    {
                                        // Check if already in matches
                                        if (std::find(matches.begin(), matches.end(), name) == matches.end())
                                        {
                                            matches.push_back(name);
                                        }
                                    }
                                }
                            }
                        }
                        closedir(dir);
                    }
                }
            }
            
            // Add aliases that match
            for (const auto& alias : shell->getAliasManager().getAliases())
            {
                if (alias.first.compare(0, text.length(), text) == 0)
                {
                    matches.push_back(alias.first);
                }
            }
        }
        else
        {
            // Argument completion (file completion)
            
            // Get current directory
            std::string dir_path = ".";
            std::string file_prefix = text;
            
            // If path contains directory separator, extract directory and prefix
            size_t slash_pos = text.find_last_of("/\\");
            if (slash_pos != std::string::npos)
            {
                dir_path = text.substr(0, slash_pos);
                file_prefix = text.substr(slash_pos + 1);
                
                if (dir_path.empty())
                {
                    dir_path = "/";
                }
            }
            
            // Open directory and search for matches
            DIR* dir;
            if ((dir = opendir(dir_path.c_str())) != NULL)
            {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    std::string name = entry->d_name;
                    
                    // Skip . and .. (unless explicitly requested)
                    if ((name == "." || name == "..") && file_prefix != "." && file_prefix != "..")
                    {
                        continue;
                    }
                    
                    // Check if matches prefix
                    if (name.compare(0, file_prefix.length(), file_prefix) == 0)
                    {
                        // Construct full path
                        std::string full_path = (dir_path == ".") ? name : (dir_path + "/" + name);
                        
                        // Check if directory
                        struct stat st;
                        if (stat(full_path.c_str(), &st) == 0)
                        {
                            // Construct result
                            std::string result;
                            if (slash_pos != std::string::npos)
                            {
                                result = text.substr(0, slash_pos + 1) + name;
                            }
                            else
                            {
                                result = name;
                            }
                            
                            // If directory, add slash
                            if (S_ISDIR(st.st_mode))
                            {
                                result += "/";
                            }
                            
                            matches.push_back(result);
                        }
                    }
                }
                closedir(dir);
            }
        }
        
        return matches;
    }

    InputSource* InputHandler::getCurrentSource() const
    {
        if (sources_.empty())
        {
            return nullptr;
        }
        
        return sources_.back();
    }

} // namespace dash
 