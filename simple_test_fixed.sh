#!/bin/sh
# simple_test_fixed.sh - 修复版测试脚本

echo "======== 修复版测试脚本 ========"

# 测试基本命令
echo ">> 测试基本命令"
echo "Hello World"
pwd

# 测试变量
echo ">> 测试变量"
TEST_VAR="测试变量"
echo "TEST_VAR = $TEST_VAR"

# 测试重定向
echo ">> 测试重定向"
echo "重定向测试" > simple_output.txt
cat simple_output.txt

# 测试管道
echo ">> 测试管道"
# 使用echo -e启用转义序列解释，或者直接使用多行输入
echo "line1" > lines.txt
echo "line2" >> lines.txt
echo "line3" >> lines.txt
cat lines.txt | grep "line2"

# 测试source命令
echo ">> 测试source命令"
# 预先创建source脚本文件，而不是使用here-document
echo '#!/bin/sh' > simple_source.sh
echo 'echo "这是从simple_source.sh输出的"' >> simple_source.sh
echo 'SOURCED_VAR="source变量"' >> simple_source.sh

source simple_source.sh
echo "SOURCED_VAR = $SOURCED_VAR"

# 清理
echo ">> 清理文件"
rm -f simple_output.txt simple_source.sh lines.txt

echo "======== 测试完成 ========" 