# LIO-Livox 项目概览文档

## 项目简介

**LIO-Livox** 是一个针对 Livox 激光雷达的鲁棒激光-惯性里程计（LiDAR-Inertial Odometry）系统。该系统仅使用单个 Livox 激光雷达及其内置 IMU，具有独立的初始化模块，能够在静态、动态或混合状态下完成初始化。

### 核心特性

- **鲁棒初始化**：支持静态、动态或混合状态的初始化，无需特定的传感器运动模式
- **高性能**：可在4公里隧道中通过，支持高速公路高速运行（约80km/h）
- **动态物体鲁棒性**：对车辆、自行车、行人等动态物体具有良好的鲁棒性
- **精确建图**：即使在大部分视场被车辆遮挡的情况下，仍能获得精确的建图结果

## 系统架构

系统由两个 ROS 节点组成：

1. **ScanRegistration** - 扫描配准节点
2. **PoseEstimation** - 位姿估计节点

### 节点功能

#### ScanRegistration 节点

- **功能模块**：`LidarFeatureExtractor` 类负责从原始点云中提取特征
- **主要任务**：
  - 动态物体移除：使用快速点云分割方法移除动态物体
  - 特征提取：提取角点特征（corner features）、面特征（surface features）和异常特征（irregular features）
  - 特征分布优化：确保特征点分布广泛且均匀，以提供对6自由度的充分约束

#### PoseEstimation 节点

- **功能模块**：`Estimator` 类负责位姿估计和建图
- **主要任务**：
  - 运动畸变补偿：使用 IMU 预积分或恒定速度模型
  - IMU 初始化：基于最大后验概率（MAP）估计方法联合初始化 IMU 偏置、速度和重力方向
  - 紧耦合融合：滑动窗口基础上的传感器融合，估计 IMU 位姿、偏置和速度
  - 并行建图：独立的线程并行构建和维护全局地图

## 项目结构

```
LIO-Livox/
├── src/
│   └── lio/
│       ├── ScanRegistration.cpp      # 扫描配准节点主程序
│       ├── PoseEstimation.cpp        # 位姿估计节点主程序
│       ├── LidarFeatureExtractor.cpp # 特征提取实现
│       ├── Estimator.cpp             # 位姿估计器实现
│       ├── IMUIntegrator.cpp         # IMU 积分器实现
│       ├── Map_Manager.cpp           # 地图管理器实现
│       └── ceresfunc.cpp             # Ceres 优化函数
├── include/
│   ├── Estimator/
│   │   └── Estimator.h               # 位姿估计器头文件
│   ├── IMUIntegrator/
│   │   └── IMUIntegrator.h           # IMU 积分器头文件
│   ├── LidarFeatureExtractor/
│   │   └── LidarFeatureExtractor.h   # 特征提取器头文件
│   ├── MapManager/
│   │   └── Map_Manager.h             # 地图管理器头文件
│   ├── segment/
│   │   ├── segment.hpp               # 点云分割模块
│   │   └── pointsCorrect.hpp         # 点云纠正模块
│   ├── utils/
│   │   ├── ceresfunc.h               # Ceres 优化函数头文件
│   │   ├── logger.h                  # 日志工具头文件
│   │   └── math_utils.hpp            # 数学工具函数
│   └── sophus/                       # Sophus 几何库
├── config/
│   ├── horizon_config.yaml           # Horizon 激光雷达配置
│   ├── hap_config.yaml               # HAP 激光雷达配置
│   └── mid360_config.yaml            # Mid-360 激光雷达配置
├── launch/
│   ├── horizon.launch.py             # Horizon 启动文件
│   ├── hap.launch.py                 # HAP 启动文件
│   └── mid360.launch.py              # Mid-360 启动文件
├── CMakeLists.txt                    # CMake 构建配置
├── package.xml                       # ROS 包描述文件
└── README.md                         # 项目说明文档
```

## 核心组件说明

### 1. LidarFeatureExtractor（特征提取器）

**功能**：从原始点云中提取三类特征点
- **角点特征（Corner Features）**：具有大曲率的点和每个扫描线上的孤立点
- **面特征（Surface Features）**：通过主成分分析（PCA）提取的平面特征
- **异常特征（Irregular Features）**：在特征贫乏场景中提供信息的异常点

