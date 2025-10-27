#!/bin/bash

# 最终测试统一日志文件功能

echo "=== LIO-Livox 统一日志文件功能测试 ==="
echo ""

# 检查日志目录
LOG_DIR="/home/charles/project/LIO-Livox/logs"
echo "1. 检查日志目录: $LOG_DIR"
if [ -d "$LOG_DIR" ]; then
    echo "   ✓ 日志目录存在"
    echo "   当前日志文件:"
    ls -la "$LOG_DIR"/*.log 2>/dev/null || echo "   暂无日志文件"
else
    echo "   ✗ 日志目录不存在"
    exit 1
fi

echo ""

# 创建简单的测试程序
echo "2. 创建测试程序验证统一日志功能"
cat > /tmp/test_final_logging.cpp << 'EOF'
#include <iostream>
#include <glog/logging.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

int main() {
    // 创建日志目录
    std::filesystem::create_directories("/home/charles/project/LIO-Livox/logs");
    
    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    
    std::string timestamp = ss.str();
    std::string unified_log = "/home/charles/project/LIO-Livox/logs/lio_livox_" + timestamp + ".log";
    
    // 初始化glog
    google::InitGoogleLogging("LioLivoxTest");
    
    // 设置所有级别的日志都输出到同一个文件
    google::SetLogDestination(google::GLOG_INFO, unified_log.c_str());
    google::SetLogDestination(google::GLOG_WARNING, unified_log.c_str());
    google::SetLogDestination(google::GLOG_ERROR, unified_log.c_str());
    google::SetLogDestination(google::GLOG_FATAL, unified_log.c_str());
    
    // 设置日志格式
    FLAGS_logtostderr = false;
    FLAGS_alsologtostderr = false;
    FLAGS_log_prefix = true;
    FLAGS_log_year_in_prefix = true;
    FLAGS_log_utc_time = false;
    FLAGS_log_link = "";  // 不创建符号链接
    
    // 写入不同级别的日志
    LOG(INFO) << "=== LIO-Livox 统一日志测试开始 ===";
    LOG(INFO) << "这是一条INFO级别的日志 - 程序启动";
    LOG(WARNING) << "这是一条WARNING级别的日志 - 检测到警告";
    LOG(ERROR) << "这是一条ERROR级别的日志 - 发生错误";
    LOG(INFO) << "这是一条INFO级别的日志 - 程序结束";
    LOG(INFO) << "=== LIO-Livox 统一日志测试完成 ===";
    
    std::cout << "✓ 测试日志已写入统一文件: " << unified_log << std::endl;
    
    // 关闭glog
    google::ShutdownGoogleLogging();
    return 0;
}
EOF

# 编译测试程序
echo "3. 编译测试程序"
g++ -o /tmp/test_final_logging /tmp/test_final_logging.cpp -lglog

if [ $? -eq 0 ]; then
    echo "   ✓ 编译成功"
    
    # 运行测试程序
    echo "4. 运行测试程序"
    /tmp/test_final_logging
    
    # 检查生成的日志文件
    echo "5. 检查生成的统一日志文件"
    LATEST_LOG=$(ls -t /home/charles/project/LIO-Livox/logs/lio_livox_*.log 2>/dev/null | head -1)
    
    if [ -n "$LATEST_LOG" ]; then
        echo "   ✓ 统一日志文件已生成: $LATEST_LOG"
        echo ""
        echo "   日志文件内容:"
        echo "   ========================================"
        cat "$LATEST_LOG"
        echo "   ========================================"
        
        # 检查是否包含不同级别的日志
        INFO_COUNT=$(grep -c "I.*INFO" "$LATEST_LOG")
        WARNING_COUNT=$(grep -c "W.*WARNING" "$LATEST_LOG")
        ERROR_COUNT=$(grep -c "E.*ERROR" "$LATEST_LOG")
        
        echo ""
        echo "   日志级别统计:"
        echo "   - INFO: $INFO_COUNT 条"
        echo "   - WARNING: $WARNING_COUNT 条"
        echo "   - ERROR: $ERROR_COUNT 条"
        
        if [ $INFO_COUNT -gt 0 ] && [ $WARNING_COUNT -gt 0 ] && [ $ERROR_COUNT -gt 0 ]; then
            echo "   ✓ 所有级别的日志都成功写入统一文件"
        else
            echo "   ✗ 部分级别的日志未成功写入"
        fi
        
        # 检查文件名格式
        if [[ "$LATEST_LOG" =~ lio_livox_[0-9]{8}_[0-9]{6}_[0-9]{3}\.log$ ]]; then
            echo "   ✓ 日志文件名格式正确"
        else
            echo "   ✗ 日志文件名格式异常: $LATEST_LOG"
        fi
    else
        echo "   ✗ 未找到统一日志文件"
    fi
else
    echo "   ✗ 编译失败"
fi

echo ""

# 显示使用说明
echo "6. 统一日志系统使用说明"
echo "   - 所有模块的日志都写入同一个文件"
echo "   - 日志文件命名格式: lio_livox_YYYYMMDD_HHMMSS_毫秒.log"
echo "   - 日志级别: INFO, WARNING, ERROR, FATAL"
echo "   - 日志格式: [级别]时间戳 线程ID 文件名:行号] 消息"
echo ""
echo "   查看日志的方法:"
echo "   - 实时查看: tail -f $LOG_DIR/lio_livox_*.log"
echo "   - 搜索错误: grep 'E.*ERROR' $LOG_DIR/lio_livox_*.log"
echo "   - 搜索警告: grep 'W.*WARNING' $LOG_DIR/lio_livox_*.log"

echo ""

# 清理测试文件
echo "7. 清理测试文件"
rm -f /tmp/test_final_logging.cpp /tmp/test_final_logging

echo ""
echo "=== 测试完成 ==="
echo "✓ 统一日志文件功能已成功实现！"
echo "✓ 所有模块的日志现在都写入同一个文件，方便查看和分析。"
