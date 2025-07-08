/**
 * @file parser.cpp
 * @brief 语法解析器实现
 */

#include "core/parser.h"
#include "core/shell.h"
#include "core/lexer.h"
#include "core/node.h"
#include <iostream>
#include <vector>

namespace dash
{

    Parser::Parser(Shell *shell)
        : shell_(shell), lexer_(nullptr), current_token_()
    {
    }

    Parser::~Parser()
    {
    }

    bool Parser::initialize()
    {
        lexer_ = shell_->getLexer();
        if (!lexer_) {
            std::cerr << "无法获取词法分析器" << std::endl;
            return false;
        }
        return true;
    }

    Node* Parser::parse()
    {
        // 获取第一个Token
        current_token_ = lexer_->getNextToken();

        // 解析命令列表
        Node* node = parseCommandList();

        // 检查是否解析到了文件末尾
        if (current_token_.type != TokenType::END) {
            std::cerr << "解析错误：意外的Token：" << current_token_.value << std::endl;
            delete node;
            return nullptr;
        }

        return node;
    }

    void Parser::eat(TokenType token_type)
    {
        if (current_token_.type == token_type) {
            current_token_ = lexer_->getNextToken();
        } else {
            std::cerr << "解析错误：期望 " << tokenTypeToString(token_type)
                      << "，但得到 " << tokenTypeToString(current_token_.type)
                      << " (" << current_token_.value << ")" << std::endl;
        }
    }

    std::string Parser::tokenTypeToString(TokenType type)
    {
        switch (type) {
        case TokenType::WORD: return "WORD";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::PIPE: return "PIPE";
        case TokenType::BACKGROUND: return "BACKGROUND";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::LESS: return "LESS";
        case TokenType::GREAT: return "GREAT";
        case TokenType::DGREAT: return "DGREAT";
        case TokenType::LESSAMP: return "LESSAMP";
        case TokenType::GREATAMP: return "GREATAMP";
        case TokenType::DGREATAMP: return "DGREATAMP";
        case TokenType::CLOBBER: return "CLOBBER";
        case TokenType::HEREDOC: return "HEREDOC";
        case TokenType::HEREDOC_DASH: return "HEREDOC_DASH";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::ELIF: return "ELIF";
        case TokenType::FI: return "FI";
        case TokenType::FOR: return "FOR";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::DONE: return "DONE";
        case TokenType::CASE: return "CASE";
        case TokenType::ESAC: return "ESAC";
        case TokenType::IN: return "IN";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::TIME: return "TIME";
        case TokenType::DQUOTED: return "DQUOTED";
        case TokenType::SQUOTED: return "SQUOTED";
        case TokenType::BQUOTED: return "BQUOTED";
        case TokenType::PARAMETER: return "PARAMETER";
        case TokenType::COMMAND: return "COMMAND";
        case TokenType::END: return "END";
        case TokenType::UNKNOWN: return "UNKNOWN";
        case TokenType::NONE: return "NONE";
        default: return "未知类型";
        }
    }

    Node* Parser::parseCommandList()
    {
        // 解析第一个命令
        Node* node = parseCommand();
        if (!node) {
            return nullptr;
        }

        // 检查是否有更多命令
        while (current_token_.type == TokenType::SEMICOLON || 
               current_token_.type == TokenType::NEWLINE) {
            eat(current_token_.type);

            // 跳过连续的分号和换行符
            while (current_token_.type == TokenType::SEMICOLON || 
                  current_token_.type == TokenType::NEWLINE) {
                eat(current_token_.type);
            }

            // 如果到达文件末尾，返回当前节点
            if (current_token_.type == TokenType::END) {
                break;
            }

            // 解析下一个命令
            Node* right = parseCommand();
            if (!right) {
                delete node;
                return nullptr;
            }

            // 创建序列节点
            Node* sequence = new Node(NodeType::SEQUENCE);
            sequence->setLeftChild(node);
            sequence->setRightChild(right);
            node = sequence;
        }

        return node;
    }

    Node* Parser::parseCommand()
    {
        // 检查是否是后台命令
        Node* command = parsePipeline();
        if (!command) {
            return nullptr;
        }

        if (current_token_.type == TokenType::BACKGROUND) {
            eat(TokenType::BACKGROUND);
            Node* bg = new Node(NodeType::BACKGROUND);
            bg->setChild(command);
            return bg;
        }

        return command;
    }