**关键特性**：
- 根据距离设置不同的阈值，使空间中的点分布尽可能均匀
- 使用分段方法进行动态物体过滤

### 2. Estimator（位姿估计器）

**功能**：执行紧耦合的激光-惯性里程计估计

**核心数据结构**：
- `LidarFrame`：存储滑动窗口中的激光雷达帧（点云、IMU积分器、位姿、速度、偏置等）
- `FeatureLine`：点-线特征（用于角点匹配）
- `FeaturePlan`：点-面特征（用于面特征匹配）
- `FeatureNon`：非特征点（用于ICP匹配）

**主要方法**：
- `EstimateLidarPose()`：估计激光雷达位姿
- `processPointToLine()`：处理点-线约束
- `processPointToPlan()`：处理点-面约束
- `processNonFeatureICP()`：处理非特征点ICP

### 3. MapManager（地图管理器）

**功能**：管理和维护全局特征地图

**核心特性**：
- 使用体素化网格存储地图点（21×11×21 网格）
- 维护三个独立的地图：角点地图、面特征地图、非特征地图
- 提供局部地图检索功能
- 支持地图保存为 PCD 文件

**关键方法**：
- `MapIncrement()`：增量添加新特征点到地图
- `MapMove()`：根据激光雷达位姿检索地图点
- `saveMapToPCD()`：将地图保存到 PCD 文件

### 4. IMUIntegrator（IMU 积分器）

**功能**：处理 IMU 数据预积分

**主要功能**：
- IMU 预积分计算
- 运动畸变补偿
- 提供 IMU 约束用于优化

## 技术栈

### 依赖库

- **ROS2 Foxy/Humble**：机器人操作系统框架（推荐使用 Humble）
- **Eigen3**：线性代数库
- **PCL（Point Cloud Library）**：点云处理库
- **Ceres Solver**：非线性优化库
- **OpenCV**：计算机视觉库
- **SuiteSparse**：稀疏矩阵求解库
- **glog**：Google 日志库

### 编程语言和标准

- **C++14**：主要编程语言，使用 C++14 标准
- **Python**：用于启动文件（ROS2 launch files）

## 配置参数说明

### Launch 文件参数

- **IMU_Mode**：IMU 信息融合策略
  - `0`：不使用 IMU，纯激光雷达里程计，使用恒定速度模型去除运动畸变
  - `1`：使用 IMU 预积分去除运动畸变
  - `2`：紧耦合 IMU 和激光雷达信息（推荐）

- **Extrinsic_Tlb**：激光雷达和 IMU 之间的外参（SE3 形式），12 元素向量

### 配置文件参数（YAML）

- **Lidar_Type**：激光雷达类型（0: Horizon, 1: HAP, 2: Mid-360）
- **Used_Line**：用于 LIO 的扫描线数量（1-6）
- **Feature_Mode**：特征提取模式（0/1）
- **Use_seg**：是否使用分割方法过滤动态物体（0/1）
- **NumCurvSize**：曲率计算窗口大小
- **DistanceFaraway**：远近距离阈值（米）
- **NumFlat**：每个部分的面特征数量
- **PartNum**：每个扫描的部分数量
- **FlatThreshold**：面特征曲率阈值
- **BreakCornerDis**：断点距离
- **LidarNearestDis**：最近距离阈值
- **KdTreeCornerOutlierDis**：角点异常值过滤阈值
- **map_skip_frame**：地图更新跳帧数

## 工作流程

### 数据流

1. **原始点云输入** → `/livox/lidar` (sensor_msgs/PointCloud2)
2. **IMU 数据输入** → `/livox/imu` (sensor_msgs/Imu)
3. **ScanRegistration 节点处理**：
   - 动态物体移除
   - 特征提取
   - 发布特征点云：`/livox_full_cloud`, `/livox_less_sharp_cloud`, `/livox_less_flat_cloud`, `/livox_nonfeature_cloud`
