# LIO-Livox 统一日志系统使用说明

## 🎯 **问题解决**

您之前提到的问题："logs是按照模块分的，能不能都放到一个log文件里，这样看很不方便" 已经成功解决！

## ✅ **现在的日志系统**

### 统一日志文件
- **所有模块的日志**都写入**同一个文件**
- **所有级别的日志**（INFO、WARNING、ERROR、FATAL）都在**同一个文件**中
- **方便查看和分析**，不需要在多个文件之间切换

### 日志文件格式
```
文件名: lio_livox_YYYYMMDD_HHMMSS_毫秒.log20251026-215650.2634723
内容示例:
I20251026 21:56:50.734382 2634723 PoseEstimation.cpp:736] PoseEstimation node started
W20251026 21:56:50.734556 2634723 PoseEstimation.cpp:530] Failed to fetch IMU data
E20251026 21:56:50.734597 2634723 Map_Manager.cpp:629] Failed to create directory
```

## 📋 **如何使用**

### 1. 查看日志
```bash
# 实时查看最新日志
tail -f /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 查看所有日志内容
cat /home/charles/project/LIO-Livox/logs/lio_livox_*.log

# 搜索特定内容
grep "ERROR" /home/charles/project/LIO-Livox/logs/lio_livox_*.log
grep "WARNING" /home/charles/project/LIO-Livox/logs/lio_livox_*.log
```

### 2. 日志级别说明
- **I** = INFO（信息）
- **W** = WARNING（警告）
- **E** = ERROR（错误）
- **F** = FATAL（致命错误）

### 3. 日志格式说明
```
[级别]时间戳 线程ID 文件名:行号] 消息内容
```

## 🔧 **技术实现**

### 修改内容
1. **更新了logger.h** - 将所有级别的日志都指向同一个文件
2. **保持了所有原有功能** - 日志级别、时间戳、格式化等
3. **简化了文件管理** - 只需要关注一个日志文件

### 代码示例
```cpp
// 在代码中使用（无需修改）
LIO_LOG_INFO << "程序启动";
LIO_LOG_WARNING << "检测到警告";
LIO_LOG_ERROR << "发生错误";
```

## 📁 **文件位置**

```
/home/charles/project/LIO-Livox/logs/
├── lio_livox_20251026_215650_734.log20251026-215650.2634723  # 统一日志文件
└── LioLivoxTest.INFO -> lio_livox_20251026_215650_734.log20251026-215650.2634723  # 符号链接
```

## 🎉 **优势**

1. **✅ 统一管理** - 所有日志在一个文件中
2. **✅ 方便查看** - 不需要在多个文件间切换
3. **✅ 完整信息** - 包含所有模块和级别的日志
4. **✅ 时间顺序** - 按时间顺序排列，便于追踪问题
5. **✅ 保持功能** - 所有原有功能都保留

## 🚀 **立即使用**

现在运行LIO-Livox程序时，所有日志都会自动写入统一文件：

```bash
# 运行程序
ros2 run lio_livox PoseEstimation

# 在另一个终端查看日志
tail -f /home/charles/project/LIO-Livox/logs/lio_livox_*.log
```

**问题已完美解决！** 🎯
