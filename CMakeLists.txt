cmake_minimum_required(VERSION 3.5)
project(lio_livox)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(PCL REQUIRED)
find_package(Ceres REQUIRED)
find_package(OpenCV REQUIRED)

# Livox ROS 2 driver (commonly livox_ros_driver2)
find_package(livox_ros_driver2 REQUIRED)

include_directories(
  include
  ${EIGEN3_INCLUDE_DIR}
  ${PCL_INCLUDE_DIRS}
  ${CERES_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

add_executable(ScanRegistration
  src/lio/ScanRegistration.cpp
  src/lio/LidarFeatureExtractor.cpp
  src/segment/segment.cpp
  src/segment/pointsCorrect.cpp
)
target_link_libraries(ScanRegistration
  ${PCL_LIBRARIES}
  ${OpenCV_LIBRARIES}
)
ament_target_dependencies(ScanRegistration
  rclcpp sensor_msgs geometry_msgs nav_msgs pcl_conversions tf2 tf2_ros tf2_geometry_msgs livox_ros_driver2
)

add_executable(PoseEstimation
  src/lio/PoseEstimation.cpp
  src/lio/Estimator.cpp
  src/lio/IMUIntegrator.cpp
  src/lio/ceresfunc.cpp
  src/lio/Map_Manager.cpp
)
target_link_libraries(PoseEstimation
  ${PCL_LIBRARIES}
  ${CERES_LIBRARIES}
)
ament_target_dependencies(PoseEstimation
  rclcpp sensor_msgs geometry_msgs nav_msgs pcl_conversions tf2 tf2_ros tf2_geometry_msgs
)

install(TARGETS
  ScanRegistration
  PoseEstimation
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch rviz_cfg config
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_package()
