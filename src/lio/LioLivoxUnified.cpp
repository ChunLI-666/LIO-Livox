#define UNIFIED_BUILD  // Define this macro before including source files

#include <ceres/ceres.h>
#include <ceres/rotation.h>
#include <ceres/local_parameterization.h>
#include "utils/logger.h"
#include "utils/ceresfunc.h"
#include "Estimator/Estimator.h"
#include "LidarFeatureExtractor/LidarFeatureExtractor.h"
#include <boost/shared_ptr.hpp>
#include <list>
#include <memory>
#include <csignal>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

// Forward declarations
class ScanRegistrationNode;
class PoseEstimationNode;

// Global variables for signal handling
PoseEstimationNode* g_node = nullptr;
std::shared_ptr<Estimator> g_estimator = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LIO_LOG_INFO << "Received signal " << signal << ". Saving map before exit...";

        if (g_estimator != nullptr) {
            std::string output_dir = "/home/user/project/LIO-Livox/mapping_results";
            g_estimator->saveMapToPCD(output_dir);
        }

        LIO_LOG_INFO << "Map saved. Exiting...";
        rclcpp::shutdown();
        exit(0);
    }
}

// Include the node implementations (they will skip their main functions due to UNIFIED_BUILD macro)
// Note: We need to be careful about multiple definitions
#include "ScanRegistration.cpp"
#include "PoseEstimation.cpp"

int main(int argc, char** argv)
{
    // Initialize logging system once for both nodes
    lio_livox::Logger::Initialize("LioLivoxUnified", "/home/user/project/LIO-Livox/logs", "INFO");

    rclcpp::init(argc, argv);

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    LIO_LOG_INFO << "Starting LioLivoxUnified - launching both ScanRegistration and PoseEstimation nodes...";

    // Create both nodes
    // Note: Both nodes share the same parameter namespace since they're in the same process
    // ScanRegistrationNode reads: config_file, msg_type
    // PoseEstimationNode reads: IMU_Mode, filter_parameter_corner, filter_parameter_surf, Extrinsic_Tlb
    auto scan_node = std::make_shared<ScanRegistrationNode>();
    auto pose_node = std::make_shared<PoseEstimationNode>();

    // Set the global node pointer for signal handler
    g_node = pose_node.get();

    // g_estimator is set in PoseEstimationNode constructor (line 111 in PoseEstimation.cpp)
    // Since constructor is synchronous, g_estimator should be set immediately after pose_node creation
    if (g_estimator == nullptr) {
        LIO_LOG_WARNING << "g_estimator is nullptr after PoseEstimationNode creation - map saving may not work!";
    } else {
        LIO_LOG_INFO << "g_estimator successfully initialized";
    }

    LIO_LOG_INFO << "Both nodes created successfully. Starting multi-threaded executor...";

    // Use multi-threaded executor to handle both nodes concurrently
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(scan_node);
    executor.add_node(pose_node);

    LIO_LOG_INFO << "LioLivoxUnified is now running. Press Ctrl+C to save map and exit.";

    // Spin the executor
    executor.spin();

    // Save map on normal exit
    if (g_estimator != nullptr) {
        LIO_LOG_INFO << "Saving map on normal exit...";
        std::string output_dir = "/home/user/project/LIO-Livox/mapping_results";
        g_estimator->saveMapToPCD(output_dir);
    }

    // Shutdown logging system
    lio_livox::Logger::Shutdown();

    rclcpp::shutdown();
    return 0;
}
