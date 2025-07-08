/**
 * @file input.cpp
 * @brief 输入处理器类实现
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "core/input.h"
#include "core/shell.h"
#include "utils/error.h"

namespace dash
{

    // FileInputSource 实现

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

    // StringInputSource 实现

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

    // StdinInputSource 实现

    StdinInputSource::StdinInputSource(bool interactive, const std::string &prompt)
        : eof_(false), interactive_(interactive), prompt_(prompt)
    {
    }

    std::string StdinInputSource::readLine()
    {
        if (interactive_)
        {
            std::cout << prompt_ << std::flush;
        }

        std::string line;
        if (std::getline(std::cin, line))
        {
            return line;
        }

        eof_ = true;
        return "";
    }

    bool StdinInputSource::isEOF() const
    {
        return eof_ || std::cin.eof();
    }

    std::string StdinInputSource::getName() const
    {
        return "stdin";
    }

    void StdinInputSource::setPrompt(const std::string &prompt)
    {
        prompt_ = prompt;
    }

    // InputHandler 实现

    InputHandler::InputHandler(Shell *shell)
        : shell_(shell)
    {
        // 初始化时将标准输入作为默认输入源
        bool interactive = shell_->isInteractive();
        input_stack_.push(std::make_unique<StdinInputSource>(interactive));
    }

    InputHandler::~InputHandler()
    {
        // 清空输入源栈
        while (!input_stack_.empty())
        {
            input_stack_.pop();
        }
    }

    std::string InputHandler::readLine(bool show_prompt)
    {
        if (input_stack_.empty())
        {
            return "";
        }

        // 如果当前输入源到达文件末尾，则弹出并尝试下一个
        while (!input_stack_.empty() && input_stack_.top()->isEOF())
        {
            input_stack_.pop();
        }

        if (input_stack_.empty())
        {
            return "";
        }

        // 如果是交互式标准输入并且需要显示提示符
        if (show_prompt && input_stack_.top()->getName() == "stdin")
        {
            auto *stdin_source = dynamic_cast<StdinInputSource *>(input_stack_.top().get());
            if (stdin_source)
            {
                // 可以在这里根据 shell 状态设置不同的提示符
                std::string prompt = "$ ";
                stdin_source->setPrompt(prompt);
            }
        }

        return input_stack_.top()->readLine();
    }

    bool InputHandler::pushFile(const std::string &filename, int flags)
    {
        try
        {
            // 如果设置了 IF_PUSH_FILE 标志，则保留当前输入源
            if (!(flags & IF_PUSH_FILE) && !input_stack_.empty())
            {
                input_stack_.pop();
            }

            // 尝试打开文件
            std::ifstream file(filename);
            if (!file.is_open())
            {
                if (flags & IF_NOFILE_OK)
                {
                    return false;
                }
                throw ShellException(ExceptionType::IO, "Cannot open file: " + filename);
            }
            file.close();

            input_stack_.push(std::make_unique<FileInputSource>(filename));
            return true;
        }
        catch (const ShellException &e)
        {
            if (!(flags & IF_NOFILE_OK))
            {
                throw;
            }
            return false;
        }
    }

    bool InputHandler::pushString(const std::string &str, const std::string &name)
    {
        input_stack_.push(std::make_unique<StringInputSource>(str, name));
        return true;
    }

    bool InputHandler::popFile()
    {
        if (input_stack_.empty())
        {
            return false;
        }

        input_stack_.pop();

        // 如果栈为空，则添加标准输入作为默认输入源
        if (input_stack_.empty())
        {
            bool interactive = shell_->isInteractive();
            input_stack_.push(std::make_unique<StdinInputSource>(interactive));
        }

        return true;
    }

    bool InputHandler::isEOF() const
    {
        return input_stack_.empty() || input_stack_.top()->isEOF();
    }

    std::string InputHandler::getCurrentSourceName() const
    {
        if (input_stack_.empty())
        {
            return "none";
        }

        return input_stack_.top()->getName();
    }

    void InputHandler::setPrompt(const std::string &prompt)
    {
        if (!input_stack_.empty() && input_stack_.top()->getName() == "stdin")
        {
            auto *stdin_source = dynamic_cast<StdinInputSource *>(input_stack_.top().get());
            if (stdin_source)
            {
                stdin_source->setPrompt(prompt);
            }
        }
    }

} // namespace dash