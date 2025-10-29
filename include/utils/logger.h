#ifndef INCLUDE_UTILS_LOGGER_H_
#define INCLUDE_UTILS_LOGGER_H_

#include <glog/logging.h>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

namespace lio_livox {

/**
 * @brief 日志管理器类，负责初始化和管理glog日志系统
 */
class Logger {
 public:
  /**
   * @brief 初始化日志系统
   * @param program_name 程序名称
   * @param log_dir 日志目录路径
   * @param log_level 日志级别 (INFO, WARNING, ERROR, FATAL)
   * @param max_log_size 单个日志文件最大大小 (MB)
   * @param max_log_files 最大日志文件数量
   */
  static void Initialize(
      const std::string& program_name,
      const std::string& log_dir = "/home/charles/project/LIO-Livox/logs",
      const std::string& log_level = "INFO", int max_log_size = 100,
      int max_log_files = 10) {
    // 创建日志目录
    std::filesystem::create_directories(log_dir);

    // 生成创建时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d-%H%M%S");

    std::string timestamp = ss.str();

    // 设置日志文件路径格式：lio_livox_create-timestamp.log
    std::string unified_log = log_dir + "/lio_livox_" + timestamp + ".log";

    // 初始化glog
    google::InitGoogleLogging("LioLivox");

    // 设置日志级别
    if (log_level == "INFO") {
      FLAGS_minloglevel = google::GLOG_INFO;
    } else if (log_level == "WARNING") {
      FLAGS_minloglevel = google::GLOG_WARNING;
    } else if (log_level == "ERROR") {
      FLAGS_minloglevel = google::GLOG_ERROR;
    } else if (log_level == "FATAL") {
      FLAGS_minloglevel = google::GLOG_FATAL;
    }

    // 设置所有级别的日志都输出到同一个文件
    google::SetLogDestination(google::GLOG_INFO, unified_log.c_str());
    google::SetLogDestination(google::GLOG_WARNING, unified_log.c_str());
    google::SetLogDestination(google::GLOG_ERROR, unified_log.c_str());
    google::SetLogDestination(google::GLOG_FATAL, unified_log.c_str());

    // 设置日志格式
    FLAGS_logtostderr = false;       // 不输出到stderr
    FLAGS_alsologtostderr = false;   // 不同时输出到stderr
    FLAGS_colorlogtostderr = false;  // 不使用颜色
    FLAGS_log_prefix = true;         // 包含日志前缀
    FLAGS_logbufsecs = 0;            // 立即刷新日志缓冲区

    // 设置日志文件大小和数量限制
    FLAGS_max_log_size = max_log_size;  // MB
    // FLAGS_max_log_files = max_log_files;  // 这个标志在某些版本的glog中不存在

    // 设置日志格式
    FLAGS_log_year_in_prefix = true;
    FLAGS_log_utc_time = false;  // 使用本地时间

    // 禁用glog的自动文件名后缀
    FLAGS_log_link = "";  // 不创建符号链接

    LOG(INFO) << "Logger initialized for program: " << program_name;
    LOG(INFO) << "Log directory: " << log_dir;
    LOG(INFO) << "Log level: " << log_level;
    LOG(INFO) << "Unified log file: " << unified_log;
  }

  /**
   * @brief 关闭日志系统
   */
  static void Shutdown() {
    LOG(INFO) << "Shutting down logger...";
    google::ShutdownGoogleLogging();
  }

  /**
   * @brief 设置日志级别
   * @param level 日志级别
   */
  static void SetLogLevel(const std::string& level) {
    if (level == "INFO") {
      FLAGS_minloglevel = google::GLOG_INFO;
    } else if (level == "WARNING") {
      FLAGS_minloglevel = google::GLOG_WARNING;
    } else if (level == "ERROR") {
      FLAGS_minloglevel = google::GLOG_ERROR;
    } else if (level == "FATAL") {
      FLAGS_minloglevel = google::GLOG_FATAL;
    }
  }

  /**
   * @brief 启用/禁用控制台输出
   * @param enable 是否启用
   */
  static void EnableConsoleOutput(bool enable) {
    FLAGS_logtostderr = enable;
    FLAGS_alsologtostderr = enable;
  }
};

}  // namespace lio_livox

// 定义便捷的日志宏
#define LIO_LOG_INFO LOG(INFO)
#define LIO_LOG_WARNING LOG(WARNING)
#define LIO_LOG_ERROR LOG(ERROR)
#define LIO_LOG_FATAL LOG(FATAL)

// 定义条件日志宏
#define LIO_LOG_INFO_IF(condition) LOG_IF(INFO, condition)
#define LIO_LOG_WARNING_IF(condition) LOG_IF(WARNING, condition)
#define LIO_LOG_ERROR_IF(condition) LOG_IF(ERROR, condition)
#define LIO_LOG_FATAL_IF(condition) LOG_IF(FATAL, condition)

// 定义频率限制日志宏
#define LIO_LOG_INFO_EVERY_N(n) LOG_EVERY_N(INFO, n)
#define LIO_LOG_WARNING_EVERY_N(n) LOG_EVERY_N(WARNING, n)
#define LIO_LOG_ERROR_EVERY_N(n) LOG_EVERY_N(ERROR, n)

// 定义调试日志宏（仅在DEBUG模式下编译）
#ifdef DEBUG
#define LIO_LOG_DEBUG LOG(INFO)
#define LIO_LOG_DEBUG_IF(condition) LOG_IF(INFO, condition)
#define LIO_LOG_DEBUG_EVERY_N(n) LOG_EVERY_N(INFO, n)
#else
#define LIO_LOG_DEBUG \
  if (false) LOG(INFO)
#define LIO_LOG_DEBUG_IF(condition) \
  if (false) LOG_IF(INFO, condition)
#define LIO_LOG_DEBUG_EVERY_N(n) \
  if (false) LOG_EVERY_N(INFO, n)
#endif

#endif  // INCLUDE_UTILS_LOGGER_H_
