/**
 * @file parser.cpp
 * @brief 解析器类实现
 */

#include <iostream>
#include <unordered_set>
#include "core/parser.h"
#include "core/shell.h"
#include "core/node.h"
#include "core/input.h"
#include "utils/error.h"

namespace dash
{

    // 保留字集合
    static const std::unordered_set<std::string> reserved_words = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while",
        "until", "do", "done", "in", "{", "}", "!", "[[", "]]"};

    Parser::Parser(Shell *shell)
        : shell_(shell), lexer_(std::make_unique<Lexer>(shell))
    {
    }

    Parser::~Parser()
    {
    }

    void Parser::setInput(const std::string &input)
    {
        lexer_->setInput(input);
    }

    std::unique_ptr<Node> Parser::parseCommand(bool interactive)
    {
        try
        {
            // 获取输入行
            std::string line;
            if (interactive)
            {
                // 从 shell 的输入处理器获取一行输入
                line = shell_->getInput()->readLine(true);
                if (line.empty())
                {
                    return nullptr; // EOF
                }

                // 设置词法分析器的输入
                lexer_->setInput(line);
            }

            // 解析命令列表
            skipNewlines();
            std::unique_ptr<Node> node = parseList();

            // 检查是否有多余的词法单元
            std::unique_ptr<Token> token = lexer_->nextToken();
            if (token->getType() != TokenType::END_OF_INPUT)
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: unexpected token '" + token->getValue() + "'");
            }

            return node;
        }
        catch (const ShellException &e)
        {
            // 重新抛出异常
            throw;
        }
        catch (const std::exception &e)
        {
            // 将标准异常转换为 ShellException
            throw ShellException(ExceptionType::SYNTAX, std::string("Parser error: ") + e.what());
        }
    }

    std::unique_ptr<Node> Parser::parseList()
    {
        // 创建列表节点
        auto list = std::make_unique<ListNode>();

        // 解析第一个命令
        auto command = parsePipeline();
        if (!command)
        {
            return nullptr;
        }

        list->addCommand(std::move(command));

        // 解析后续命令
        while (true)
        {
            // 查看下一个词法单元
            const Token *token = lexer_->peekToken();

            // 如果是分号或换行符，跳过并继续解析
            if (token->getType() == TokenType::OPERATOR && token->getValue() == ";")
            {
                lexer_->nextToken(); // 消耗分号
                skipNewlines();

                // 解析下一个命令
                command = parsePipeline();
                if (command)
                {
                    list->addCommand(std::move(command), ";");
                }
            }
            // 如果是 && 或 ||，继续解析
            else if (token->getType() == TokenType::OPERATOR &&
                     (token->getValue() == "&&" || token->getValue() == "||"))
            {
                std::string op = token->getValue();
                lexer_->nextToken(); // 消耗操作符
                skipNewlines();

                // 解析下一个命令
                command = parsePipeline();
                if (!command)
                {
                    throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected command after '" + op + "'");
                }

                list->addCommand(std::move(command), op);
            }
            // 如果是其他词法单元，结束解析
            else
            {
                break;
            }
        }

        // 如果列表中只有一个命令，直接返回该命令
        if (list->getCommands().size() == 1 && list->getOperators().empty())
        {
            // 由于 getCommands() 返回的是 const std::vector<std::unique_ptr<Node>>&，
            // 我们不能直接移动其中的元素。
            // 因此，我们创建一个新的节点并返回整个列表节点。
            return list;
        }

        return list;
    }

    std::unique_ptr<Node> Parser::parsePipeline()
    {
        // 检查是否是后台命令
        bool background = false;

        // 解析第一个命令
        auto command = parseSimpleCommand();
        if (!command)
        {
            return nullptr;
        }

        // 查看下一个词法单元
        const Token *token = lexer_->peekToken();

        // 如果是管道符，继续解析
        if (token->getType() == TokenType::OPERATOR && token->getValue() == "|")
        {
            lexer_->nextToken(); // 消耗管道符
            skipNewlines();

            // 解析右侧命令
            auto right = parsePipeline();
            if (!right)
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected command after '|'");
            }

            // 创建管道节点
            return std::make_unique<PipeNode>(std::move(command), std::move(right), background);
        }

        // 检查是否是后台运行
        if (token->getType() == TokenType::OPERATOR && token->getValue() == "&")
        {
            lexer_->nextToken(); // 消耗 &
            background = true;

            // 不论命令类型如何，都创建一个PipeNode并设置后台标志
            return std::make_unique<PipeNode>(std::move(command), nullptr, background);
        }

        return command;
    }

    std::unique_ptr<Node> Parser::parseSimpleCommand()
    {
        // 跳过空白和换行符
        skipNewlines();

        // 查看下一个词法单元
        const Token *token = lexer_->peekToken();

        // 如果是 EOF，返回空
        if (token->getType() == TokenType::END_OF_INPUT)
        {
            return nullptr;
        }

        // 检查是否是保留字
        if (token->getType() == TokenType::WORD)
        {
            std::string word = token->getValue();

            // 处理特殊命令结构
            if (word == "if")
            {
                return parseIf();
            }
            else if (word == "for")
            {
                return parseFor();
            }
            else if (word == "while")
            {
                return parseWhile(false);
            }
            else if (word == "until")
            {
                return parseWhile(true);
            }
            else if (word == "case")
            {
                return parseCase();
            }
            else if (word == "(")
            {
                return parseSubshell();
            }
        }

        // 创建命令节点
        auto command = std::make_unique<CommandNode>();

        // 解析变量赋值和参数
        bool first_arg = true;

        while (true)
        {
            // 查看下一个词法单元
            token = lexer_->peekToken();

            // 如果是 EOF 或不是单词，结束解析
            if (token->getType() == TokenType::END_OF_INPUT ||
                (token->getType() != TokenType::WORD && token->getType() != TokenType::ASSIGNMENT))
            {
                break;
            }

            // 处理变量赋值
            if (token->getType() == TokenType::ASSIGNMENT)
            {
                if (!first_arg)
                {
                    // 变量赋值只能出现在命令前面
                    break;
                }

                command->addAssignment(token->getValue());
                lexer_->nextToken(); // 消耗赋值词法单元
                continue;
            }

            // 处理普通参数
            command->addArg(token->getValue());
            lexer_->nextToken(); // 消耗单词词法单元
            first_arg = false;

            // 解析重定向
            while (parseRedirection(command.get()))
            {
                // 继续解析重定向
            }
        }

        // 如果没有参数，检查是否只有变量赋值
        if (command->getArgs().empty() && !command->getAssignments().empty())
        {
            // 只有变量赋值的命令是有效的
            return command;
        }

        // 如果没有参数也没有变量赋值，返回空
        if (command->getArgs().empty() && command->getAssignments().empty())
        {
            return nullptr;
        }

        return command;
    }

    bool Parser::parseRedirection(Node *node)
    {
        // 查看下一个词法单元
        const Token *token = lexer_->peekToken();

        // 检查是否是 IO 编号
        int fd = -1;
        if (token->getType() == TokenType::IO_NUMBER)
        {
            fd = std::stoi(token->getValue());
            lexer_->nextToken(); // 消耗 IO 编号
            token = lexer_->peekToken();
        }

        // 检查是否是重定向操作符
        if (token->getType() != TokenType::OPERATOR || !isRedirectionOperator(token))
        {
            // 如果已经消耗了 IO 编号但后面不是重定向操作符，报错
            if (fd != -1)
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected redirection operator after IO number");
            }
            return false;
        }

        // 获取重定向类型
        std::string op = token->getValue();
        RedirType type;

        if (op == "<")
        {
            type = RedirType::REDIR_INPUT;
            fd = (fd == -1) ? 0 : fd;
        }
        else if (op == ">")
        {
            type = RedirType::REDIR_OUTPUT;
            fd = (fd == -1) ? 1 : fd;
        }
        else if (op == ">>")
        {
            type = RedirType::REDIR_APPEND;
            fd = (fd == -1) ? 1 : fd;
        }
        else if (op == "<&")
        {
            type = RedirType::REDIR_INPUT_DUP;
            fd = (fd == -1) ? 0 : fd;
        }
        else if (op == ">&")
        {
            type = RedirType::REDIR_OUTPUT_DUP;
            fd = (fd == -1) ? 1 : fd;
        }
        else if (op == "<<")
        {
            type = RedirType::REDIR_HEREDOC;
            fd = (fd == -1) ? 0 : fd;
        }
        else
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: unknown redirection operator '" + op + "'");
        }

        lexer_->nextToken(); // 消耗重定向操作符

        // 获取文件名或目标
        token = lexer_->peekToken();
        if (token->getType() != TokenType::WORD)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected word after redirection operator");
        }

        std::string filename = token->getValue();
        lexer_->nextToken(); // 消耗文件名

        // 创建重定向
        Redirection redir(type, fd, filename);

        // 添加重定向到节点
        if (node->getType() == NodeType::COMMAND)
        {
            static_cast<CommandNode *>(node)->addRedirection(redir);
        }
        else if (node->getType() == NodeType::SUBSHELL)
        {
            static_cast<SubshellNode *>(node)->addRedirection(redir);
        }
        else
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: redirection not allowed here");
        }

        return true;
    }

    std::unique_ptr<Token> Parser::expectToken(TokenType type, const std::string &error_message)
    {
        std::unique_ptr<Token> token = lexer_->nextToken();
        if (token->getType() != type)
        {
            throw ShellException(ExceptionType::SYNTAX, error_message);
        }
        return token;
    }

    void Parser::skipNewlines()
    {
        while (true)
        {
            const Token *token = lexer_->peekToken();
            if (token->getType() != TokenType::NEWLINE)
            {
                break;
            }
            lexer_->nextToken(); // 消耗换行符
        }
    }

    bool Parser::isReservedWord(const std::string &word) const
    {
        return reserved_words.find(word) != reserved_words.end();
    }

    bool Parser::isRedirectionOperator(const Token *token) const
    {
        if (token->getType() != TokenType::OPERATOR)
        {
            return false;
        }

        const std::string &op = token->getValue();
        return op == "<" || op == ">" || op == ">>" || op == "<&" || op == ">&" || op == "<<";
    }

    // 以下是复杂控制结构的解析函数，暂时只提供基本实现

    std::unique_ptr<Node> Parser::parseIf()
    {
        // 消耗 if 关键字
        expectToken(TokenType::WORD, "Syntax error: expected 'if'");

        // 解析条件
        auto condition = parseList();
        if (!condition)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected condition after 'if'");
        }

        // 期望 then 关键字
        auto token = expectToken(TokenType::WORD, "Syntax error: expected 'then' after condition");
        if (token->getValue() != "then")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'then' after condition");
        }

        // 解析 then 部分
        auto then_part = parseList();
        if (!then_part)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands after 'then'");
        }

        // 检查是否有 else 部分
        std::unique_ptr<Node> else_part = nullptr;
        const Token* peek_token = lexer_->peekToken();

        if (peek_token->getType() == TokenType::WORD && peek_token->getValue() == "else")
        {
            lexer_->nextToken(); // 消耗 else 关键字

            // 解析 else 部分
            else_part = parseList();
            if (!else_part)
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands after 'else'");
            }
        }

        // 期望 fi 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'fi' to end if statement");
        if (token->getValue() != "fi")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'fi' to end if statement");
        }

        // 创建 if 节点
        return std::make_unique<IfNode>(std::move(condition), std::move(then_part), std::move(else_part));
    }

    std::unique_ptr<Node> Parser::parseFor()
    {
        // 这里只提供一个基本实现，完整实现需要处理更多情况

        // 消耗 for 关键字
        expectToken(TokenType::WORD, "Syntax error: expected 'for'");

        // 获取循环变量
        auto token = expectToken(TokenType::WORD, "Syntax error: expected variable name after 'for'");
        std::string var = token->getValue();

        // 期望 in 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'in' after variable name");
        if (token->getValue() != "in")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'in' after variable name");
        }

        // 收集单词列表
        std::vector<std::string> words;
        while (true)
        {
            const Token* peek_token = lexer_->peekToken();
            if (peek_token->getType() == TokenType::WORD && peek_token->getValue() != "do")
            {
                words.push_back(peek_token->getValue());
                lexer_->nextToken(); // 消耗单词
            }
            else
            {
                break;
            }
        }

        // 期望 do 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'do' after word list");
        if (token->getValue() != "do")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'do' after word list");
        }

        // 解析循环体
        auto body = parseList();
        if (!body)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands after 'do'");
        }

        // 期望 done 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'done' to end for loop");
        if (token->getValue() != "done")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'done' to end for loop");
        }

        // 创建 for 节点
        return std::make_unique<ForNode>(var, words, std::move(body));
    }

    std::unique_ptr<Node> Parser::parseWhile(bool until)
    {
        // 消耗 while/until 关键字
        expectToken(TokenType::WORD, until ? "Syntax error: expected 'until'" : "Syntax error: expected 'while'");

        // 解析条件
        auto condition = parseList();
        if (!condition)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected condition after 'while'/'until'");
        }

        // 期望 do 关键字
        auto token = expectToken(TokenType::WORD, "Syntax error: expected 'do' after condition");
        if (token->getValue() != "do")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'do' after condition");
        }

        // 解析循环体
        auto body = parseList();
        if (!body)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands after 'do'");
        }

        // 期望 done 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'done' to end while/until loop");
        if (token->getValue() != "done")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'done' to end while/until loop");
        }

        // 创建 while 节点
        return std::make_unique<WhileNode>(std::move(condition), std::move(body), until);
    }

    std::unique_ptr<Node> Parser::parseCase()
    {
        // 这里只提供一个基本实现，完整实现需要处理更多情况

        // 消耗 case 关键字
        expectToken(TokenType::WORD, "Syntax error: expected 'case'");

        // 获取匹配词
        auto token = expectToken(TokenType::WORD, "Syntax error: expected word after 'case'");
        std::string word = token->getValue();

        // 期望 in 关键字
        token = expectToken(TokenType::WORD, "Syntax error: expected 'in' after word");
        if (token->getValue() != "in")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected 'in' after word");
        }

        // 创建 case 节点
        auto case_node = std::make_unique<CaseNode>(word);

        // 解析 case 项
        while (true)
        {
            skipNewlines();

            // 检查是否是 esac
            const Token* peek_token = lexer_->peekToken();
            if (peek_token->getType() == TokenType::WORD && peek_token->getValue() == "esac")
            {
                lexer_->nextToken(); // 消耗 esac
                break;
            }

            // 收集模式
            std::vector<std::string> patterns;
            while (true)
            {
                peek_token = lexer_->peekToken();
                if (peek_token->getType() != TokenType::WORD)
                {
                    throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected pattern in case item");
                }

                patterns.push_back(peek_token->getValue());
                lexer_->nextToken(); // 消耗模式

                // 检查是否有更多模式
                peek_token = lexer_->peekToken();
                if (peek_token->getType() == TokenType::OPERATOR && peek_token->getValue() == "|")
                {
                    lexer_->nextToken(); // 消耗 |
                    continue;
                }

                break;
            }

            // 期望 ) 操作符
            peek_token = lexer_->peekToken();
            if (peek_token->getType() != TokenType::OPERATOR || peek_token->getValue() != ")")
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected ')' after pattern");
            }
            lexer_->nextToken(); // 消耗 )

            // 解析命令
            auto commands = parseList();
            if (!commands)
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands in case item");
            }

            // 期望 ;; 操作符
            peek_token = lexer_->peekToken();
            if (peek_token->getType() != TokenType::OPERATOR || peek_token->getValue() != ";;")
            {
                throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected ';;' after case item");
            }
            lexer_->nextToken(); // 消耗 ;;

            // 添加 case 项
            case_node->addItem(patterns, std::move(commands));
        }

        return case_node;
    }

    std::unique_ptr<Node> Parser::parseSubshell()
    {
        // 消耗 ( 操作符
        expectToken(TokenType::OPERATOR, "Syntax error: expected '('");

        // 解析命令
        auto commands = parseList();
        if (!commands)
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected commands in subshell");
        }

        // 期望 ) 操作符
        auto token = expectToken(TokenType::OPERATOR, "Syntax error: expected ')' to end subshell");
        if (token->getValue() != ")")
        {
            throw ShellException(ExceptionType::SYNTAX, "Syntax error: expected ')' to end subshell");
        }

        // 创建子 shell 节点
        auto subshell = std::make_unique<SubshellNode>(std::move(commands));

        // 解析重定向
        while (parseRedirection(subshell.get()))
        {
            // 继续解析重定向
        }

        return subshell;
    }

} // namespace dash