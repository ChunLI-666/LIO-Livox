# 环境安装文档

本目录包含 LIO-Livox 项目的环境安装和配置文档。

## 文档索引

### Docker 环境安装

- **[Docker 快速安装指南](./INSTALL_DOCKER.md)** - Docker 环境的快速安装和使用说明
- **[Docker 详细文档](./DOCKER_SETUP.md)** - Docker 环境的完整安装、配置和使用文档

### 系统依赖

项目所需的主要依赖库：

- **ROS2 Humble** - 机器人操作系统框架
- **Eigen3** - 线性代数库
- **Ceres Solver** - 非线性优化库
- **PCL** - 点云处理库
- **OpenCV** - 计算机视觉库
- **SuiteSparse** - 稀疏矩阵求解库
- **Google Glog** - 日志库
- **colcon** - ROS2 构建工具

## 快速开始

### 使用 Docker（推荐）

最快的开始方式是使用 Docker 环境：

```bash
# 1. 安装 Docker（如果未安装）
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
newgrp docker

# 2. 构建和运行（从工作空间根目录）
cd /path/to/colcon_ws
docker-compose build
docker-compose up -d
docker-compose exec lio-livox bash

# 3. 在容器内构建项目
cd /workspace
colcon build --packages-select lio_livox
```

详细步骤请参考 [Docker 安装文档](./INSTALL_DOCKER.md)。

### 本地安装（不使用 Docker）

#### Ubuntu 22.04 + ROS2 Humble

1. **安装 ROS2 Humble**

```bash
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository universe
sudo apt update && sudo apt install -y curl gnupg lsb-release
sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
sudo sh -c 'echo "deb [arch=$(dpkg --print-architecture)] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-latest.list'
sudo apt update
sudo apt install -y ros-humble-desktop
```

2. **安装依赖库**

```bash
sudo apt install -y \
    libeigen3-dev \
    libceres-dev \
    libpcl-dev \
    libopencv-dev \
    libsuitesparse-dev \
    libgoogle-glog-dev \
    python3-colcon-common-extensions
```

3. **安装 colcon（如果未安装）**

```bash
pip3 install --user colcon-common-extensions
export PATH=$HOME/.local/bin:$PATH
```

4. **构建项目**

```bash
cd /path/to/colcon_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select lio_livox
source install/setup.bash
```

## 环境要求

- **操作系统**: Ubuntu 20.04 (ROS2 Foxy) 或 Ubuntu 22.04 (ROS2 Humble)
- **ROS2 版本**: Foxy 或 Humble（推荐 Humble）
- **C++ 标准**: C++14 或更高
- **内存**: 建议至少 8GB RAM
- **磁盘空间**: 建议至少 10GB 可用空间

## 验证安装

安装完成后，验证环境是否正确：

```bash
# 检查 ROS2 环境
source /opt/ros/humble/setup.bash
ros2 --version

# 检查依赖库
pkg-config --modversion eigen3
pkg-config --modversion ceres
pkg-config --modversion opencv4

# 检查项目构建
cd /path/to/colcon_ws
colcon build --packages-select lio_livox
```

## 故障排除

### Docker 相关问题

请参考 [Docker 详细文档](./DOCKER_SETUP.md) 中的"常见问题"部分。

### 依赖安装问题

如果遇到依赖安装问题：

1. 检查 Ubuntu 版本是否匹配
2. 更新软件包列表：`sudo apt update`
3. 检查 ROS2 源是否正确配置
4. 查看错误日志获取详细信息

### 编译错误

如果编译失败：

1. 确认所有依赖都已正确安装
2. 清理构建目录后重新编译：
   ```bash
   rm -rf build/ install/ log/
   colcon build --packages-select lio_livox
   ```
3. 查看详细编译日志：
   ```bash
   colcon build --packages-select lio_livox --event-handlers console_direct+
   ```

## 相关文档

- [项目概览文档](../PROJECT_OVERVIEW.md)
- [主 README 文档](../../README.md)