    Node* Parser::parsePipeline()
    {
        // 解析第一个简单命令
        Node* node = parseSimpleCommand();
        if (!node) {
            return nullptr;
        }

        // 检查是否有管道
        while (current_token_.type == TokenType::PIPE) {
            eat(TokenType::PIPE);

            // 解析下一个简单命令
            Node* right = parseSimpleCommand();
            if (!right) {
                delete node;
                return nullptr;
            }

            // 创建管道节点
            Node* pipe = new Node(NodeType::PIPE);
            pipe->setLeftChild(node);
            pipe->setRightChild(right);
            node = pipe;
        }

        return node;
    }

    Node* Parser::parseSimpleCommand()
    {
        // 创建命令节点
        Node* command = new Node(NodeType::COMMAND);

        // 解析命令名和参数
        if (current_token_.type == TokenType::WORD || 
            current_token_.type == TokenType::DQUOTED || 
            current_token_.type == TokenType::SQUOTED || 
            current_token_.type == TokenType::PARAMETER || 
            current_token_.type == TokenType::COMMAND || 
            current_token_.type == TokenType::BQUOTED) {
            
            // 添加命令名
            command->addArg(current_token_.value);
            eat(current_token_.type);

            // 解析参数
            while (current_token_.type == TokenType::WORD || 
                  current_token_.type == TokenType::DQUOTED || 
                  current_token_.type == TokenType::SQUOTED || 
                  current_token_.type == TokenType::PARAMETER || 
                  current_token_.type == TokenType::COMMAND || 
                  current_token_.type == TokenType::BQUOTED) {
                
                command->addArg(current_token_.value);
                eat(current_token_.type);
            }
        } else if (isRedirectionToken(current_token_.type)) {
            // 只有重定向，没有命令名
        } else {
            delete command;
            return nullptr;
        }

        // 解析重定向
        while (isRedirectionToken(current_token_.type)) {
            Redirection redirection;
            
            // 设置重定向类型
            switch (current_token_.type) {
            case TokenType::LESS:
                redirection.type = RedirectionType::INPUT;
                redirection.fd = 0;
                break;
            case TokenType::GREAT:
                redirection.type = RedirectionType::OUTPUT;
                redirection.fd = 1;
                break;
            case TokenType::DGREAT:
                redirection.type = RedirectionType::APPEND;
                redirection.fd = 1;
                break;
            case TokenType::LESSAMP:
                redirection.type = RedirectionType::DUPLICATE;
                redirection.fd = 0;
                break;
            case TokenType::GREATAMP:
                redirection.type = RedirectionType::DUPLICATE;
                redirection.fd = 1;
                break;
            case TokenType::DGREATAMP:
                redirection.type = RedirectionType::APPEND;
                redirection.fd = 1;
                break;
            case TokenType::CLOBBER:
                redirection.type = RedirectionType::OUTPUT;
                redirection.fd = 1;
                break;
            case TokenType::HEREDOC:
            case TokenType::HEREDOC_DASH:
                redirection.type = RedirectionType::HERE_DOC;
                redirection.fd = 0;
                break;
            default:
                break;
            }

            // 跳过重定向符号
            eat(current_token_.type);

            // 获取重定向目标
            if (current_token_.type == TokenType::WORD || 
                current_token_.type == TokenType::DQUOTED || 
                current_token_.type == TokenType::SQUOTED) {
                
                redirection.filename = current_token_.value;
                eat(current_token_.type);
            } else {
                std::cerr << "解析错误：重定向后缺少文件名" << std::endl;
                delete command;
                return nullptr;
            }

            // 添加重定向到命令
            command->addRedirection(redirection);
        }

        return command;
    }

    bool Parser::isRedirectionToken(TokenType type)
    {
        return type == TokenType::LESS || 
               type == TokenType::GREAT || 
               type == TokenType::DGREAT || 
               type == TokenType::LESSAMP || 
               type == TokenType::GREATAMP || 
               type == TokenType::DGREATAMP || 
               type == TokenType::CLOBBER || 
               type == TokenType::HEREDOC || 
               type == TokenType::HEREDOC_DASH;
    }

