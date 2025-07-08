#!/bin/sh
# simple_test.sh - 简化版测试脚本，避免使用控制结构

echo "======== 简化版测试脚本 ========"

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
echo "line1\nline2\nline3" | grep "line2"

# 测试source命令
echo ">> 测试source命令"
cat > simple_source.sh << EOF
#!/bin/sh
echo "这是从simple_source.sh输出的"
SOURCED_VAR="source变量"
EOF

source simple_source.sh
echo "SOURCED_VAR = $SOURCED_VAR"

# 清理
echo ">> 清理文件"
rm -f simple_output.txt simple_source.sh

echo "======== 测试完成 ======== 