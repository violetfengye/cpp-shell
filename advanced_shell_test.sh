#!/bin/sh
# advanced_shell_test.sh - 测试shell的高级功能

echo "======== Dash-CPP Shell 高级功能测试 ========"

# 测试部分1: 控制结构 - if语句
echo "\n===== 测试if语句 ====="

echo ">> 测试简单if"
if true; then
    echo "条件为真"
fi

echo ">> 测试if-else"
if false; then
    echo "这不应该被打印"
else
    echo "条件为假"
fi

echo ">> 测试if-elif-else"
VALUE=10
if [ $VALUE -lt 5 ]; then
    echo "VALUE小于5"
elif [ $VALUE -lt 15 ]; then
    echo "VALUE小于15"
else
    echo "VALUE大于等于15"
fi

# 测试部分2: 控制结构 - for循环
echo "\n===== 测试for循环 ====="

echo ">> 测试基本for循环"
for i in 1 2 3 4 5; do
    echo "循环计数: $i"
done

echo ">> 测试for循环遍历文件"
for file in *.sh; do
    echo "发现脚本文件: $file"
done

# 测试部分3: 控制结构 - while循环
echo "\n===== 测试while循环 ====="

echo ">> 测试基本while循环"
COUNT=1
while [ $COUNT -le 5 ]; do
    echo "While循环计数: $COUNT"
    COUNT=$((COUNT+1))
done

# 测试部分4: 函数
echo "\n===== 测试函数 ====="

echo ">> 定义和调用函数"
hello_func() {
    echo "Hello from function!"
    echo "参数1: $1"
    echo "参数2: $2"
    return 42
}

hello_func "arg1" "arg2"
echo "函数返回值: $?"

# 测试部分5: 命令替换
echo "\n===== 测试命令替换 ====="

echo ">> 测试命令替换 \`command\`"
CURRENT_DIR=`pwd`
echo "当前目录(使用反引号): $CURRENT_DIR"

echo ">> 测试命令替换 \$(command)"
FILES_COUNT=$(ls | wc -l)
echo "当前目录文件数量: $FILES_COUNT"

# 测试部分6: 算术运算
echo "\n===== 测试算术运算 ====="

echo ">> 测试基本算术"
A=5
B=3
echo "A + B = $((A + B))"
echo "A - B = $((A - B))"
echo "A * B = $((A * B))"
echo "A / B = $((A / B))"
echo "A % B = $((A % B))"

# 测试部分7: 字符串操作
echo "\n===== 测试字符串操作 ====="

echo ">> 测试字符串连接"
STR1="Hello"
STR2="World"
STR3="${STR1} ${STR2}"
echo "连接结果: $STR3"

echo ">> 测试字符串长度"
echo "STR3长度: ${#STR3}"

# 测试部分8: 特殊变量
echo "\n===== 测试特殊变量 ====="

echo ">> 脚本名称: $0"
echo ">> 当前进程ID: $$"
echo ">> 最后一个后台进程ID: $!"
echo ">> 最后一个命令的退出状态: $?"

# 测试部分9: 信号处理
echo "\n===== 测试信号处理 ====="

echo ">> 设置信号处理器"
trap "echo '捕获到中断信号'" INT
echo "按Ctrl+C测试信号处理(5秒后继续)"
sleep 5
trap - INT  # 恢复默认处理

echo "\n======== 高级测试完成 ======== 