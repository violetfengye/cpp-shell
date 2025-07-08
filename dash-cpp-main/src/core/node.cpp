/**
 * @file node.cpp
 * @brief 语法树节点类实现
 */

#include <iostream>
#include <iomanip>
#include "core/node.h"

namespace dash
{

// Node 基类实现
Node::Node(NodeType type)
    : type_(type)
{
}

// CommandNode 实现
CommandNode::CommandNode()
    : Node(NodeType::COMMAND), background_(false)
{
}

void CommandNode::addArg(const std::string& arg)
{
    args_.push_back(arg);
}

void CommandNode::addAssignment(const std::string& assignment)
{
    assignments_.push_back(assignment);
}

void CommandNode::addRedirection(const Redirection& redir)
{
    redirections_.push_back(redir);
}

void CommandNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "CommandNode:" << std::endl;
    
    // 打印参数
    if (!args_.empty()) {
        std::cout << std::setw(indent + 2) << "" << "Args:" << std::endl;
        for (const auto& arg : args_) {
            std::cout << std::setw(indent + 4) << "" << arg << std::endl;
        }
    }
    
    // 打印变量赋值
    if (!assignments_.empty()) {
        std::cout << std::setw(indent + 2) << "" << "Assignments:" << std::endl;
        for (const auto& assignment : assignments_) {
            std::cout << std::setw(indent + 4) << "" << assignment << std::endl;
        }
    }
    
    // 打印重定向
    if (!redirections_.empty()) {
        std::cout << std::setw(indent + 2) << "" << "Redirections:" << std::endl;
        for (const auto& redir : redirections_) {
            std::cout << std::setw(indent + 4) << "" << "fd=" << redir.fd << " ";
            
            switch (redir.type) {
                case RedirType::REDIR_INPUT:
                    std::cout << "< ";
                    break;
                case RedirType::REDIR_OUTPUT:
                    std::cout << "> ";
                    break;
                case RedirType::REDIR_APPEND:
                    std::cout << ">> ";
                    break;
                case RedirType::REDIR_INPUT_DUP:
                    std::cout << "<& ";
                    break;
                case RedirType::REDIR_OUTPUT_DUP:
                    std::cout << ">& ";
                    break;
                case RedirType::REDIR_HEREDOC:
                    std::cout << "<< ";
                    break;
            }
            
            std::cout << redir.filename << std::endl;
        }
    }
}

// PipeNode 实现
PipeNode::PipeNode(std::unique_ptr<Node> left, std::unique_ptr<Node> right, bool background)
    : Node(NodeType::PIPE), left_(std::move(left)), right_(std::move(right)), background_(background)
{
}

void PipeNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "PipeNode:" << (background_ ? " (background)" : "") << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Left:" << std::endl;
    left_->print(indent + 4);
    
    std::cout << std::setw(indent + 2) << "" << "Right:" << std::endl;
    right_->print(indent + 4);
}

// ListNode 实现
ListNode::ListNode()
    : Node(NodeType::LIST)
{
}

void ListNode::addCommand(std::unique_ptr<Node> command, const std::string& op)
{
    commands_.push_back(std::move(command));
    operators_.push_back(op);
}

void ListNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "ListNode:" << std::endl;
    
    for (size_t i = 0; i < commands_.size(); ++i) {
        std::cout << std::setw(indent + 2) << "" << "Command " << i + 1 << ":" << std::endl;
        commands_[i]->print(indent + 4);
        
        if (i < operators_.size() && !operators_[i].empty()) {
            std::cout << std::setw(indent + 2) << "" << "Operator: " << operators_[i] << std::endl;
        }
    }
}

// IfNode 实现
IfNode::IfNode(std::unique_ptr<Node> condition, std::unique_ptr<Node> then_part, std::unique_ptr<Node> else_part)
    : Node(NodeType::IF), condition_(std::move(condition)), then_part_(std::move(then_part)), else_part_(std::move(else_part))
{
}

void IfNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "IfNode:" << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Condition:" << std::endl;
    condition_->print(indent + 4);
    
    std::cout << std::setw(indent + 2) << "" << "Then:" << std::endl;
    then_part_->print(indent + 4);
    
    if (else_part_) {
        std::cout << std::setw(indent + 2) << "" << "Else:" << std::endl;
        else_part_->print(indent + 4);
    }
}

// ForNode 实现
ForNode::ForNode(const std::string& var, const std::vector<std::string>& words, std::unique_ptr<Node> body)
    : Node(NodeType::FOR), var_(var), words_(words), body_(std::move(body))
{
}

void ForNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "ForNode:" << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Variable: " << var_ << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Words:" << std::endl;
    for (const auto& word : words_) {
        std::cout << std::setw(indent + 4) << "" << word << std::endl;
    }
    
    std::cout << std::setw(indent + 2) << "" << "Body:" << std::endl;
    body_->print(indent + 4);
}

// WhileNode 实现
WhileNode::WhileNode(std::unique_ptr<Node> condition, std::unique_ptr<Node> body, bool until)
    : Node(NodeType::WHILE), condition_(std::move(condition)), body_(std::move(body)), until_(until)
{
}

void WhileNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << (until_ ? "UntilNode:" : "WhileNode:") << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Condition:" << std::endl;
    condition_->print(indent + 4);
    
    std::cout << std::setw(indent + 2) << "" << "Body:" << std::endl;
    body_->print(indent + 4);
}

// CaseNode 实现
CaseNode::CaseNode(const std::string& word)
    : Node(NodeType::CASE), word_(word)
{
}

void CaseNode::addItem(const std::vector<std::string>& patterns, std::unique_ptr<Node> commands)
{
    items_.push_back(std::make_unique<CaseItem>(patterns, std::move(commands)));
}

void CaseNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "CaseNode:" << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Word: " << word_ << std::endl;
    
    for (size_t i = 0; i < items_.size(); ++i) {
        std::cout << std::setw(indent + 2) << "" << "Item " << i + 1 << ":" << std::endl;
        
        std::cout << std::setw(indent + 4) << "" << "Patterns:" << std::endl;
        for (const auto& pattern : items_[i]->patterns) {
            std::cout << std::setw(indent + 6) << "" << pattern << std::endl;
        }
        
        std::cout << std::setw(indent + 4) << "" << "Commands:" << std::endl;
        items_[i]->commands->print(indent + 6);
    }
}

// SubshellNode 实现
SubshellNode::SubshellNode(std::unique_ptr<Node> commands)
    : Node(NodeType::SUBSHELL), commands_(std::move(commands))
{
}

void SubshellNode::addRedirection(const Redirection& redir)
{
    redirections_.push_back(redir);
}

void SubshellNode::print(int indent) const
{
    std::cout << std::setw(indent) << "" << "SubshellNode:" << std::endl;
    
    std::cout << std::setw(indent + 2) << "" << "Commands:" << std::endl;
    commands_->print(indent + 4);
    
    // 打印重定向
    if (!redirections_.empty()) {
        std::cout << std::setw(indent + 2) << "" << "Redirections:" << std::endl;
        for (const auto& redir : redirections_) {
            std::cout << std::setw(indent + 4) << "" << "fd=" << redir.fd << " ";
            
            switch (redir.type) {
                case RedirType::REDIR_INPUT:
                    std::cout << "< ";
                    break;
                case RedirType::REDIR_OUTPUT:
                    std::cout << "> ";
                    break;
                case RedirType::REDIR_APPEND:
                    std::cout << ">> ";
                    break;
                case RedirType::REDIR_INPUT_DUP:
                    std::cout << "<& ";
                    break;
                case RedirType::REDIR_OUTPUT_DUP:
                    std::cout << ">& ";
                    break;
                case RedirType::REDIR_HEREDOC:
                    std::cout << "<< ";
                    break;
            }
            
            std::cout << redir.filename << std::endl;
        }
    }
}

} // namespace dash 