#!/bin/bash

echo "===== 问题测试脚本 ====="
echo "此脚本测试已发现的dash-cpp实现问题"

# 1. 测试控制结构问题
echo -e "\n[测试1] 控制结构测试（if, for, while）"
echo "注意：这些命令应直接在shell中执行，不通过source"
cat << 'EOT'
# 测试if结构
if [ 1 -eq 1 ]; then
    echo "if测试成功"
else
    echo "if测试失败"
fi

# 测试for循环
for i in 1 2 3; do
    echo "for循环: $i"
done

# 测试while循环
count=0
while [ $count -lt 3 ]; do
    echo "while循环: $count"
    count=$((count + 1))
done
EOT

# 2. 测试Here-document语法
echo -e "\n[测试2] Here-document语法测试"
echo "注意：这些命令应直接在shell中执行，不通过source"
cat << 'EOT'
cat << EOF
这是here-document测试
可以包含多行文本
EOF
EOT

# 3. 测试变量作用域在嵌套source中的问题
echo -e "\n[测试3] 嵌套source中的变量作用域"
echo "创建临时测试文件..."

# 创建父脚本
cat > parent.sh << 'EOF'
#!/bin/bash
echo "父脚本: 设置变量 test_var=parent"
test_var="parent"
echo "父脚本: test_var=$test_var"
echo "父脚本: 执行子脚本..."
source child.sh
echo "父脚本: 子脚本执行后 test_var=$test_var"
EOF

# 创建子脚本
cat > child.sh << 'EOF'
#!/bin/bash
echo "子脚本: 收到的 test_var=$test_var"
echo "子脚本: 修改变量 test_var=child"
test_var="child"
echo "子脚本: 修改后 test_var=$test_var"
EOF

echo "测试方法："
echo "1. 执行: source parent.sh"
echo "2. 观察变量在父脚本和子脚本间的传递和修改"

# 4. 测试组合重定向语法
echo -e "\n[测试4] 组合重定向语法测试"
echo "注意：这些命令应直接在shell中执行，不通过source"
cat << 'EOT'
# 测试标准输出和错误输出重定向到同一文件
echo "正常输出" &> redirect_test.txt
echo "带错误" 2>&1 > redirect_test.txt
cat redirect_test.txt

# 测试管道和重定向组合
ls -la | grep "test" > pipe_test.txt
cat pipe_test.txt
EOT

echo -e "\n===== 直接在shell中执行的命令清单 ====="
echo "以下命令应直接在shell中执行，而非通过source命令："
echo "1. 所有控制结构（if, for, while）"
echo "2. Here-document语法（<<EOF）"
echo "3. 复杂的重定向组合（&>, 2>&1）"
echo "4. 具有管道和重定向组合的复杂命令"

echo -e "\n可以通过source执行的命令："
echo "1. 简单命令执行"
echo "2. 变量设置和读取"
echo "3. 简单重定向（> 和 <）"

echo -e "\n清理临时文件方法："
echo "rm parent.sh child.sh redirect_test.txt pipe_test.txt (如果已创建)" 