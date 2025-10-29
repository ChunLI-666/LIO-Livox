#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

typedef pcl::PointXYZINormal PointType;

void testSaveMap() {
  // Create output directory if it doesn't exist
  std::string output_dir = "/home/charles/project/LIO-Livox/mapping_results";
  std::string mkdir_cmd = "mkdir -p " + output_dir;
  int result = system(mkdir_cmd.c_str());
  if (result != 0) {
    std::cerr << "Failed to create directory: " << output_dir << std::endl;
    return;
  }

  // Get current timestamp for unique filenames
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
  std::string timestamp = ss.str();

  // Create a test point cloud
  pcl::PointCloud<PointType> testMap;
  PointType point;
  point.x = 1.0;
  point.y = 2.0;
  point.z = 3.0;
  point.intensity = 100.0;
  testMap.push_back(point);

  point.x = 4.0;
  point.y = 5.0;
  point.z = 6.0;
  point.intensity = 200.0;
  testMap.push_back(point);

  // Save test map
  std::string full_map_path = output_dir + "/test_map_" + timestamp + ".pcd";
  if (pcl::io::savePCDFileBinary(full_map_path, testMap) == -1) {
    std::cerr << "Failed to save test map to: " << full_map_path << std::endl;
  } else {
    std::cout << "Test map saved to: " << full_map_path << std::endl;
    std::cout << "Total points: " << testMap.size() << std::endl;
  }
}

int main() {
  std::cout << "Testing PCD save functionality..." << std::endl;
  testSaveMap();
  std::cout << "Test completed!" << std::endl;
  return 0;
}
