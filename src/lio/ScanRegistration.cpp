// ROS 2 Foxy port
#include "LidarFeatureExtractor/LidarFeatureExtractor.h"
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <livox_ros_driver2/msg/custom_msg.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <opencv2/core.hpp>

using livox_ros_driver2::msg::CustomMsg;
using sensor_msgs::msg::PointCloud2;
typedef pcl::PointXYZINormal PointType;

class ScanRegistrationNode : public rclcpp::Node {
public:
  ScanRegistrationNode() : rclcpp::Node("ScanRegistration") {
    // Declare and get parameters
    std::string config_file = this->declare_parameter<std::string>("config_file", "");
    int msg_type = this->declare_parameter<int>("msg_type", 0);

    cv::FileStorage fsSettings(config_file, cv::FileStorage::READ);
    if (!fsSettings.isOpened()) {
      RCLCPP_ERROR(this->get_logger(), "Cannot open config_file: %s", config_file.c_str());
      throw std::runtime_error("config_file open failed");
    }
    lidar_type_ = static_cast<int>(fsSettings["Lidar_Type"]);
    n_scans_ = static_cast<int>(fsSettings["Used_Line"]);
    feature_mode_ = static_cast<int>(fsSettings["Feature_Mode"]);
    use_seg_ = static_cast<int>(fsSettings["Use_seg"]);

    int NumCurvSize = static_cast<int>(fsSettings["NumCurvSize"]);
    float DistanceFaraway = static_cast<float>(fsSettings["DistanceFaraway"]);
    int NumFlat = static_cast<int>(fsSettings["NumFlat"]);
    int PartNum = static_cast<int>(fsSettings["PartNum"]);
    float FlatThreshold = static_cast<float>(fsSettings["FlatThreshold"]);
    float BreakCornerDis = static_cast<float>(fsSettings["BreakCornerDis"]);
    float LidarNearestDis = static_cast<float>(fsSettings["LidarNearestDis"]);
    float KdTreeCornerOutlierDis = static_cast<float>(fsSettings["KdTreeCornerOutlierDis"]);

    laserCloud_.reset(new pcl::PointCloud<PointType>);
    laserConerCloud_.reset(new pcl::PointCloud<PointType>);
    laserSurfCloud_.reset(new pcl::PointCloud<PointType>);
    laserNonFeatureCloud_.reset(new pcl::PointCloud<PointType>);

    pubFullLaserCloud_ = this->create_publisher<PointCloud2>("/livox_full_cloud", rclcpp::SensorDataQoS());
    pubSharpCloud_ = this->create_publisher<PointCloud2>("/livox_less_sharp_cloud", rclcpp::SensorDataQoS());
    pubFlatCloud_ = this->create_publisher<PointCloud2>("/livox_less_flat_cloud", rclcpp::SensorDataQoS());
    pubNonFeature_ = this->create_publisher<PointCloud2>("/livox_nonfeature_cloud", rclcpp::SensorDataQoS());

    lidarFeatureExtractor_ = std::make_unique<LidarFeatureExtractor>(
        n_scans_, NumCurvSize, DistanceFaraway, NumFlat, PartNum, FlatThreshold, BreakCornerDis, LidarNearestDis,
        KdTreeCornerOutlierDis);

    auto qos = rclcpp::SensorDataQoS();
    if (lidar_type_ == 0) {
      subCustom_ = this->create_subscription<CustomMsg>(
          "/livox/lidar", qos, std::bind(&ScanRegistrationNode::lidarCallBackHorizon, this, std::placeholders::_1));
    } else if (lidar_type_ == 1) {
      subCustom_ = this->create_subscription<CustomMsg>(
          "/livox/lidar", qos, std::bind(&ScanRegistrationNode::lidarCallBackHAP, this, std::placeholders::_1));
    } else if (lidar_type_ == 2) {
      if (msg_type == 0) {
        subCustom_ = this->create_subscription<CustomMsg>(
            "/livox/lidar", qos, std::bind(&ScanRegistrationNode::lidarCallBackHorizon, this, std::placeholders::_1));
      } else if (msg_type == 1) {
        subPc2_ = this->create_subscription<PointCloud2>(
            "/livox/lidar", qos, std::bind(&ScanRegistrationNode::lidarCallBackPc2, this, std::placeholders::_1));
      }
    }
  }

private:
  void publishCloudFromPclWithLivoxStamp(const CustomMsg::ConstSharedPtr &msg) {
    if (laserCloud_->empty()) return;
    PointCloud2 laserCloudMsg;
    pcl::toROSMsg(*laserCloud_, laserCloudMsg);
    laserCloudMsg.header = msg->header;
    if (!msg->points.empty()) {
      uint64_t ns = msg->timebase + msg->points.back().offset_time;
      laserCloudMsg.header.stamp.sec = static_cast<int32_t>(ns / 1000000000ULL);
      laserCloudMsg.header.stamp.nanosec = static_cast<uint32_t>(ns % 1000000000ULL);
    }
    pubFullLaserCloud_->publish(laserCloudMsg);
  }

