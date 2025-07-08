#!/bin/bash
echo "开始测试作业控制功能"
sleep 10 &
echo "运行 sleep 10 &"
jobs
echo "10秒后再次检查作业状态"
sleep 10
jobs
echo "测试完成" 