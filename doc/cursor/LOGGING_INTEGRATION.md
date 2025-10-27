# LIO-Livox 日志系统集成说明

## 概述

本项目已成功集成Google Logging (glog)库，将所有原有的日志输出（std::cout、std::cerr、RCLCPP_*等）统一替换为glog日志系统，并配置了固定的日志目录输出。

## 主要特性

### 1. 统一的日志管理
- **日志库**: Google Logging (glog) v0.6.0
- **日志目录**: `/home/charles/project/LIO-Livox/logs`
- **日志级别**: INFO, WARNING, ERROR, FATAL, DEBUG
- **文件格式**: `{程序名}_{级别}_{时间戳}.log`

### 2. 日志文件分类
- **统一日志文件**: `lio_livox_YYYYMMDD_HHMMSS_毫秒.log20251026-215650.2634723`
- **所有级别**: INFO, WARNING, ERROR, FATAL 都写入同一个文件
- **符号链接**: glog会自动创建符号链接方便访问

### 3. 日志配置
- **文件大小限制**: 100MB
- **时间戳格式**: YYYYMMDD_HHMMSS_毫秒
- **日志前缀**: 包含时间戳和程序名
- **缓冲区刷新**: 立即刷新，确保日志实时写入

## 代码修改详情

### 1. 配置文件修改

#### CMakeLists.txt
```cmake
# 添加glog依赖
find_package(glog REQUIRED)

# 添加包含目录
include_directories(${GLOG_INCLUDE_DIRS})

# 链接glog库
target_link_libraries(ScanRegistration ${GLOG_LIBRARIES})
target_link_libraries(PoseEstimation ${GLOG_LIBRARIES})
```

#### package.xml
```xml
<build_depend>libgoogle-glog-dev</build_depend>
<exec_depend>libgoogle-glog0v5</exec_depend>
```

### 2. 新增文件

#### include/utils/logger.h
统一的日志管理头文件，提供：
- 日志系统初始化函数
- 便捷的日志宏定义
- 日志级别控制
- 控制台输出开关

### 3. 源文件修改

#### 修改的文件列表
- `src/lio/PoseEstimation.cpp` - 主程序文件
- `src/lio/Map_Manager.cpp` - 地图管理器
- `src/lio/ScanRegistration.cpp` - 扫描注册
- `src/lio/Estimator.cpp` - 估计器
- `src/lio/IMUIntegrator.cpp` - IMU积分器
- `src/segment/segment.cpp` - 点云分割
- `src/segment/pointsCorrect.cpp` - 点云校正

#### 日志调用替换示例
```cpp
// 原来的代码
std::cout << "Processing frame " << frame_id << std::endl;
RCLCPP_INFO(this->get_logger(), "Processing frame %d", frame_id);

// 替换后的代码
LIO_LOG_INFO << "Processing frame " << frame_id;
```

## 使用方法

### 1. 在代码中使用日志

```cpp
#include "utils/logger.h"

// 信息日志
LIO_LOG_INFO << "程序启动成功";

// 警告日志
LIO_LOG_WARNING << "检测到异常情况: " << error_code;

// 错误日志
LIO_LOG_ERROR << "初始化失败: " << error_message;

// 致命错误日志
LIO_LOG_FATAL << "系统崩溃，无法继续运行";

// 调试日志（仅在DEBUG模式下编译）
LIO_LOG_DEBUG << "调试信息: " << debug_data;

// 条件日志
LIO_LOG_INFO_IF(condition) << "条件满足时的日志";

// 频率限制日志
LIO_LOG_INFO_EVERY_N(100) << "每100次执行一次的日志";
```

### 2. 程序启动时的日志初始化

```cpp
int main(int argc, char** argv) {
    // 初始化日志系统
    lio_livox::Logger::Initialize("ProgramName", "/path/to/logs", "INFO");
    
    // 程序逻辑...
    
    // 关闭日志系统
    lio_livox::Logger::Shutdown();
    return 0;
}
```

