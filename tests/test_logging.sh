#!/bin/bash

# LIO-Livox 日志系统测试脚本

echo "=== LIO-Livox 日志系统测试 ==="
echo ""

# 检查日志目录
LOG_DIR="/home/charles/project/LIO-Livox/logs"
echo "1. 检查日志目录: $LOG_DIR"
if [ -d "$LOG_DIR" ]; then
    echo "   ✓ 日志目录存在"
    echo "   目录内容:"
    ls -la "$LOG_DIR"
else
    echo "   ✗ 日志目录不存在"
    exit 1
fi

echo ""

# 检查可执行文件
echo "2. 检查可执行文件"
BUILD_DIR="/home/charles/project/LIO-Livox/build/lio_livox"
if [ -f "$BUILD_DIR/ScanRegistration" ]; then
    echo "   ✓ ScanRegistration 可执行文件存在"
else
    echo "   ✗ ScanRegistration 可执行文件不存在"
fi

if [ -f "$BUILD_DIR/PoseEstimation" ]; then
    echo "   ✓ PoseEstimation 可执行文件存在"
else
    echo "   ✗ PoseEstimation 可执行文件不存在"
fi

echo ""

# 检查glog库
echo "3. 检查glog库"
if pkg-config --exists libglog; then
    echo "   ✓ glog库已安装"
    echo "   版本信息:"
    pkg-config --modversion libglog
else
    echo "   ✗ glog库未安装"
fi

echo ""

# 创建测试日志
echo "4. 创建测试日志文件"
TEST_LOG="$LOG_DIR/test_$(date +%Y%m%d_%H%M%S).log"
echo "测试日志内容 - $(date)" > "$TEST_LOG"
echo "   ✓ 测试日志文件已创建: $TEST_LOG"

echo ""

# 显示日志配置信息
echo "5. 日志配置信息"
echo "   日志目录: $LOG_DIR"
echo "   日志级别: INFO"
echo "   日志格式: 包含时间戳和程序名"
echo "   文件大小限制: 100MB"
echo "   日志文件命名: {程序名}_{级别}_{时间戳}.log"

echo ""

# 显示使用说明
echo "6. 使用说明"
echo "   在代码中使用日志:"
echo "   - LIO_LOG_INFO << \"信息日志\";"
echo "   - LIO_LOG_WARNING << \"警告日志\";"
echo "   - LIO_LOG_ERROR << \"错误日志\";"
echo "   - LIO_LOG_FATAL << \"致命错误日志\";"
echo "   - LIO_LOG_DEBUG << \"调试日志\"; (仅在DEBUG模式下编译)"

echo ""

echo "=== 测试完成 ==="
