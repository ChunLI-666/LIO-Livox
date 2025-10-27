#!/bin/bash

# 测试统一日志系统
# 所有节点都会写入同一个日志文件：lio_livox_YYYYMMDD-HHMMSS.log

echo "========================================="
echo "Testing Unified Logging System"
echo "========================================="

# 设置日志目录
LOG_DIR="/home/charles/project/LIO-Livox/logs"

# 清理旧的日志文件
echo ""
echo "Cleaning old log files..."
rm -f ${LOG_DIR}/lio_livox_*.log*
rm -f ${LOG_DIR}/*.INFO

# 检查日志目录
echo ""
echo "Log directory: ${LOG_DIR}"
ls -lh ${LOG_DIR}/

echo ""
echo "========================================="
echo "Now starting ROS2 nodes..."
echo "All logs will be written to: ${LOG_DIR}/lio_livox_YYYYMMDD-HHMMSS.log"
echo "========================================="

# 注意：实际运行节点需要ROS2环境和数据源
# 这个脚本主要用于验证配置

echo ""
echo "Expected log file format:"
echo "  - Format: lio_livox_YYYYMMDD-HHMMSS.log"
echo "  - Example: lio_livox_20251026-215742.log"
echo ""
echo "All nodes (ScanRegistration, PoseEstimation) will write to the same file!"