### 3. 运行时日志级别控制

```cpp
// 动态调整日志级别
lio_livox::Logger::SetLogLevel("WARNING");  // 只显示WARNING及以上级别

// 启用/禁用控制台输出
lio_livox::Logger::EnableConsoleOutput(true);  // 同时输出到控制台
```

## 日志文件管理

### 1. 日志文件位置
```
/home/charles/project/LIO-Livox/logs/
├── lio_livox_20251026_215650_734.log20251026-215650.2634723  # 统一日志文件
├── LioLivoxTest.INFO -> lio_livox_20251026_215650_734.log20251026-215650.2634723  # 符号链接
└── ...
```

### 2. 日志文件内容示例
```
I1026 21:47:17.123456 12345 PoseEstimation.cpp:736] PoseEstimation node started. Press Ctrl+C to save map and exit.
I1026 21:47:17.234567 12345 PoseEstimation.cpp:517] [PoseEstimation] Processing new lidar frame, time: 1234567890.123456, LidarIMUInited: false
W1026 21:47:17.345678 12345 PoseEstimation.cpp:600] [PoseEstimation] No IMU data but LidarIMUInited=true, skipping this frame
```

### 3. 日志文件清理
- 日志文件大小达到100MB时会自动轮转
- 建议定期清理旧的日志文件
- 可以使用logrotate等工具进行日志管理

## 构建和运行

### 1. 构建项目
```bash
cd /home/charles/project/LIO-Livox
colcon build --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### 2. 运行程序
```bash
# 运行PoseEstimation
ros2 run lio_livox PoseEstimation

# 运行ScanRegistration
ros2 run lio_livox ScanRegistration
```

### 3. 查看日志
```bash
# 实时查看日志
tail -f /home/charles/project/LIO-Livox/logs/PoseEstimation_info_*.log

# 查看所有日志文件
ls -la /home/charles/project/LIO-Livox/logs/

# 搜索特定日志
grep "ERROR" /home/charles/project/LIO-Livox/logs/*.log
```

## 故障排除

### 1. 常见问题

#### 编译错误
- 确保已安装glog库: `sudo apt install libgoogle-glog-dev`
- 检查CMakeLists.txt中的glog配置

#### 日志文件未生成
- 检查日志目录权限: `chmod 755 /home/charles/project/LIO-Livox/logs`
- 确保程序有写入权限

#### 日志级别不正确
- 检查Logger::Initialize()调用中的日志级别参数
- 使用Logger::SetLogLevel()动态调整

### 2. 调试技巧

#### 启用控制台输出
```cpp
lio_livox::Logger::EnableConsoleOutput(true);
```

#### 查看glog内部状态
```cpp
// 在代码中添加
LOG(INFO) << "Glog initialized successfully";
```

#### 检查日志文件权限
```bash
ls -la /home/charles/project/LIO-Livox/logs/
```

## 性能考虑

### 1. 日志性能优化
- 使用条件日志避免不必要的字符串构造
- 在发布版本中考虑禁用DEBUG日志
- 定期清理旧日志文件

### 2. 磁盘空间管理
- 监控日志目录大小
- 设置日志文件大小限制
- 考虑使用日志压缩

## 扩展功能

### 1. 自定义日志格式
可以修改`logger.h`中的日志格式设置：
```cpp
FLAGS_log_prefix = true;
FLAGS_log_year_in_prefix = true;
FLAGS_log_utc_time = false;
```

### 2. 日志过滤
可以添加日志过滤功能，根据模块或标签过滤日志。

### 3. 远程日志
可以扩展日志系统支持远程日志服务器。

## 总结

通过集成glog日志系统，LIO-Livox项目现在具有：
- ✅ 统一的日志管理
- ✅ 结构化的日志输出
- ✅ 灵活的日志级别控制
- ✅ 自动的日志文件管理
- ✅ 良好的调试支持

所有原有的日志输出都已成功迁移到glog系统，日志文件将自动保存到`/home/charles/project/LIO-Livox/logs`目录中。
