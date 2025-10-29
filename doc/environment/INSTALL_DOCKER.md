# Docker 环境安装说明

## 快速安装指南

### 1. 安装 Docker

```bash
# Ubuntu/Debian
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# 将当前用户添加到 docker 组
sudo usermod -aG docker $USER
newgrp docker
```

### 2. 构建和运行

```bash
# 进入工作空间
cd /path/to/colcon_ws

# 构建 Docker 镜像
./docker/quick_start.sh build

# 启动容器
./docker/quick_start.sh up

# 进入容器
./docker/quick_start.sh shell
```

### 3. 在容器内构建项目

```bash
# 方法 1: 使用快速启动脚本
./docker/quick_start.sh build-project

# 方法 2: 进入容器后手动构建
docker-compose exec lio-livox bash
cd /workspace
colcon build --packages-select lio_livox
```

## 更多信息

详细的使用说明请参考 [DOCKER_SETUP.md](./DOCKER_SETUP.md)