4. **PoseEstimation 节点处理**：
   - 运动畸变补偿
   - IMU 初始化（LO 模式）
   - 位姿估计（LIO 模式）
   - 地图构建
   - 发布里程计：`/livox_odometry_mapped`, `/livox_odometry_path_mapped`

### 初始化流程

1. **LO 模式**（LiDAR Odometry）：
   - 使用帧到模型的点云配准估计传感器位姿
   - 同时初始化 IMU 状态

2. **IMU 初始化**：
   - 使用最大后验概率（MAP）估计方法
   - 联合初始化 IMU 偏置、速度和重力方向
   - 无需特殊的初始化过程，支持任意运动

3. **切换到 LIO 模式**：
   - 初始化成功后切换到 LiDAR-Inertial Odometry 模式
   - 执行紧耦合传感器融合

## 编译和运行

> **推荐使用 Docker**: 我们提供了完整的 Docker 环境，可以在任何 Ubuntu 机器上快速部署。详细说明请参考 [环境安装文档](./doc/environment/README.md)。

### 编译

#### 使用 Docker（推荐）

```bash
cd /path/to/colcon_ws
docker-compose build
docker-compose up -d
docker-compose exec lio-livox bash

# 在容器内
cd /workspace
colcon build --packages-select lio_livox
source install/setup.bash
```

#### 本地编译

```bash
cd ~/colcon_ws
source /opt/ros/humble/setup.bash  # 或 source /opt/ros/foxy/setup.bash
colcon build --packages-select lio_livox
source install/setup.bash
```

> 详细的环境安装说明（包括依赖安装）请参考 [环境安装文档](./doc/environment/README.md)。

### 运行

**使用 bag 文件**：
```bash
ros2 launch lio_livox horizon.launch.py
ros2 bag play YOUR_ROSBAG.bag
```

**使用实际设备**：
```bash
# 首先启动激光雷达驱动
ros2 launch livox_ros_driver2 livox_lidar_msg.launch.py

# 然后启动 LIO-Livox
ros2 launch lio_livox horizon.launch.py
```

## 支持的硬件

- **Livox Horizon**：主要支持型号，适用于大规模室外环境
- **Livox HAP**：支持
- **Livox Mid-360**：支持，适用于机器人平台

## 性能特点

- **精度**：高精度定位，即使在交通拥堵情况下
- **鲁棒性**：对动态物体、隧道等挑战性环境具有良好鲁棒性
- **实时性**：实时位姿估计和建图
- **大规模**：支持大范围室外环境建图

## 测试模块

项目包含测试脚本：
- `test_final_logging.sh`：最终日志测试
- `test_logging.sh`：日志测试
- `test_unified_logging.sh`：统一日志测试
- `test_save_map.cpp`：地图保存测试
- `test_save_simple.cpp`：简单保存测试

## 日志系统

项目集成了统一的日志系统（`utils/logger.h`）：
- 支持不同日志级别（INFO, WARN, ERROR, DEBUG）
- 日志文件保存在指定目录
- 支持 ROS2 日志系统集成

## 地图保存功能

系统支持在退出时保存地图：
- 通过信号处理（SIGINT/SIGTERM）触发
- 保存为 PCD 格式文件
- 分别保存角点地图、面特征地图和非特征地图

## 相关资源

- **开发者**：Livox（览沃科技）
- **许可证**：BSD
- **依赖项目**：LOAM, VINS-Mono, LIO-mapping, ORB-SLAM3, LiLi-OM 等

## 关键技术点

1. **动态物体过滤**：使用欧几里得聚类和点云分割方法
2. **特征提取优化**：确保特征点分布均匀，避免退化
3. **紧耦合融合**：滑动窗口优化的激光-惯性融合
4. **鲁棒初始化**：基于 MAP 的初始化方法，无需特殊运动
5. **并行建图**：独立线程进行地图构建，不阻塞主估计线程

## 开发注意事项

1. **参数调优**：不同环境可能需要调整配置文件中的参数
2. **坐标系**：注意激光雷达和 IMU 的外参标定
3. **性能优化**：根据硬件性能调整滑动窗口大小和地图更新频率
4. **内存管理**：大规模建图时注意内存使用情况
