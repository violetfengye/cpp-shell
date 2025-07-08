/**
 * @file node.cpp
 * @brief 语法树节点实现
 */

#include "core/node.h"
#include <iostream>
#include <sstream>

namespace dash
{

    Node::Node(NodeType type)
        : type_(type), left_child_(nullptr), right_child_(nullptr),
          condition_(nullptr), then_branch_(nullptr), else_branch_(nullptr),
          body_(nullptr), child_(nullptr)
    {
    }

    Node::~Node()
    {
        // 递归删除子节点
        delete left_child_;
        delete right_child_;
        delete condition_;
        delete then_branch_;
        delete else_branch_;
        delete body_;
        delete child_;

        // 删除cases中的节点
        for (auto& pair : cases_) {
            delete pair.second;
        }
    }

    NodeType Node::getType() const
    {
        return type_;
    }

    void Node::setType(NodeType type)
    {
        type_ = type;
    }

    std::string Node::getCommandName() const
    {
        if (!args_.empty()) {
            return args_[0];
        }
        return std::string();
    }

    std::vector<std::string> Node::getArgs() const
    {
        return args_;
    }

    void Node::addArg(const std::string& arg)
    {
        args_.push_back(arg);
    }

    void Node::setArgs(const std::vector<std::string>& args)
    {
        args_ = args;
    }

    std::vector<Redirection> Node::getRedirections() const
    {
        return redirections_;
    }

    void Node::addRedirection(const Redirection& redirection)
    {
        redirections_.push_back(redirection);
    }

    void Node::setLeftChild(Node* node)
    {
        delete left_child_;
        left_child_ = node;
    }

    Node* Node::getLeftChild() const
    {
        return left_child_;
    }

    void Node::setRightChild(Node* node)
    {
        delete right_child_;
        right_child_ = node;
    }

    Node* Node::getRightChild() const
    {
        return right_child_;
    }

    void Node::setCondition(Node* node)
    {
        delete condition_;
        condition_ = node;
    }

    Node* Node::getCondition() const
    {
        return condition_;
    }

    void Node::setThenBranch(Node* node)
    {
        delete then_branch_;
        then_branch_ = node;
    }

    Node* Node::getThenBranch() const
    {
        return then_branch_;
    }

    void Node::setElseBranch(Node* node)
    {
        delete else_branch_;
        else_branch_ = node;
    }

    Node* Node::getElseBranch() const
    {
        return else_branch_;
    }

    void Node::setBody(Node* node)
    {
        delete body_;
        body_ = node;
    }

    Node* Node::getBody() const
    {
        return body_;
    }

    void Node::setChild(Node* node)
    {
        delete child_;
        child_ = node;
    }

    Node* Node::getChild() const
    {
        return child_;
    }

    void Node::setVarName(const std::string& var_name)
    {
        var_name_ = var_name;
    }

    std::string Node::getVarName() const
    {
        return var_name_;
    }

    void Node::addValue(const std::string& value)
    {
        values_.push_back(value);
    }

    void Node::setValues(const std::vector<std::string>& values)
    {
        values_ = values;
    }

    std::vector<std::string> Node::getValues() const
    {
        return values_;
    }

    void Node::setWord(const std::string& word)
    {
        word_ = word;
    }

    std::string Node::getWord() const
    {
        return word_;
    }

    void Node::addCase(const std::vector<std::string>& patterns, Node* actions)
    {
        cases_.push_back(std::make_pair(patterns, actions));
    }

    std::vector<std::pair<std::vector<std::string>, Node*>> Node::getCases() const
    {
        return cases_;
    }

    void Node::setAssignments(const std::vector<std::pair<std::string, std::string>>& assignments)
    {
        assignments_ = assignments;
    }

    std::vector<std::pair<std::string, std::string>> Node::getAssignments() const
    {
        return assignments_;
    }

    void Node::addAssignment(const std::string& name, const std::string& value)
    {
        assignments_.push_back(std::make_pair(name, value));
    }

    std::string Node::toString() const
    {
        std::stringstream ss;

        switch (type_) {
        case NodeType::COMMAND:
            ss << "命令: ";
            for (const auto& arg : args_) {
                ss << arg << " ";
            }
            break;

        case NodeType::PIPE:
            ss << "管道: [左] " << (left_child_ ? left_child_->toString() : "null")
               << " | [右] " << (right_child_ ? right_child_->toString() : "null");
            break;

        case NodeType::SEQUENCE:
            ss << "序列: [左] " << (left_child_ ? left_child_->toString() : "null")
               << "; [右] " << (right_child_ ? right_child_->toString() : "null");
            break;

        case NodeType::BACKGROUND:
            ss << "后台: " << (child_ ? child_->toString() : "null") << " &";
            break;

        case NodeType::IF:
            ss << "IF: [条件] " << (condition_ ? condition_->toString() : "null")
               << " [THEN] " << (then_branch_ ? then_branch_->toString() : "null");
            if (else_branch_) {
                ss << " [ELSE] " << else_branch_->toString();
            }
            break;

        case NodeType::WHILE:
            ss << "WHILE: [条件] " << (condition_ ? condition_->toString() : "null")
               << " [循环体] " << (body_ ? body_->toString() : "null");
            break;

        case NodeType::FOR:
            ss << "FOR: " << var_name_ << " in ";
            for (const auto& value : values_) {
                ss << value << " ";
            }
            ss << "[循环体] " << (body_ ? body_->toString() : "null");
            break;

        case NodeType::CASE:
            ss << "CASE: " << word_ << " in ";
            for (const auto& case_item : cases_) {
                ss << "(";
                for (const auto& pattern : case_item.first) {
                    ss << pattern << "|";
                }
                ss << ") ";
                if (case_item.second) {
                    ss << case_item.second->toString();
                }
                ss << ";; ";
            }
            break;

        case NodeType::FUNCTION:
            ss << "函数: " << getCommandName() << "() { ... }";
            break;

        case NodeType::SUBSHELL:
            ss << "子shell: (" << (child_ ? child_->toString() : "null") << ")";
            break;

        case NodeType::GROUP:
            ss << "命令组: { " << (child_ ? child_->toString() : "null") << " }";
            break;

        default:
            ss << "未知节点类型";
            break;
        }

        // 添加重定向信息
        if (!redirections_.empty()) {
            ss << " [重定向: ";
            for (const auto& redir : redirections_) {
                switch (redir.type) {
                case RedirectionType::INPUT:
                    ss << "<" << redir.filename << " ";
                    break;
                case RedirectionType::OUTPUT:
                    ss << ">" << redir.filename << " ";
                    break;
                case RedirectionType::APPEND:
                    ss << ">>" << redir.filename << " ";
                    break;
                case RedirectionType::DUPLICATE:
                    ss << redir.fd << ">&" << redir.filename << " ";
                    break;
                case RedirectionType::HERE_DOC:
                    ss << "<<EOF" << " ";
                    break;
                }
            }
            ss << "]";
        }

        return ss.str();
    }

} // namespace dash 