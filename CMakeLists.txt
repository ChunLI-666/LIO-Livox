cmake_minimum_required(VERSION 3.8)
project(lio_livox)

# Generate compile_commands.json for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(message_filters REQUIRED)
find_package(std_srvs REQUIRED)
find_package(visualization_msgs REQUIRED)

find_package(Eigen3 REQUIRED)
find_package(PCL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Ceres REQUIRED)
find_package(glog REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include
                   ${EIGEN3_INCLUDE_DIR}
                   ${PCL_INCLUDE_DIRS}
                   ${CERES_INCLUDE_DIRS}
                   ${OpenCV_INCLUDE_DIRS}
                   ${SUITESPARSE_INCLUDE_DIRS}
                   ${GLOG_INCLUDE_DIRS})

# ScanRegistration executable
add_executable(ScanRegistration 
               src/lio/ScanRegistration.cpp
               src/lio/LidarFeatureExtractor.cpp
               src/segment/segment.cpp
               src/segment/pointsCorrect.cpp)

ament_target_dependencies(ScanRegistration
  rclcpp
  geometry_msgs
  nav_msgs
  sensor_msgs
  std_msgs
  tf2_ros
  tf2_geometry_msgs
  pcl_conversions
  pcl_ros
  message_filters
  std_srvs
  visualization_msgs
)

target_link_libraries(ScanRegistration 
  ${PCL_LIBRARIES} 
  ${OpenCV_LIBRARIES}
  ${CERES_LIBRARIES}
  ${GLOG_LIBRARIES}
)

# PoseEstimation executable
add_executable(PoseEstimation 
               src/lio/PoseEstimation.cpp
               src/lio/Estimator.cpp
               src/lio/IMUIntegrator.cpp
               src/lio/ceresfunc.cpp 
               src/lio/Map_Manager.cpp)

ament_target_dependencies(PoseEstimation
  rclcpp
  geometry_msgs
  nav_msgs
  sensor_msgs
  std_msgs
  tf2_ros
  tf2_geometry_msgs
  pcl_conversions
  pcl_ros
  message_filters
  std_srvs
  visualization_msgs
)

target_link_libraries(PoseEstimation 
  ${PCL_LIBRARIES}
  ${CERES_LIBRARIES}
  ${GLOG_LIBRARIES}
)

# Install executables
install(TARGETS
  ScanRegistration
  PoseEstimation
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/
)

# Install config files
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}/
)

# Install rviz config
install(DIRECTORY rviz_cfg
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()



