# Dash-CPP-Merge 实施计划（更新版）

## 详细比较分析

### 共同功能
1. **词法分析器和解析器**：两个系统的词法分析器实现完全相同，解析器实现非常相似
2. **执行器**：基本功能相同，包括内置命令处理、外部命令执行、重定向等
3. **作业控制基础**：基本作业控制功能和数据结构
4. **基本内置命令**：cd, echo, exit, bg, fg, jobs, pwd等常见命令

### dash-cpp 特有功能
1. **调试系统**：debug_command实现的详细调试功能
2. **帮助系统**：help_command提供的在线帮助
3. **历史记录命令**：history_command提供的历史记录管理
4. **脚本执行**：source_command提供的脚本执行功能
5. **解析器增强**：parse方法和getLastCommand函数

### dash-cpp-main 特有功能
1. **后台任务API**：C风格的bg_job_control实现
2. **任务管理适配器**：bg_job_adapter提供的C++接口
3. **进程管理命令**：kill_command提供的信号发送功能
4. **作业等待命令**：wait_command提供的等待后台作业功能
5. **命令节点增强**：CommandNode中的background_属性和相关方法

## 已完成工作

1. 创建了基本的项目目录结构
2. 添加了CMakeLists.txt和FindReadline.cmake文件
3. 创建了主要头文件：
   - dash.h - 主头文件
   - shell.h - Shell类定义
   - builtin_command.h - 内置命令基类
   - job_control.h - 作业控制类，整合了两个系统的功能
   - bg_job_control.h - C风格后台任务控制API
   - bg_job_adapter.h - C++风格适配器
   - kill_command.h - 杀进程命令
   - wait_command.h - 等待命令
4. 创建了项目README.md

## 整合方案（更新）

我们的整合策略是保留两个系统的所有功能，并确保它们无缝协同工作：

### 1. 核心类整合

1. **Shell类**：
   - 在Shell类中添加getBGJobAdapter方法以访问后台作业适配器
   - 确保setupSignalHandlers处理两个系统的信号要求

2. **Node类**：
   - 整合两个系统的CommandNode实现，加入background_属性和相关方法
   - 更新PipeNode以包含dash-cpp-main中的setBackground方法

3. **Parser类**：
   - 保留dash-cpp的parse方法和getLastCommand功能
   - 确保对后台任务的正确解析

4. **Executor类**：
   - 整合两系统的executeExternalCommand实现，确保正确处理后台作业
   - 注册所有内置命令

5. **JobControl类**：
   - 使用bg_job_adapter实例作为成员变量
   - 针对C风格API提供相应C++接口
   - 确保JobControl::waitForJob方法可以等待特定PID

### 2. 内置命令整合

将两个系统的所有内置命令合并，并确保所有输出都使用中文：

1. **保留dash-cpp中的命令**：
   - debug_command（调试命令）
   - help_command（帮助命令）
   - history_command（历史命令）
   - source_command（源文件执行）

2. **添加dash-cpp-main中的命令**：
   - kill_command（发送信号）
   - wait_command（等待作业）

3. **整合共同命令**：
   - 确保bg_command, fg_command, jobs_command支持两种任务控制系统

### 3. 任务管理整合

1. **双层任务控制架构**：
   - JobControl类提供高级C++接口
   - BGJobAdapter类连接到底层C风格API

2. **任务控制流程**：
   ```
   Shell -> JobControl -> BGJobAdapter -> bg_job_control API
   ```

3. **改进点**：
   - 统一作业状态表示
   - 确保作业通知机制正常工作
   - 支持命令行作业规格（如%1, %2等）

## 待完成工作（详细）

1. **源文件实现**：
   - bg_job_control.c - 从dash-cpp-main复制，并添加中文注释和错误信息
   - bg_job_adapter.cpp - 从dash-cpp-main复制并整合
   - job_control.cpp - 整合两个系统的实现
   - shell.cpp - 整合两个系统的实现，确保支持所有功能

2. **内置命令实现**：
   - 从dash-cpp复制：debug_command, help_command, history_command, source_command
   - 从dash-cpp-main添加：kill_command, wait_command
   - 整合共同命令：bg_command, cd_command, echo_command, exit_command, fg_command, jobs_command, pwd_command
   - 修改所有命令输出为中文

3. **核心组件实现**：
   - lexer.cpp - 保持原有功能
   - parser.cpp - 整合两个系统的实现
   - node.cpp - 整合两个系统的实现，确保支持后台任务
   - executor.cpp - 整合两个系统的实现
   - input.cpp - 保持原有功能
   - output.cpp - 修改输出为中文
   - 变量管理器 - 保持原有功能

4. **测试和验证**：
   - 编写基本测试用例
   - 测试后台命令执行
   - 测试任务控制功能
   - 测试中文输出

## 后续改进

1. 添加更多内置命令
2. 改进命令补全功能
3. 增强脚本执行能力
4. 优化性能和内存使用
5. 添加更多用户友好的功能（如命令别名、自定义提示符等） 