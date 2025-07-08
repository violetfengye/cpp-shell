#!/bin/sh
# test_nested_source.sh - 测试嵌套source功能

echo "======== 测试嵌套source功能 ========"

# 创建第一个脚本
echo '#!/bin/sh' > source_level1.sh
echo 'echo "这是第一级source脚本"' >> source_level1.sh
echo 'LEVEL1_VAR="第一级变量"' >> source_level1.sh
echo 'source source_level2.sh' >> source_level1.sh
echo 'echo "回到第一级脚本，LEVEL2_VAR = $LEVEL2_VAR"' >> source_level1.sh

# 创建第二个脚本
echo '#!/bin/sh' > source_level2.sh
echo 'echo "这是第二级source脚本"' >> source_level2.sh
echo 'LEVEL2_VAR="第二级变量"' >> source_level2.sh
echo 'echo "在第二级脚本中，LEVEL1_VAR = $LEVEL1_VAR"' >> source_level2.sh

# 执行第一个脚本
echo ">> 执行嵌套source测试"
source source_level1.sh

# 验证变量
echo ">> 验证变量"
echo "LEVEL1_VAR = $LEVEL1_VAR"
echo "LEVEL2_VAR = $LEVEL2_VAR"

# 清理
echo ">> 清理文件"
rm -f source_level1.sh source_level2.sh

echo "======== 测试完成 ========" 