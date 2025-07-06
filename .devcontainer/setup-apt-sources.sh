#!/bin/bash

# 备份原始的 sources.list
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak

# 定义清华大学 Ubuntu 22.04 LTS (Jammy Jellyfish) 的 APT 源
TUNA_SOURCES="""
# 默认注释了源码镜像以提高 apt update 速度，但软件包依然是二进制分发，不影响使用
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-updates main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-updates main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-backports main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-backports main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-security main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-security main restricted universe multiverse

# 清华大学开源软件镜像站 | https://mirrors.tuna.tsinghua.edu.cn/
# deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-proposed main restricted universe multiverse
# deb-src https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ jammy-proposed main restricted universe multiverse
"""

# 将新的源写入 sources.list
echo "${TUNA_SOURCES}" | sudo tee /etc/apt/sources.list

# 更新 APT 软件包列表
sudo apt update

sudo apt install -y build-essential libreadline-dev cmake

# 安装nodejs 和 gemini
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash

export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" # This loads nvm
[ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion" # This loads nvm bash_completion

nvm ls-remote

nvm install node

nvm alias default node

npm config set registry https://registry.npmmirror.com

npm install -g @google/gemini-cli