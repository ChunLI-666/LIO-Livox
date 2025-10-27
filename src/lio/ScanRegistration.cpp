#include "LidarFeatureExtractor/LidarFeatureExtractor.h"
#include "utils/logger.h"

typedef pcl::PointXYZINormal PointType;

class ScanRegistrationNode : public rclcpp::Node
{
public:
    ScanRegistrationNode() : Node("scan_registration")
    {
        // Declare parameters
        this->declare_parameter("config_file", std::string(""));
        this->declare_parameter("msg_type", 0);
        
        // Get parameters
        std::string config_file;
        int msg_type = 0;
        this->get_parameter("config_file", config_file);
        this->get_parameter("msg_type", msg_type);
        
        // Load config
        cv::FileStorage fsSettings(config_file, cv::FileStorage::READ);
        if (!fsSettings.isOpened()) {
            LIO_LOG_ERROR << "config_file error: cannot open " << config_file;
            return;
        }
        
        Lidar_Type = static_cast<int>(fsSettings["Lidar_Type"]);
        N_SCANS = static_cast<int>(fsSettings["Used_Line"]);
        Feature_Mode = static_cast<int>(fsSettings["Feature_Mode"]);
        Use_seg = static_cast<int>(fsSettings["Use_seg"]);
        
        int NumCurvSize = static_cast<int>(fsSettings["NumCurvSize"]);
        float DistanceFaraway = static_cast<float>(fsSettings["DistanceFaraway"]);
        int NumFlat = static_cast<int>(fsSettings["NumFlat"]);
        int PartNum = static_cast<int>(fsSettings["PartNum"]);
        float FlatThreshold = static_cast<float>(fsSettings["FlatThreshold"]);
        float BreakCornerDis = static_cast<float>(fsSettings["BreakCornerDis"]);
        float LidarNearestDis = static_cast<float>(fsSettings["LidarNearestDis"]);
        float KdTreeCornerOutlierDis = static_cast<float>(fsSettings["KdTreeCornerOutlierDis"]);
        
        // Initialize point clouds
        laserCloud.reset(new pcl::PointCloud<PointType>);
        laserConerCloud.reset(new pcl::PointCloud<PointType>);
        laserSurfCloud.reset(new pcl::PointCloud<PointType>);
        laserNonFeatureCloud.reset(new pcl::PointCloud<PointType>);
        
        // Create publishers
        pubFullLaserCloud = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox_full_cloud", 10);
        pubSharpCloud = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox_less_sharp_cloud", 10);
        pubFlatCloud = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox_less_flat_cloud", 10);
        pubNonFeature = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox_nonfeature_cloud", 10);
        
        // Create subscription based on lidar type
        if (Lidar_Type == 0 || Lidar_Type == 1) {
            // For Horizon and HAP, subscribe to PointCloud2
            subPointCloud = this->create_subscription<sensor_msgs::msg::PointCloud2>(
                "/livox/lidar", 100, 
                std::bind(&ScanRegistrationNode::pointCloudCallback, this, std::placeholders::_1));
        } else if (Lidar_Type == 2) {
            if (msg_type == 0) {
                // Custom message type
                subPointCloud = this->create_subscription<sensor_msgs::msg::PointCloud2>(
                    "/livox/lidar", 100, 
                    std::bind(&ScanRegistrationNode::pointCloudCallback, this, std::placeholders::_1));
            } else if (msg_type == 1) {
                // Standard PointCloud2
                subPointCloud = this->create_subscription<sensor_msgs::msg::PointCloud2>(
                    "/livox/lidar", 100, 
                    std::bind(&ScanRegistrationNode::pointCloudCallback, this, std::placeholders::_1));
            }
        }
        
        // Initialize feature extractor
        lidarFeatureExtractor = new LidarFeatureExtractor(N_SCANS, NumCurvSize, DistanceFaraway, NumFlat, PartNum,
                                                          FlatThreshold, BreakCornerDis, LidarNearestDis, KdTreeCornerOutlierDis);
    }
    
    ~ScanRegistrationNode()
    {
        delete lidarFeatureExtractor;
    }

private:
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubFullLaserCloud;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubSharpCloud;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubFlatCloud;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubNonFeature;
    
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr subPointCloud;
    
    LidarFeatureExtractor* lidarFeatureExtractor;
    pcl::PointCloud<PointType>::Ptr laserCloud;
    pcl::PointCloud<PointType>::Ptr laserConerCloud;
    pcl::PointCloud<PointType>::Ptr laserSurfCloud;
    pcl::PointCloud<PointType>::Ptr laserNonFeatureCloud;
    int Lidar_Type = 0;
    int N_SCANS = 6;
    bool Feature_Mode = false;
    bool Use_seg = false;
    
    void pointCloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
    {
        pcl::PointCloud<pcl::PointXYZI>::Ptr laser_cloud(new pcl::PointCloud<pcl::PointXYZI>());
        pcl::PointCloud<pcl::PointXYZINormal>::Ptr laser_cloud_custom(new pcl::PointCloud<pcl::PointXYZINormal>());
        
        pcl::fromROSMsg(*msg, *laser_cloud);
        
        for (uint64_t i = 0; i < laser_cloud->points.size(); i++)
        {
            auto p = laser_cloud->points.at(i);
            pcl::PointXYZINormal p_custom;
            if(Lidar_Type == 0 || Lidar_Type == 1)
            {
                if(p.x < 0.01) continue;
            }
            else if(Lidar_Type == 2)
            {
                if(std::fabs(p.x) < 0.01) continue;
            }
            p_custom.x = p.x;
            p_custom.y = p.y;
            p_custom.z = p.z;
            p_custom.intensity = p.intensity;
            p_custom.normal_x = float(i) / float(laser_cloud->points.size());
            p_custom.normal_y = i % 4;
            laser_cloud_custom->points.push_back(p_custom);
        }
        
        lidarFeatureExtractor->FeatureExtract_Mid(laser_cloud_custom, laserConerCloud, laserSurfCloud);
        
        // publish
        sensor_msgs::msg::PointCloud2 laserCloudMsg;
        pcl::toROSMsg(*laser_cloud_custom, laserCloudMsg);
        laserCloudMsg.header = msg->header;
        pubFullLaserCloud->publish(laserCloudMsg);
    }
};

// Global variables for backward compatibility (will be removed)
rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubFullLaserCloud;
rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubSharpCloud;
rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubFlatCloud;
rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubNonFeature;

int main(int argc, char** argv)
{
    // Initialize logging system
    lio_livox::Logger::Initialize("LioLivox", "/home/charles/project/LIO-Livox/logs", "INFO");
    
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ScanRegistrationNode>();
    rclcpp::spin(node);
    
    // Shutdown logging system
    lio_livox::Logger::Shutdown();
    
    rclcpp::shutdown();
    return 0;
}

