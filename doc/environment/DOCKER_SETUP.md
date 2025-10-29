# LIO-Livox Docker 环境安装指南

本文档说明如何在其他 Ubuntu 机器上通过 Docker 安装和运行 LIO-Livox 项目。

> **注意**: 本文档适用于项目根目录（colcon_ws）的 Docker 配置。如果从项目子目录（src/LIO-Livox）运行，需要调整路径。

## 前置要求

- Docker Engine (版本 20.10 或更高)
- Docker Compose (版本 2.0 或更高，可选但推荐)
- 至少 10GB 可用磁盘空间
- 如果使用 GUI 工具（RViz），需要 X11 转发支持

### 安装 Docker（如果未安装）

```bash
# 更新包索引
sudo apt update

# 安装必要的依赖
sudo apt install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release

# 添加 Docker 官方 GPG 密钥
sudo mkdir -p /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg

# 设置 Docker 仓库
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

# 安装 Docker Engine
sudo apt update
sudo apt install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# 将当前用户添加到 docker 组（避免每次使用 sudo）
sudo usermod -aG docker $USER
newgrp docker  # 或重新登录以应用组更改
```

### 安装 Docker Compose（如果需要独立版本）

```bash
sudo apt install -y docker-compose
```

## 快速开始

### 方法 1：使用 Docker Compose（推荐）

1. **确保在 colcon_ws 根目录**

```bash
cd /path/to/colcon_ws
# 确保 Dockerfile 和 docker-compose.yml 在当前目录
```

2. **构建 Docker 镜像**

```bash
docker-compose build
```

3. **启动容器**

```bash
docker-compose up -d  # 后台运行
# 或
docker-compose run --rm lio-livox  # 临时运行，退出后删除容器
```

4. **进入容器**

```bash
docker-compose exec lio-livox bash
```

### 方法 2：使用 Docker 命令

1. **构建镜像**

```bash
cd /path/to/colcon_ws
docker build -t lio-livox:humble .
```

2. **运行容器**

```bash
# 基础运行
docker run -it --rm \
  --network host \
  -v $(pwd)/src:/workspace/src:rw \
  -v $(pwd)/build:/workspace/build:rw \
  -v $(pwd)/install:/workspace/install:rw \
  -v $(pwd)/logs:/workspace/logs:rw \
  lio-livox:humble

# 如果需要 GUI 支持（RViz）
xhost +local:docker  # 允许 Docker 访问 X11
docker run -it --rm \
  --network host \
  -e DISPLAY=$DISPLAY \
  -e QT_X11_NO_MITSHM=1 \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  -v $(pwd)/src:/workspace/src:rw \
  -v $(pwd)/build:/workspace/build:rw \
  -v $(pwd)/install:/workspace/install:rw \
  -v $(pwd)/logs:/workspace/logs:rw \
  --shm-size=2gb \
  lio-livox:humble
```

## 在容器内构建项目

进入容器后：

```bash
# 确保 ROS2 环境已加载（entrypoint.sh 已处理）
cd /workspace

# 构建项目
colcon build --packages-select lio_livox

# 或者构建所有包
colcon build

# 如果构建失败，查看详细日志
colcon build --packages-select lio_livox --event-handlers console_direct+
```

## 运行项目

### 准备启动文件

确保配置文件和启动文件在正确位置：

```bash
# 在容器内
source /workspace/install/setup.bash

# 运行 ScanRegistration 节点
ros2 run lio_livox ScanRegistration

# 运行 PoseEstimation 节点
ros2 run lio_livox PoseEstimation

# 或使用 launch 文件
ros2 launch lio_livox horizon.launch.py
```

### 运行示例（需要 bag 文件）

```bash
# 在容器内
source /workspace/install/setup.bash

# 启动 LIO 系统
ros2 launch lio_livox horizon.launch.py

# 在另一个终端（也可以从主机）播放 bag 文件
# docker-compose exec lio-livox bash
ros2 bag play /path/to/your/rosbag.bag
```

## 图形界面支持（RViz）

如果需要在容器内运行 RViz：

1. **主机端配置**

```bash
# 允许 Docker 访问 X11
xhost +local:docker

# 或在 /etc/X11/xhost.conf 中添加（更安全）
# 只允许特定容器
```

2. **使用 docker-compose 运行**

docker-compose.yml 已经配置了 X11 转发，直接使用即可。

3. **验证图形支持**

```bash
# 在容器内
ros2 run rviz2 rviz2
```

## 数据持久化

项目使用卷挂载来持久化数据：

- `./src/` - 源代码（可写，方便修改）
- `./build/` - 构建输出
- `./install/` - 安装输出
- `./logs/` - 日志文件

这些目录在主机和容器之间是共享的。

## 常见问题

### 1. 构建失败：找不到依赖包

```bash
# 确保在容器内，ROS2 环境已加载
source /opt/ros/humble/setup.bash

# 安装缺失的依赖（Dockerfile 应该已包含）
apt-get update
apt-get install -y <missing-package>
```

### 2. 权限问题

```bash
# 如果遇到权限问题，可以调整挂载的用户
# 在 docker-compose.yml 中取消注释 user 行并设置正确的 UID/GID
```

### 3. 网络问题（ROS2 节点无法通信）

```bash
# 确保使用 --network host 模式
# docker-compose.yml 已经配置了 network_mode: host
```

### 4. GUI 无法显示

```bash
# 检查 DISPLAY 环境变量
echo $DISPLAY

# 允许 X11 转发
xhost +local:docker

# 检查 /tmp/.X11-unix 挂载
ls -la /tmp/.X11-unix
```

## 开发工作流

### 方式 1：在容器内直接开发

```bash
# 进入容器
docker-compose exec lio-livox bash

# 修改代码（使用 vim 或其他编辑器）
vim src/LIO-Livox/src/lio/SomeFile.cpp

# 重新构建
colcon build --packages-select lio_livox
```

### 方式 2：在主机上编辑，容器内构建

```bash
# 在主机上使用 IDE 编辑代码
# 代码变更会自动同步到容器（通过卷挂载）

# 在容器内重新构建
docker-compose exec lio-livox bash -c "cd /workspace && colcon build --packages-select lio_livox"
```

## 清理

```bash
# 停止并删除容器
docker-compose down

# 删除镜像（可选）
docker rmi lio-livox:humble

# 清理构建产物（如果需要）
rm -rf build/ install/ log/
```

## 环境信息

- **基础镜像**: Ubuntu 22.04
- **ROS2 版本**: Humble Hawksbill
- **Python 版本**: Python 3.10
- **C++ 标准**: C++14

## 依赖列表

容器内已安装的主要依赖：

- ROS2 Humble Desktop
- Eigen3
- Ceres Solver
- PCL (Point Cloud Library)
- OpenCV
- SuiteSparse
- Google Glog
- colcon 构建工具

完整依赖列表请参考 Dockerfile。

## 联系与支持

如有问题，请查阅项目主 [README.md](../../README.md) 或提交 Issue。

## 相关文档

- [环境安装索引](./README.md)
- [项目概览文档](../../PROJECT_OVERVIEW.md)
