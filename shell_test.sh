#!/bin/sh
# shell_test.sh - 全面测试dash-cpp shell功能

echo "======== Dash-CPP Shell 功能测试 ========"

# 测试部分1: 基本命令和内置命令
echo "\n===== 测试基本命令和内置命令 ====="

echo ">> 测试 echo 命令"
echo "Hello World"

echo ">> 测试 pwd 命令"
pwd

echo ">> 测试 cd 命令"
echo "当前目录:"
pwd
echo "切换到上级目录:"
cd ..
pwd
echo "切换回原目录:"
cd -
pwd

echo ">> 测试 help 命令"
help

# 测试部分2: 变量操作
echo "\n===== 测试变量操作 ====="

echo ">> 测试变量赋值和展开"
TEST_VAR="Hello Variable"
echo "TEST_VAR = $TEST_VAR"

echo ">> 测试环境变量"
echo "PATH = $PATH"
echo "HOME = $HOME"

# 测试部分3: 重定向
echo "\n===== 测试重定向 ====="

echo ">> 测试输出重定向 >"
echo "This is a test file" > test_output.txt
echo "查看文件内容:"
cat test_output.txt

echo ">> 测试追加重定向 >>"
echo "This is appended text" >> test_output.txt
echo "查看文件内容:"
cat test_output.txt

echo ">> 测试输入重定向 <"
cat < test_output.txt

echo ">> 测试错误重定向 2>"
ls /nonexistent 2> error.txt
echo "错误输出内容:"
cat error.txt

# 测试部分4: 管道
echo "\n===== 测试管道 ====="

echo ">> 测试简单管道"
ls -la | grep "test"

echo ">> 测试多级管道"
ls -la | grep "test" | wc -l

# 测试部分5: 作业控制
echo "\n===== 测试作业控制 ====="

echo ">> 等待1秒"
sleep 1

# 测试部分6: 条件执行
echo "\n===== 测试条件执行 ====="

echo ">> 测试 && 操作符"
echo "第一个命令" && echo "第二个命令"

echo ">> 测试 || 操作符"
false || echo "前一个命令失败了"
true || echo "这不应该被打印"

# 测试部分7: source命令
echo "\n===== 测试source命令 ====="

echo ">> 创建一个测试脚本"
cat > source_test_temp.sh << 'EOF'
#!/bin/sh
echo "这是通过source执行的脚本"
SOURCE_TEST_VAR="source成功"
EOF

echo ">> 执行source命令"
source source_test_temp.sh
echo "SOURCE_TEST_VAR = $SOURCE_TEST_VAR"

# 测试部分8: 清理
echo "\n===== 清理测试文件 ====="
rm -f test_output.txt error.txt source_test_temp.sh

echo "\n======== 测试完成 ========" 