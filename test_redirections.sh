#!/bin/sh
# test_redirections.sh - 测试重定向功能

echo "======== 测试重定向功能 ========"

# 测试标准输出重定向
echo ">> 测试标准输出重定向 >"
echo "这是标准输出重定向测试" > stdout.txt
echo "文件内容:"
cat stdout.txt

# 测试标准输出追加重定向
echo ">> 测试标准输出追加重定向 >>"
echo "这是第一行" > append.txt
echo "这是追加的第二行" >> append.txt
echo "文件内容:"
cat append.txt

# 测试标准输入重定向
echo ">> 测试标准输入重定向 <"
echo "这是输入文件内容" > input.txt
cat < input.txt

# 测试标准错误重定向
echo ">> 测试标准错误重定向 2>"
ls /nonexistent 2> stderr.txt
echo "错误输出:"
cat stderr.txt

# 测试标准输出和标准错误分别重定向
echo ">> 测试标准输出和标准错误分别重定向 > 2>"
ls -la . /nonexistent > stdout_err.txt 2> stderr_err.txt
echo "标准输出:"
cat stdout_err.txt
echo "标准错误:"
cat stderr_err.txt

# 测试标准输出和标准错误合并重定向
echo ">> 测试标准输出和标准错误合并重定向 &>"
ls -la . /nonexistent &> combined.txt
echo "合并输出:"
cat combined.txt

# 清理
echo ">> 清理文件"
rm -f stdout.txt append.txt input.txt stderr.txt stdout_err.txt stderr_err.txt combined.txt

echo "======== 测试完成 ========" 