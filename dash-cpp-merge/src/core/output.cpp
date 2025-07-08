/**
 * @file output.cpp
 * @brief Output handler implementation
 */

#include "../../include/core/output.h"
#include "../../include/core/shell.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

namespace dash {

// ANSI color codes
namespace Color {
    static const char* RESET     = "\033[0m";
    static const char* RED       = "\033[31m";
    static const char* GREEN     = "\033[32m";
    static const char* YELLOW    = "\033[33m";
    static const char* BLUE      = "\033[34m";
    static const char* MAGENTA   = "\033[35m";
    static const char* CYAN      = "\033[36m";
    static const char* BOLD      = "\033[1m";
    static const char* UNDERLINE = "\033[4m";
}

Output::Output(Shell& shell)
    : shell_(shell), colorEnabled_(true), debugEnabled_(false),
      outStream_(&std::cout), errStream_(&std::cerr) {
    // Check if running in terminal
    colorEnabled_ = isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
}

Output::~Output() {
    // Cleanup resources
}

void Output::print(const std::string& message) {
    output(message, OutputType::NORMAL, false);
}

void Output::println(const std::string& message) {
    output(message, OutputType::NORMAL, true);
}

void Output::error(const std::string& message) {
    output(message, OutputType::ERROR, false);
}

void Output::errorln(const std::string& message) {
    output(message, OutputType::ERROR, true);
}

void Output::debug(const std::string& message) {
    if (debugEnabled_) {
        output(message, OutputType::DEBUG, false);
    }
}

void Output::debugln(const std::string& message) {
    if (debugEnabled_) {
        output(message, OutputType::DEBUG, true);
    }
}

void Output::prompt(const std::string& prompt) {
    output(prompt, OutputType::PROMPT, false);
}

void Output::setColorEnabled(bool enable) {
    colorEnabled_ = enable;
}

void Output::setDebugEnabled(bool enable) {
    debugEnabled_ = enable;
}

bool Output::isColorEnabled() const {
    return colorEnabled_;
}

bool Output::isDebugEnabled() const {
    return debugEnabled_;
}

void Output::setStreams(std::ostream& outStream, std::ostream& errStream) {
    outStream_ = &outStream;
    errStream_ = &errStream;
}

void Output::output(const std::string& message, OutputType type, bool newline) {
    std::ostream& stream = (type == OutputType::ERROR) ? *errStream_ : *outStream_;
    
    if (colorEnabled_) {
        switch (type) {
            case OutputType::NORMAL:
                // Normal output, no color needed
                stream << message;
                break;
            case OutputType::ERROR:
                // Error output, red
                stream << Color::RED << message << Color::RESET;
                break;
            case OutputType::DEBUG:
                // Debug output, yellow
                stream << Color::YELLOW << "[DEBUG] " << message << Color::RESET;
                break;
            case OutputType::PROMPT:
                // Prompt, cyan
                stream << Color::CYAN << message << Color::RESET;
                break;
        }
    } else {
        // No color
        if (type == OutputType::DEBUG) {
            stream << "[DEBUG] ";
        }
        stream << message;
    }
    
    if (newline) {
        stream << std::endl;
    } else {
        stream.flush();
    }
}

} // namespace dash 