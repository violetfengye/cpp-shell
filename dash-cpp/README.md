# Dash-CPP

Dash-CPP 是一个使用C++重新实现的dash shell，旨在提供一个现代化、面向对象的shell实现。

## 特点

- 利用现代C++17特性
- 面向对象设计
- 异常处理机制
- 智能指针进行内存管理
- 模块化架构
- 单元测试支持

## 当前状态

该项目目前正在积极开发中。以下组件已经实现：

- 核心shell基础设施
- 输入处理系统
- 变量管理
- 词法分析器
- shell命令解析器
- 抽象语法树节点
- 命令执行框架
- 作业控制系统
- 基本内置命令（cd、exit、echo）

## 最新改进

我们最近对后台作业管理功能进行了显著改进：

1. 修复了后台作业状态错误问题：
   - 解决了后台命令（如"sleep 30 &"）立即显示为"已完成"的问题
   - 通过在Job::updateStatus()中添加kill(pid, 0)检查来验证进程是否仍在运行

2. 清理了调试输出：
   - 删除了bg_job_adapter.cpp、bg_job_control.c、shell.cpp和executor.cpp中多余的DEBUG打印语句
   - 提高了代码的可读性和执行效率

3. 改进了用户界面：
   - 使用真实作业ID替代了硬编码的[1]
   - 简化了后台作业提示，移除了多余的重定向信息
   - 优化了输出文件命名格式，从"bg_output_PID_时间戳.txt"改为简洁的"output_PID.txt"
   - 修改为让子进程直接使用自己的PID创建输出文件

4. 增强了作业状态显示：
   - 在C++和C两个版本的实现中，都在"已完成"状态后显示进程PID信息
   - 提供更清晰的作业状态反馈

## 构建要求

- C++17兼容编译器（GCC 7+、Clang 5+、MSVC 2017+）
- CMake 3.10或更高版本
- 可选：Google Test（用于单元测试）

## 构建步骤

```bash
# 克隆仓库
git clone https://github.com/yourusername/dash-cpp.git
cd dash-cpp

# 创建构建目录
mkdir build
cd build

# 配置
cmake ..

# 构建
cmake --build .

# 运行测试（可选）
ctest
```

## 项目结构

```
dash-cpp/
├── include/                 # 头文件
│   ├── builtins/            # 内置命令
│   ├── core/                # 核心组件
│   ├── job/                 # 作业控制
│   ├── utils/               # 实用工具类
│   ├── variable/            # 变量管理
│   └── dash.h               # 主头文件
├── src/                     # 源文件
│   ├── builtins/            # 内置命令实现
│   ├── core/                # 核心组件实现
│   ├── job/                 # 作业控制实现
│   ├── utils/               # 实用工具类实现
│   ├── variable/            # 变量管理实现
│   └── main.cpp             # 程序入口点
├── tests/                   # 测试文件
├── CMakeLists.txt           # CMake配置文件
└── README.md                # 本文件
```

## 主要组件

- **Shell**：程序的核心控制器，协调其他组件的工作
- **Parser**：解析命令行输入并构建抽象语法树
- **Lexer**：词法分析器，将输入分解为词法标记
- **Executor**：执行命令树
- **InputHandler**：处理输入源（标准输入、文件、字符串）
- **VariableManager**：管理shell变量和环境变量
- **JobControl**：管理作业控制
- **Node**：抽象语法树节点

## 路线图

- 实现更多内置命令
- 增强作业控制功能
- 添加命令历史和行编辑
- 实现命令补全
- 添加脚本功能（循环、条件、函数）
- 改进错误处理和报告

## 许可证

该项目基于MIT许可证。详情请参见LICENSE文件。

## 贡献

欢迎对代码进行贡献、报告问题或提出改进建议！
