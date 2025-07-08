/**
 * @file lexer_test.cpp
 * @brief 词法分析器的单元测试
 */

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "core/lexer.h"
#include "core/shell.h"
#include "utils/error.h"

using namespace dash;

// 测试辅助类，用于存储和检查词法单元序列
class TokenSequence
{
public:
    TokenSequence() = default;

    void addToken(TokenType type, const std::string &value)
    {
        tokens_.push_back({type, value});
    }

    bool match(Lexer &lexer)
    {
        for (const auto &expected : tokens_)
        {
            auto token = lexer.nextToken();
            if (!token || token->getType() != expected.first || token->getValue() != expected.second)
            {
                return false;
            }
        }

        // 确保没有多余的词法单元
        auto extra = lexer.nextToken();
        return extra && extra->getType() == TokenType::END_OF_INPUT;
    }

private:
    std::vector<std::pair<TokenType, std::string>> tokens_;
};

// 词法分析器测试夹具
class LexerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        shell_ = std::make_unique<Shell>();
        lexer_ = std::make_unique<Lexer>(shell_.get());
    }

    std::unique_ptr<Shell> shell_;
    std::unique_ptr<Lexer> lexer_;
};

// 测试简单命令
TEST_F(LexerTest, SimpleCommand)
{
    lexer_->setInput("echo hello world");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "echo");
    expected.addToken(TokenType::WORD, "hello");
    expected.addToken(TokenType::WORD, "world");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试管道
TEST_F(LexerTest, Pipeline)
{
    lexer_->setInput("ls -l | grep foo | wc -l");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "ls");
    expected.addToken(TokenType::WORD, "-l");
    expected.addToken(TokenType::OPERATOR, "|");
    expected.addToken(TokenType::WORD, "grep");
    expected.addToken(TokenType::WORD, "foo");
    expected.addToken(TokenType::OPERATOR, "|");
    expected.addToken(TokenType::WORD, "wc");
    expected.addToken(TokenType::WORD, "-l");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试重定向
TEST_F(LexerTest, Redirection)
{
    lexer_->setInput("cat file.txt > output.txt 2> error.log");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "cat");
    expected.addToken(TokenType::WORD, "file.txt");
    expected.addToken(TokenType::OPERATOR, ">");
    expected.addToken(TokenType::WORD, "output.txt");
    expected.addToken(TokenType::IO_NUMBER, "2");
    expected.addToken(TokenType::OPERATOR, ">");
    expected.addToken(TokenType::WORD, "error.log");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试变量赋值
TEST_F(LexerTest, VariableAssignment)
{
    lexer_->setInput("VAR=value command arg");

    TokenSequence expected;
    expected.addToken(TokenType::ASSIGNMENT, "VAR=value");
    expected.addToken(TokenType::WORD, "command");
    expected.addToken(TokenType::WORD, "arg");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试引号
TEST_F(LexerTest, Quotes)
{
    lexer_->setInput("echo \"Hello, world!\" 'Single quotes'");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "echo");
    expected.addToken(TokenType::WORD, "\"Hello, world!\"");
    expected.addToken(TokenType::WORD, "'Single quotes'");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试注释
TEST_F(LexerTest, Comments)
{
    lexer_->setInput("echo hello # This is a comment\necho world");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "echo");
    expected.addToken(TokenType::WORD, "hello");
    expected.addToken(TokenType::NEWLINE, "\n");
    expected.addToken(TokenType::WORD, "echo");
    expected.addToken(TokenType::WORD, "world");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试复杂操作符
TEST_F(LexerTest, ComplexOperators)
{
    lexer_->setInput("cmd1 && cmd2 || cmd3");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "cmd1");
    expected.addToken(TokenType::OPERATOR, "&&");
    expected.addToken(TokenType::WORD, "cmd2");
    expected.addToken(TokenType::OPERATOR, "||");
    expected.addToken(TokenType::WORD, "cmd3");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试转义字符
TEST_F(LexerTest, EscapeCharacters)
{
    lexer_->setInput("echo \"Hello\\\"World\"");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "echo");
    expected.addToken(TokenType::WORD, "\"Hello\\\"World\"");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试多行输入
TEST_F(LexerTest, MultilineInput)
{
    lexer_->setInput("cmd1\ncmd2\ncmd3");

    TokenSequence expected;
    expected.addToken(TokenType::WORD, "cmd1");
    expected.addToken(TokenType::NEWLINE, "\n");
    expected.addToken(TokenType::WORD, "cmd2");
    expected.addToken(TokenType::NEWLINE, "\n");
    expected.addToken(TokenType::WORD, "cmd3");

    EXPECT_TRUE(expected.match(*lexer_));
}

// 测试未终止的引号
TEST_F(LexerTest, UnterminatedQuote)
{
    lexer_->setInput("echo \"Hello");

    EXPECT_THROW({
        while (lexer_->nextToken()->getType() != TokenType::END_OF_INPUT) {} }, ShellException);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}