TRLIB_CAR_CLIENT
<!-- 2024-9-3 联调问题记录
1. 存在不同设备发送文件数量不同的问题，发送一段时间后就断开了。
2. 存在多摄像头采集时间不不同步问题。
3. 存在不同子线程结束时间不同步问题，当主线程结束后，发送子线程还未发送的文件也会被强制结束。 -->
## 环境要求

本项目的开发和运行环境要求如下：

- **操作系统**: Ubuntu ≥ 20.04 LTS
- **构建工具**: CMake ≥ 3.15
- **编译器**: GCC ≥ 9.1
- **依赖库**:
  - **Boost**: ≥ 1.65.0（需要 `system` 组件）
  - **OpenCV**: ≥ 3.2.0
  - **PCL (Point Cloud Library)**: ≥ 1.2
  - **PortAudio**: ≥ 19.6.0
  - **OpenSSL**: ≥ 1.1.1

确保系统已满足以上版本要求，以下步骤将帮助完成项目的构建与运行。

## 运行流程

### 克隆项目并进入项目目录
- git clone <git@github.com:Bouealt/TRLIB_CLIENT_CAR.git>
- cd file_client

### 切换到dev分支
git checkout dev

### 生成构建文件
cmake .

### 编译项目
make

<!-- ### 给予运行权限
sudo chmod 666 /dev/ttyUSB0 保证运行
sudo chmod 666 /dev/video0 保证运行 -->

### 运行可执行文件（启动需要权限）
./VehicleClient



## 依赖库安装
### 安装CMake和GCC
sudo apt update
sudo apt install -y cmake gcc g++

### 安装Boost库
sudo apt install -y libboost-all-dev

### 安装OpenCV
sudo apt install -y libopencv-dev

### 安装PCL
sudo apt install -y libpcl-dev

### 安装PortAudio
sudo apt install -y portaudio19-dev

### 安装OpenSSL
sudo apt install -y libssl-dev

## 快捷指令

### 检查Ubuntu版本
lsb_release -a

### 检查CMake版本
cmake --version

### 检查GCC版本
gcc --version

### 检查Boost版本
dpkg -s libboost-dev | grep 'Version'

### 检查OpenCV版本
pkg-config --modversion opencv4  # 或 opencv，如果使用的是3.x版本

### 检查PCL版本
dpkg -s libpcl-dev | grep 'Version'

### 检查PortAudio版本
dpkg -s portaudio19-dev | grep 'Version'

### 检查OpenSSL版本
openssl version