  void lidarCallBackHorizon(const CustomMsg::ConstSharedPtr msg) {
    PointCloud2 msg2;
    if (use_seg_) {
      lidarFeatureExtractor_->FeatureExtract_with_segment(msg, laserCloud_, laserConerCloud_, laserSurfCloud_,
                                                         laserNonFeatureCloud_, msg2, n_scans_);
    } else {
      lidarFeatureExtractor_->FeatureExtract(msg, laserCloud_, laserConerCloud_, laserSurfCloud_, n_scans_, lidar_type_);
    }
    publishCloudFromPclWithLivoxStamp(msg);
  }

  void lidarCallBackHAP(const CustomMsg::ConstSharedPtr msg) {
    PointCloud2 msg2;
    if (use_seg_) {
      lidarFeatureExtractor_->FeatureExtract_with_segment_hap(msg, laserCloud_, laserConerCloud_, laserSurfCloud_,
                                                             laserNonFeatureCloud_, msg2, n_scans_);
    } else {
      lidarFeatureExtractor_->FeatureExtract_hap(msg, laserCloud_, laserConerCloud_, laserSurfCloud_,
                                                laserNonFeatureCloud_, n_scans_);
    }
    publishCloudFromPclWithLivoxStamp(msg);
  }

  void lidarCallBackPc2(const PointCloud2::ConstSharedPtr msg) {
    pcl::PointCloud<pcl::PointXYZI>::Ptr laser_cloud(new pcl::PointCloud<pcl::PointXYZI>());
    pcl::PointCloud<pcl::PointXYZINormal>::Ptr laser_cloud_custom(new pcl::PointCloud<pcl::PointXYZINormal>());
    pcl::fromROSMsg(*msg, *laser_cloud);

    for (uint64_t i = 0; i < laser_cloud->points.size(); i++) {
      auto p = laser_cloud->points.at(i);
      pcl::PointXYZINormal p_custom;
      if (lidar_type_ == 0 || lidar_type_ == 1) {
        if (p.x < 0.01) continue;
      } else if (lidar_type_ == 2) {
        if (std::fabs(p.x) < 0.01) continue;
      }
      p_custom.x = p.x;
      p_custom.y = p.y;
      p_custom.z = p.z;
      p_custom.intensity = p.intensity;
      p_custom.normal_x = float(i) / float(laser_cloud->points.size());
      p_custom.normal_y = i % 4;
      laser_cloud_custom->points.push_back(p_custom);
    }

    lidarFeatureExtractor_->FeatureExtract_Mid(laser_cloud_custom, laserConerCloud_, laserSurfCloud_);

    PointCloud2 laserCloudMsg;
    pcl::toROSMsg(*laser_cloud_custom, laserCloudMsg);
    laserCloudMsg.header = msg->header;
    pubFullLaserCloud_->publish(laserCloudMsg);
  }

private:
  // Params
  int lidar_type_ {0};
  int n_scans_ {6};
  bool feature_mode_ {false};
  bool use_seg_ {false};

  // Publishers
  rclcpp::Publisher<PointCloud2>::SharedPtr pubFullLaserCloud_;
  rclcpp::Publisher<PointCloud2>::SharedPtr pubSharpCloud_;
  rclcpp::Publisher<PointCloud2>::SharedPtr pubFlatCloud_;
  rclcpp::Publisher<PointCloud2>::SharedPtr pubNonFeature_;

  // Subscribers
  rclcpp::Subscription<CustomMsg>::SharedPtr subCustom_;
  rclcpp::Subscription<PointCloud2>::SharedPtr subPc2_;

  // Data holders
  std::unique_ptr<LidarFeatureExtractor> lidarFeatureExtractor_;
  pcl::PointCloud<PointType>::Ptr laserCloud_;
  pcl::PointCloud<PointType>::Ptr laserConerCloud_;
  pcl::PointCloud<PointType>::Ptr laserSurfCloud_;
  pcl::PointCloud<PointType>::Ptr laserNonFeatureCloud_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<ScanRegistrationNode>();
    rclcpp::spin(node);
  } catch (const std::exception &e) {
    // Node failed to start, still shutdown cleanly
  }
  rclcpp::shutdown();
  return 0;
}