    Node* Parser::parseIf()
    {
        // 创建IF节点
        Node* ifNode = new Node(NodeType::IF);

        // 解析IF关键字
        eat(TokenType::IF);

        // 解析条件
        Node* condition = parseCommandList();
        if (!condition) {
            delete ifNode;
            return nullptr;
        }
        ifNode->setCondition(condition);

        // 解析THEN关键字
        if (current_token_.type != TokenType::THEN) {
            std::cerr << "解析错误：缺少THEN关键字" << std::endl;
            delete ifNode;
            return nullptr;
        }
        eat(TokenType::THEN);

        // 解析THEN分支
        Node* thenBranch = parseCommandList();
        if (!thenBranch) {
            delete ifNode;
            return nullptr;
        }
        ifNode->setThenBranch(thenBranch);

        // 解析ELSE分支（如果有）
        if (current_token_.type == TokenType::ELSE) {
            eat(TokenType::ELSE);
            
            Node* elseBranch = parseCommandList();
            if (!elseBranch) {
                delete ifNode;
                return nullptr;
            }
            ifNode->setElseBranch(elseBranch);
        }

        // 解析FI关键字
        if (current_token_.type != TokenType::FI) {
            std::cerr << "解析错误：缺少FI关键字" << std::endl;
            delete ifNode;
            return nullptr;
        }
        eat(TokenType::FI);

        return ifNode;
    }

    Node* Parser::parseWhile()
    {
        // 创建WHILE节点
        Node* whileNode = new Node(NodeType::WHILE);

        // 解析WHILE关键字
        eat(TokenType::WHILE);

        // 解析条件
        Node* condition = parseCommandList();
        if (!condition) {
            delete whileNode;
            return nullptr;
        }
        whileNode->setCondition(condition);

        // 解析DO关键字
        if (current_token_.type != TokenType::DO) {
            std::cerr << "解析错误：缺少DO关键字" << std::endl;
            delete whileNode;
            return nullptr;
        }
        eat(TokenType::DO);

        // 解析循环体
        Node* body = parseCommandList();
        if (!body) {
            delete whileNode;
            return nullptr;
        }
        whileNode->setBody(body);

        // 解析DONE关键字
        if (current_token_.type != TokenType::DONE) {
            std::cerr << "解析错误：缺少DONE关键字" << std::endl;
            delete whileNode;
            return nullptr;
        }
        eat(TokenType::DONE);

        return whileNode;
    }

    Node* Parser::parseFor()
    {
        // 创建FOR节点
        Node* forNode = new Node(NodeType::FOR);

        // 解析FOR关键字
        eat(TokenType::FOR);

        // 解析变量名
        if (current_token_.type != TokenType::WORD) {
            std::cerr << "解析错误：FOR循环缺少变量名" << std::endl;
            delete forNode;
            return nullptr;
        }
        
        forNode->setVarName(current_token_.value);
        eat(TokenType::WORD);

        // 解析IN关键字
        if (current_token_.type != TokenType::IN) {
            std::cerr << "解析错误：缺少IN关键字" << std::endl;
            delete forNode;
            return nullptr;
        }
        eat(TokenType::IN);

        // 解析值列表
        while (current_token_.type == TokenType::WORD || 
              current_token_.type == TokenType::DQUOTED || 
              current_token_.type == TokenType::SQUOTED || 
              current_token_.type == TokenType::PARAMETER || 
              current_token_.type == TokenType::COMMAND || 
              current_token_.type == TokenType::BQUOTED) {
            
            forNode->addValue(current_token_.value);
            eat(current_token_.type);
        }

        // 解析DO关键字
        if (current_token_.type != TokenType::DO) {
            std::cerr << "解析错误：缺少DO关键字" << std::endl;
            delete forNode;
            return nullptr;
        }
        eat(TokenType::DO);

        // 解析循环体
        Node* body = parseCommandList();
        if (!body) {
            delete forNode;
            return nullptr;
        }
        forNode->setBody(body);

        // 解析DONE关键字
        if (current_token_.type != TokenType::DONE) {
            std::cerr << "解析错误：缺少DONE关键字" << std::endl;
            delete forNode;
            return nullptr;
        }
        eat(TokenType::DONE);

        return forNode;
    }

