#ifndef LIO_LIVOX_INCLUDE_MAPMANAGER_MAP_MANAGER_H_
#define LIO_LIVOX_INCLUDE_MAPMANAGER_MAP_MANAGER_H_
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <future>
class MapManager {
  typedef pcl::PointXYZINormal PointType;

 public:
  std::mutex mtx_map_manager;
  /** \brief constructor of MapManager */
  MapManager(const float& filter_corner, const float& filter_surf);

  static size_t ToIndex(int i, int j, int k);

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

  /** \brief transform point pi to the MAP coordinate
   * \param[in] pi: point to be transformed
   * \param[in] po: point after transfomation
   * \param[in] _transformTobeMapped: transform matrix between pi and po
   */
  static void pointAssociateToMap(PointType const* const pi,
                                  PointType* const po,
                                  const Eigen::Matrix4d& _transformTobeMapped);

  void featureAssociateToMap(
      const pcl::PointCloud<PointType>::Ptr& laserCloudCorner,
      const pcl::PointCloud<PointType>::Ptr& laserCloudSurf,
      const pcl::PointCloud<PointType>::Ptr& laserCloudNonFeature,
      const pcl::PointCloud<PointType>::Ptr& laserCloudCornerToMap,
      const pcl::PointCloud<PointType>::Ptr& laserCloudSurfToMap,
      const pcl::PointCloud<PointType>::Ptr& laserCloudNonFeatureToMap,
      const Eigen::Matrix4d& transformTobeMapped);
  /** \brief add new lidar points to the map
   * \param[in] laserCloudCornerStack: coner feature points that need to be
   * added to map \param[in] laserCloudSurfStack: surf feature points that need
   * to be added to map \param[in] transformTobeMapped: transform matrix of the
   * lidar pose
   */
  void MapIncrement(
      const pcl::PointCloud<PointType>::Ptr& laserCloudCornerStack,
      const pcl::PointCloud<PointType>::Ptr& laserCloudSurfStack,
      const pcl::PointCloud<PointType>::Ptr& laserCloudNonFeatureStack,
      const Eigen::Matrix4d& transformTobeMapped);

  /** \brief retrieve map points according to the lidar pose
   * \param[in] laserCloudCornerFromMap: store coner feature points retrieved
   * from map \param[in] laserCloudSurfFromMap: tore surf feature points
   * retrieved from map \param[in] transformTobeMapped: transform matrix of the
   * lidar pose
   */
  void MapMove(const Eigen::Matrix4d& transformTobeMapped);


  size_t FindUsedCornerMap(const PointType* p, int a, int b, int c);

  size_t FindUsedSurfMap(const PointType* p, int a, int b, int c);

  size_t FindUsedNonFeatureMap(const PointType* p, int a, int b, int c);

  pcl::KdTreeFLANN<PointType> getCornerKdMap(int i) {
    return corner_kd_map_last_[i];
  }
  pcl::KdTreeFLANN<PointType> getSurfKdMap(int i) {
    return surf_kd_map_last_[i];
  }
  pcl::KdTreeFLANN<PointType> getNonFeatureKdMap(int i) {
    return non_feature_kd_map_last_[i];
  }
  pcl::PointCloud<PointType>::Ptr get_corner_map() {
    return laser_cloud_corner_from_map_;
  }
  pcl::PointCloud<PointType>::Ptr get_surf_map() {
    return laser_cloud_surf_from_map_;
  }
  pcl::PointCloud<PointType>::Ptr get_nonfeature_map() {
    return laser_cloud_non_feature_from_map_;
  }
  int get_map_current_pos() {
    return current_update_pos_;
  }
  int get_laser_cloud_cen_width_last() {
    return laser_cloud_cen_width_last_;
  }
  int get_laser_cloud_cen_height_last() {
    return laser_cloud_cen_height_last_;
  }
  int get_laser_cloud_cen_depth_last() {
    return laser_cloud_cen_depth_last_;
  }
  pcl::PointCloud<PointType> laser_cloud_surf_for_match_[4851];
  pcl::PointCloud<PointType> laser_cloud_corner_for_match_[4851];
  pcl::PointCloud<PointType> laser_cloud_non_feature_for_match_[4851];

  /** \brief Save the complete map to PCD files
   * \param[in] output_dir: directory to save the map files
   */
  void saveMapToPCD(const std::string& output_dir);

 private:
  int laser_cloud_cen_width_ = 10;
  int laser_cloud_cen_height_ = 5;
  int laser_cloud_cen_depth_ = 10;

  int laser_cloud_cen_width_last_ = 10;
  int laser_cloud_cen_height_last_ = 5;
  int laser_cloud_cen_depth_last_ = 10;

  static const int kLaserCloudWidth = 21;
  static const int kLaserCloudHeight = 11;
  static const int kLaserCloudDepth = 21;
  static const int kLaserCloudNum =
      kLaserCloudWidth * kLaserCloudHeight * kLaserCloudDepth;  // 4851
  pcl::PointCloud<PointType>::Ptr laser_cloud_corner_array_[kLaserCloudNum];
  pcl::PointCloud<PointType>::Ptr laser_cloud_surf_array_[kLaserCloudNum];
  pcl::PointCloud<PointType>::Ptr
      laser_cloud_non_feature_array_[kLaserCloudNum];
  pcl::PointCloud<PointType>::Ptr
      laser_cloud_corner_array_stack_[kLaserCloudNum];
  pcl::PointCloud<PointType>::Ptr laser_cloud_surf_array_stack_[kLaserCloudNum];
  pcl::PointCloud<PointType>::Ptr
      laser_cloud_non_feature_array_stack_[kLaserCloudNum];

  pcl::VoxelGrid<PointType> down_size_filter_corner_;
  pcl::VoxelGrid<PointType> down_size_filter_surf_;
  pcl::VoxelGrid<PointType> down_size_filter_non_feature_;

  pcl::PointCloud<PointType>::Ptr laser_cloud_corner_from_map_;
  pcl::PointCloud<PointType>::Ptr laser_cloud_surf_from_map_;
  pcl::PointCloud<PointType>::Ptr laser_cloud_non_feature_from_map_;

  pcl::KdTreeFLANN<PointType>::Ptr
      laser_cloud_corner_kd_map_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType>::Ptr laser_cloud_surf_kd_map_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType>::Ptr
      laser_cloud_non_feature_kd_map_[kLaserCloudNum];

  pcl::KdTreeFLANN<PointType> corner_kd_map_copy_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType> surf_kd_map_copy_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType> non_feature_kd_map_copy_[kLaserCloudNum];

  pcl::KdTreeFLANN<PointType> corner_kd_map_last_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType> surf_kd_map_last_[kLaserCloudNum];
  pcl::KdTreeFLANN<PointType> non_feature_kd_map_last_[kLaserCloudNum];

  static const int kLocalMapWindowSize = 60;
  pcl::PointCloud<PointType>::Ptr local_corner_map_[kLocalMapWindowSize];
  pcl::PointCloud<PointType>::Ptr local_surf_map_[kLocalMapWindowSize];
  pcl::PointCloud<PointType>::Ptr
      local_non_feature_map_[kLocalMapWindowSize];

  int local_map_id_ = 0;

  int current_update_pos_ = 0;
  int estimator_pos_ = 0;
};

#endif  // LIO_LIVOX_INCLUDE_MAPMANAGER_MAP_MANAGER_H_
