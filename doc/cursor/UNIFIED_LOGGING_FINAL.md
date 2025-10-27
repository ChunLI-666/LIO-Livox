# LIO-Livox 统一日志系统

## ✅ **问题已解决**

您现在有了一个**统一的日志系统**，所有节点（ScanRegistration、PoseEstimation等）的日志都会写入**同一个文件**中。

## 📋 **日志文件格式**

```
文件名格式: lio_livox_YYYYMMDD-HHMMSS.log
实际文件名: lio_livox_20251026-220000.log
```

## 🎯 **关键改进**

1. ✅ **统一程序名** - 所有节点使用相同的程序名 "LioLivox"
2. ✅ **统一日志文件** - 所有级别（INFO、WARNING、ERROR、FATAL）都写入同一个文件
3. ✅ **简洁的文件名** - 格式为 `lio_livox_YYYYMMDD-HHMMSS.log`
4. ✅ **无额外时间戳后缀** - glog不再在文件后缀添加额外的时间戳

## 📁 **日志文件位置**

```
/home/charles/project/LIO-Livox/logs/lio_livox_20251026-220000.log
```

## 🚀 **如何使用**

### 1. 运行程序
```bash
# 启动所有节点（使用launch文件）
ros2 launch lio_livox your_launch_file.launch.py
```

### 2. 查看日志
```bash
# 实时查看最新日志
tail -f /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 查看所有日志
cat /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 搜索错误
grep "ERROR" /home/charles/project/LIO-Livox/logs/lio_livox_*.log
```

## 📊 **日志内容示例**

```log
I20251026 22:00:00.123456 12345 PoseEstimation.cpp:736] PoseEstimation node started
I20251026 22:00:00.234567 12346 ScanRegistration.cpp:50] Configuration loaded
W20251026 22:00:01.345678 12345 IMUIntegrator.cpp:100] dt <= 0
E20251026 22:00:02.456789 12346 Map_Manager.cpp:629] Failed to create directory
```

## 🔧 **技术实现**

### 修改的文件：
1. **logger.h** - 统一日志文件命名格式
2. **PoseEstimation.cpp** - 使用统一的程序名
3. **ScanRegistration.cpp** - 使用统一的程序名

### 关键配置：
```cpp
// 统一程序名
lio_livox::Logger::Initialize("LioLivox", "/home/charles/project/LIO-Livox/logs", "INFO");

// 日志文件格式
std::string unified_log = log_dir + "/lio_livox_" + timestamp + ".log";
```

## ✨ **优势**

1. ✅ **方便查看** - 所有日志在一个文件中，不需要切换多个文件
2. ✅ **完整信息** - 包含所有节点和所有级别的日志
3. ✅ **时间顺序** - 按时间顺序排列，便于追踪问题
4. ✅ **简洁命名** - 文件名清晰，易于识别和管理

## 🎉 **问题已完美解决！**

现在您的LIO-Livox系统具有：
- ✅ 统一的日志文件
- ✅ 简洁的文件名格式
- ✅ 所有节点共享一个日志文件
- ✅ 易于查看和调试

可以开始使用了！
