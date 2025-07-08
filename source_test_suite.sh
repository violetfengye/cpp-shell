#!/bin/sh
# source_test_suite.sh - 专门测试source命令功能

echo "======== Source 命令测试套件 ========"

# 创建测试脚本1 - 基本变量
echo ">> 创建基本变量测试脚本"
cat > source_test1.sh << 'EOF'
#!/bin/sh
# 基本变量测试
echo "执行 source_test1.sh"
TEST_VAR1="变量1"
TEST_VAR2=42
TEST_VAR3="包含空格的变量"
EOF

# 创建测试脚本2 - 函数定义
echo ">> 创建函数定义测试脚本"
cat > source_test2.sh << 'EOF'
#!/bin/sh
# 函数定义测试
echo "执行 source_test2.sh"
test_func1() {
    echo "这是测试函数1"
    echo "参数: $1 $2"
}

test_func2() {
    local local_var="局部变量"
    echo "这是测试函数2"
    echo "局部变量: $local_var"
    return 123
}
EOF

# 创建测试脚本3 - 复杂脚本
echo ">> 创建复杂脚本测试"
cat > source_test3.sh << 'EOF'
#!/bin/sh
# 复杂脚本测试
echo "执行 source_test3.sh"

# 定义数组（如果shell支持）
ITEMS="item1 item2 item3 item4"

# 循环处理
for item in $ITEMS; do
    echo "处理: $item"
done

# 条件判断
if [ -f "source_test1.sh" ]; then
    echo "source_test1.sh 文件存在"
fi

# 命令替换
CURRENT_TIME=$(date)
echo "当前时间: $CURRENT_TIME"

# 导出环境变量
export EXPORTED_VAR="这是一个导出变量"
EOF

# 创建测试脚本4 - 嵌套source
echo ">> 创建嵌套source测试脚本"
cat > source_test4.sh << 'EOF'
#!/bin/sh
# 嵌套source测试
echo "执行 source_test4.sh"
NESTED_VAR="嵌套变量"

# 嵌套source调用
echo "从source_test4.sh中调用source_test1.sh"
source source_test1.sh
echo "从source_test4.sh访问TEST_VAR1: $TEST_VAR1"
EOF

# 开始测试
echo "\n===== 开始Source命令测试 ====="

echo "\n>> 测试1: 基本变量"
source source_test1.sh
echo "TEST_VAR1 = $TEST_VAR1"
echo "TEST_VAR2 = $TEST_VAR2"
echo "TEST_VAR3 = $TEST_VAR3"

echo "\n>> 测试2: 函数定义"
source source_test2.sh
test_func1 "参数1" "参数2"
test_func2
echo "函数返回值: $?"

echo "\n>> 测试3: 复杂脚本"
source source_test3.sh
echo "EXPORTED_VAR = $EXPORTED_VAR"

echo "\n>> 测试4: 嵌套source"
source source_test4.sh
echo "NESTED_VAR = $NESTED_VAR"
echo "再次确认TEST_VAR1 = $TEST_VAR1"

# 清理
echo "\n===== 清理测试文件 ====="
rm -f source_test1.sh source_test2.sh source_test3.sh source_test4.sh

echo "\n======== Source 命令测试完成 ======== 