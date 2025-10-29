#ifndef LIO_LIVOX_INCLUDE_LIDARFEATUREEXTRACTOR_LIDARFEATUREEXTRACTOR_H_
#define LIO_LIVOX_INCLUDE_LIDARFEATUREEXTRACTOR_LIDARFEATUREEXTRACTOR_H_
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <future>
#include "opencv2/core.hpp"
#include "segment/segment.hpp"
class LidarFeatureExtractor {
  typedef pcl::PointXYZINormal PointType;

 public:
  /** \brief constructor of LidarFeatureExtractor
   * \param[in] n_scans: lines used to extract lidar features
   */
  LidarFeatureExtractor(int n_scans, int NumCurvSize, float DistanceFaraway,
                        int NumFlat, int PartNum, float FlatThreshold,
                        float BreakCornerDis, float LidarNearestDis,
                        float KdTreeCornerOutlierDis);

  /** \brief transform float to int
   */
  static uint32_t FloatAsInt(float f) {
    union {
      uint32_t i;
      float f;
    } conv{};
    conv.f = f;
    return conv.i;
  }

  /** \brief transform int to float
   */
  static float IntAsFloat(uint32_t i) {
    union {
      float f;
      uint32_t i;
    } conv{};
    conv.i = i;
    return conv.f;
  }

  /** \brief Determine whether the point_list is flat
   * \param[in] point_list: points need to be judged
   * \param[in] plane_threshold
   */
  bool plane_judge(const std::vector<PointType>& point_list,
                   const int plane_threshold);

  /** \brief Detect lidar feature points
   * \param[in] cloud: original lidar cloud need to be detected
   * \param[in] pointsLessSharp: less sharp index of cloud
   * \param[in] pointsLessFlat: less flat index of cloud
   */
  void detectFeaturePoint(pcl::PointCloud<PointType>::Ptr& cloud,
                          std::vector<int>& pointsLessSharp,
                          std::vector<int>& pointsLessFlat);

  void detectFeaturePoint2(pcl::PointCloud<PointType>::Ptr& cloud,
                           pcl::PointCloud<PointType>::Ptr& pointsLessFlat,
                           pcl::PointCloud<PointType>::Ptr& pointsNonFeature);

  void detectFeaturePoint3(pcl::PointCloud<PointType>::Ptr& cloud,
                           std::vector<int>& pointsLessSharp);

  /** \brief Detect lidar feature points from PointCloud2
   * \param[in] msg: original PointCloud2 message
   * \param[in] laserCloud: transform PointCloud2 to pcl point cloud format
   * \param[in] laserConerFeature: less Coner features extracted from
   * laserCloud \param[in] laserSurfFeature: less Surf features extracted from
   * laserCloud
   */
  void FeatureExtract(const sensor_msgs::msg::PointCloud2::SharedPtr& msg,
                      pcl::PointCloud<PointType>::Ptr& laserCloud,
                      pcl::PointCloud<PointType>::Ptr& laserConerFeature,
                      pcl::PointCloud<PointType>::Ptr& laserSurfFeature,
                      int Used_Line = 1, const int lidar_type = 0);
  void FeatureExtract_Mid(
      pcl::PointCloud<pcl::PointXYZINormal>::Ptr& msg,
      pcl::PointCloud<PointType>::Ptr& laserConerFeature,
      pcl::PointCloud<PointType>::Ptr& laserSurfFeature);
 private:
  /** \brief lines used to extract lidar features */
  const int kNScans;

  /** \brief store original points of each line */
  std::vector<pcl::PointCloud<PointType>::Ptr> vlines_;

  /** \brief store corner feature index of each line */
  std::vector<std::vector<int>> vcorner_;

  /** \brief store surf feature index of each line */
  std::vector<std::vector<int>> vsurf_;

  int th_num_curv_size_;

  float th_distance_faraway_;

  int th_num_flat_;

  int th_part_num_;

  float th_flat_threshold_;

  float th_break_corner_dis_;

  float th_lidar_nearest_dis_;  
};

#endif  // LIO_LIVOX_INCLUDE_LIDARFEATUREEXTRACTOR_LIDARFEATUREEXTRACTOR_H_
