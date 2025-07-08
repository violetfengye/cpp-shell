#!/bin/bash
# test_completion.sh - 测试dash-cpp shell的补全功能

# 创建测试目录结构
echo "创建测试目录结构..."
mkdir -p test_dir/subdir1/subsubdir
mkdir -p test_dir/subdir2
touch test_dir/file1.txt
touch test_dir/file2.cpp
touch test_dir/subdir1/file3.h
touch test_dir/subdir1/subsubdir/deep_file.txt
chmod +x test_dir/file2.cpp  # 使其可执行

# 创建测试用的可执行文件
echo "创建测试用的可执行文件..."
echo '#!/bin/bash
echo "This is test_exec1"' > test_dir/test_exec1
chmod +x test_dir/test_exec1

echo '#!/bin/bash
echo "This is test_exec2"' > test_dir/subdir1/test_exec2
chmod +x test_dir/subdir1/test_exec2

# 创建测试输入文件
echo "创建测试输入文件..."
cat > test_completion_input.txt << EOF
# 测试命令补全
ec
# 应该补全为 echo

cd
# 应该列出目录

cd test_
# 应该补全为 test_dir/

cd test_dir/sub
# 应该列出 subdir1/ 和 subdir2/

cd test_dir/subdir1/sub
# 应该补全为 subsubdir/

# 测试文件补全
cat test_dir/f
# 应该列出 file1.txt 和 file2.cpp

# 测试可执行文件补全
./test_dir/test_
# 应该补全为 test_exec1

# 测试多级路径补全
cat test_dir/subdir1/sub
# 应该补全为 subsubdir/

# 测试特殊字符
cat test_dir/file\ 
# 如果有空格文件名，应该正确处理

# 测试管道后的命令补全
echo hello | g
# 应该补全为 grep

# 测试重定向后的文件补全
echo hello > test_dir/f
# 应该列出 file1.txt 和 file2.cpp

# 退出
exit
EOF

# 打印使用说明
echo "测试文件已创建。请按照以下步骤进行测试："
echo "1. 编译dash-cpp shell"
echo "2. 运行shell：./dash"
echo "3. 在shell中，按照test_completion_input.txt中的说明进行测试"
echo "4. 对于每一行注释后的命令，输入命令并按Tab键，观察补全结果"
echo "5. 验证补全结果是否符合预期"
echo ""
echo "测试目录结构："
find test_dir -type f | sort
echo ""
echo "您也可以直接查看测试输入文件：cat test_completion_input.txt" 