    Node* Parser::parseCase()
    {
        // 创建CASE节点
        Node* caseNode = new Node(NodeType::CASE);

        // 解析CASE关键字
        eat(TokenType::CASE);

        // 解析word
        if (current_token_.type != TokenType::WORD && 
            current_token_.type != TokenType::DQUOTED && 
            current_token_.type != TokenType::SQUOTED && 
            current_token_.type != TokenType::PARAMETER) {
            std::cerr << "解析错误：CASE语句缺少word" << std::endl;
            delete caseNode;
            return nullptr;
        }
        
        caseNode->setWord(current_token_.value);
        eat(current_token_.type);

        // 解析IN关键字
        if (current_token_.type != TokenType::IN) {
            std::cerr << "解析错误：缺少IN关键字" << std::endl;
            delete caseNode;
            return nullptr;
        }
        eat(TokenType::IN);

        // 解析case项
        while (current_token_.type != TokenType::ESAC) {
            // 跳过换行和分号
            while (current_token_.type == TokenType::NEWLINE || 
                  current_token_.type == TokenType::SEMICOLON) {
                eat(current_token_.type);
            }
            
            // 如果到达ESAC，退出循环
            if (current_token_.type == TokenType::ESAC) {
                break;
            }

            // 解析模式列表
            std::vector<std::string> patterns;
            
            do {
                if (current_token_.type != TokenType::WORD && 
                    current_token_.type != TokenType::DQUOTED && 
                    current_token_.type != TokenType::SQUOTED && 
                    current_token_.type != TokenType::PARAMETER) {
                    std::cerr << "解析错误：CASE模式必须是单词或字符串" << std::endl;
                    delete caseNode;
                    return nullptr;
                }
                
                patterns.push_back(current_token_.value);
                eat(current_token_.type);
                
                // 如果是|，继续解析下一个模式
                if (current_token_.type == TokenType::PIPE) {
                    eat(TokenType::PIPE);
                } else {
                    break;
                }
            } while (true);

            // 解析)
            if (current_token_.type != TokenType::RPAREN) {
                std::cerr << "解析错误：缺少右括号" << std::endl;
                delete caseNode;
                return nullptr;
            }
            eat(TokenType::RPAREN);

            // 解析命令列表
            Node* actions = parseCommandList();
            
            // 解析;;
            if (current_token_.type != TokenType::SEMICOLON || lexer_->peekNextToken().type != TokenType::SEMICOLON) {
                std::cerr << "解析错误：缺少;;" << std::endl;
                delete caseNode;
                if (actions) {
                    delete actions;
                }
                return nullptr;
            }
            eat(TokenType::SEMICOLON);
            eat(TokenType::SEMICOLON);

            // 添加case项
            caseNode->addCase(patterns, actions);
        }

        // 解析ESAC关键字
        if (current_token_.type != TokenType::ESAC) {
            std::cerr << "解析错误：缺少ESAC关键字" << std::endl;
            delete caseNode;
            return nullptr;
        }
        eat(TokenType::ESAC);

        return caseNode;
    }

    Node* Parser::parseSubshell()
    {
        // 创建子shell节点
        Node* subshell = new Node(NodeType::SUBSHELL);

        // 解析左括号
        eat(TokenType::LPAREN);

        // 解析命令列表
        Node* commands = parseCommandList();
        if (!commands) {
            delete subshell;
            return nullptr;
        }
        subshell->setChild(commands);

        // 解析右括号
        if (current_token_.type != TokenType::RPAREN) {
            std::cerr << "解析错误：缺少右括号" << std::endl;
            delete subshell;
            return nullptr;
        }
        eat(TokenType::RPAREN);

        return subshell;
    }

    Node* Parser::parseGroup()
    {
        // 创建命令组节点
        Node* group = new Node(NodeType::GROUP);

        // 解析左花括号
        eat(TokenType::LBRACE);

        // 解析命令列表
        Node* commands = parseCommandList();
        if (!commands) {
            delete group;
            return nullptr;
        }
        group->setChild(commands);

        // 解析右花括号
        if (current_token_.type != TokenType::RBRACE) {
            std::cerr << "解析错误：缺少右花括号" << std::endl;
            delete group;
            return nullptr;
        }
        eat(TokenType::RBRACE);

        return group;
    }

} // namespace dash 