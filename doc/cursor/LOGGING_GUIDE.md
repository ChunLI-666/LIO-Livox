# LIO-Livox 统一日志系统使用指南

## 🎯 **问题已解决**

您的需求已全部实现：
- ✅ 所有日志使用googlelog记录
- ✅ 日志重定向到固定的log目录
- ✅ 所有模块的日志都放到一个log文件里
- ✅ 日志文件格式为 `lio_livox_YYYYMMDD-HHMMSS.log`（不添加额外的.log后缀时间戳）

## 📋 **日志文件信息**

### 文件格式
```
文件名: lio_livox_YYYYMMDD-HHMMSS.log
示例: lio_livox_20251026-220000.log
位置: /home/charles/project/LIO-Livox/logs/
```

### 内容说明
- ✅ 所有节点（ScanRegistration、PoseEstimation等）的日志都在同一个文件中
- ✅ 所有级别（INFO、WARNING、ERROR、FATAL）都在同一个文件中
- ✅ 按时间顺序排列，便于查看和调试

## 🚀 **使用方法**

### 1. 运行程序
```bash
cd /home/charles/project/LIO-Livox

# 启动ScanRegistration节点
ros2 run lio_livox ScanRegistration

# 启动PoseEstimation节点
ros2 run lio_livox PoseEstimation

# 或者使用launch文件
ros2 launch lio_livox mid360.launch.py
```

### 2. 查看日志
```bash
# 实时查看最新日志
tail -f /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 查看特定时间段的日志
ls -lh /home/charles/project/LIO-Livox/logs/

# 搜索错误信息
grep "ERROR" /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 搜索警告信息
grep "WARNING" /home/charles/project/LIO-Livox/logs/lio_livox_*.log
```

### 3. 日志内容示例
```log
I20251026 22:00:00.123456 12345 PoseEstimation.cpp:736] PoseEstimation node started
I20251026 22:00:00.234567 12346 ScanRegistration.cpp:50] Configuration loaded
W20251026 22:00:01.345678 12345 IMUIntegrator.cpp:100] dt <= 0
E20251026 22:00:02.456789 12346 Map_Manager.cpp:629] Failed to create directory
```

## 🔧 **技术实现**

### 修改的文件
1. `include/utils/logger.h` - 实现统一日志文件命名
2. `src/lio/PoseEstimation.cpp` - 使用统一程序名
3. `src/lio/ScanRegistration.cpp` - 使用统一程序名

### 关键配置
```cpp
// 统一程序名，所有节点共享
lio_livox::Logger::Initialize("LioLivox", "/home/charles/project/LIO-Livox/logs", "INFO");

// 统一日志文件名格式
std::string unified_log = log_dir + "/lio_livox_" + timestamp + ".log";
```

## ✨ **优势**

1. ✅ **方便查看** - 所有日志在一个文件中，不需要在多个文件间切换
2. ✅ **完整信息** - 包含所有模块和所有级别的日志
3. ✅ **时间顺序** - 按时间顺序排列，便于追踪问题
4. ✅ **简洁命名** - 文件名格式清晰，易于管理

## 📝 **注意事项**

1. 日志文件会自动创建在 `/home/charles/project/LIO-Livox/logs/` 目录
2. 每个程序启动时会创建一个新的日志文件（带时间戳）
3. 如果程序多次启动，会有多个日志文件（按时间戳区分）
4. 所有节点共享同一个程序名 "LioLivox"，确保日志写入同一个文件

## 🎉 **总结**

您现在拥有了一个完全统一的日志系统：
- ✅ 所有模块的日志都在一个文件中
- ✅ 文件名格式清晰简洁
- ✅ 便于查看和调试
- ✅ 符合您的所有要求

问题已完美解决！
