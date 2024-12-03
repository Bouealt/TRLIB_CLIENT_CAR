# 使用 Ubuntu 20.04 作为基础镜像
FROM ubuntu:20.04

# 设置环境变量，避免交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 更新并安装必要的工具和依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    gcc \
    git \
    wget \
    libboost-all-dev \
    libopencv-dev \
    libpcl-dev \
    portaudio19-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /app

# 将项目代码复制到 Docker 容器中
COPY . /app

# 创建并构建项目
RUN cmake . && make

# 设置容器启动时进入 bash
CMD ["/bin/bash"]

