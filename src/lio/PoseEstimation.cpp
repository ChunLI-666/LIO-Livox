#include <ceres/ceres.h>
#include <ceres/rotation.h>
#include "utils/ceresfunc.h"
#include "utils/logger.h"
#include "Estimator/Estimator.h"
#include <boost/shared_ptr.hpp>
#include <list>
#include <memory>
#include <csignal>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
typedef pcl::PointXYZINormal PointType;

// Forward declaration
class PoseEstimationNode;

// Global variables for signal handling
PoseEstimationNode* g_node = nullptr;
std::shared_ptr<Estimator> g_estimator = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LIO_LOG_INFO << "Received signal " << signal << ". Saving map before exit...";
        
        if (g_estimator != nullptr) {
            std::string output_dir = "/home/charles/project/LIO-Livox/mapping_results";
            g_estimator->saveMapToPCD(output_dir);
        }
        
        LIO_LOG_INFO << "Map saved. Exiting...";
        rclcpp::shutdown();
        exit(0);
    }
}

class PoseEstimationNode : public rclcpp::Node
{
public:
    PoseEstimationNode() : Node("pose_estimation")
    {
        // Declare parameters
        this->declare_parameter("filter_parameter_corner", 0.2);
        this->declare_parameter("filter_parameter_surf", 0.4);
        this->declare_parameter("IMU_Mode", 2);
        this->declare_parameter("Extrinsic_Tlb", std::vector<double>());
        
        // Get parameters
        this->get_parameter("filter_parameter_corner", filter_parameter_corner);
        this->get_parameter("filter_parameter_surf", filter_parameter_surf);
        this->get_parameter("IMU_Mode", IMU_Mode);
        
        std::vector<double> vecTlb;
        this->get_parameter("Extrinsic_Tlb", vecTlb);
        
        // Validate extrinsic parameter vector size
        if (vecTlb.size() < 12) {
            LIO_LOG_ERROR << "Extrinsic_Tlb parameter must have at least 12 elements, got " << vecTlb.size();
            throw std::runtime_error("Invalid Extrinsic_Tlb parameter size");
        }
        
        // Set extrinsic matrix between lidar & IMU
        Eigen::Matrix3d R;
        Eigen::Vector3d t;
        R << vecTlb[0], vecTlb[1], vecTlb[2],
             vecTlb[4], vecTlb[5], vecTlb[6],
             vecTlb[8], vecTlb[9], vecTlb[10];
        t << vecTlb[3], vecTlb[7], vecTlb[11];
        Eigen::Quaterniond qr(R);
        R = qr.normalized().toRotationMatrix();
        exTlb.topLeftCorner(3,3) = R;
        exTlb.topRightCorner(3,1) = t;
        exRlb = R;
        exRbl = R.transpose();
        exPlb = t;
        exPbl = -1.0 * exRbl * exPlb;
        
        // Create publishers
        pubLaserOdometry = this->create_publisher<nav_msgs::msg::Odometry>("/livox_odometry_mapped", 5);
        pubLaserOdometryPath = this->create_publisher<nav_msgs::msg::Path>("/livox_odometry_path_mapped", 5);
        pubFullLaserCloud = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox_full_cloud_mapped", 10);
        pubGps = this->create_publisher<sensor_msgs::msg::NavSatFix>("/lidar", 1000);
        
        // Create subscribers
        subFullCloud = this->create_subscription<sensor_msgs::msg::PointCloud2>(
            "/livox_full_cloud", 10, 
            std::bind(&PoseEstimationNode::fullCallBack, this, std::placeholders::_1));
        
        if(IMU_Mode > 0) {
            sub_imu = this->create_subscription<sensor_msgs::msg::Imu>(
                "/livox/imu", 2000, 
                std::bind(&PoseEstimationNode::imu_callback, this, std::placeholders::_1));
        }
        
        if(IMU_Mode < 2)
            WINDOWSIZE = 1;
        else
            WINDOWSIZE = 20;
        
        // Initialize transform broadcaster
        tfBroadcaster = std::make_shared<tf2_ros::TransformBroadcaster>(this);
        
        // Initialize other components
        laserCloudFullRes.reset(new pcl::PointCloud<PointType>);
        estimator = new Estimator(filter_parameter_corner, filter_parameter_surf);
        g_estimator = std::shared_ptr<Estimator>(estimator, [](Estimator*){});
        lidarFrameList.reset(new std::list<Estimator::LidarFrame>);
        
        // Start processing thread
        process_thread = std::thread(&PoseEstimationNode::process, this);
    }
    
    ~PoseEstimationNode()
    {
        if(process_thread.joinable()) {
            process_thread.join();
        }
        delete estimator;
    }

private:
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pubLaserOdometry;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr pubLaserOdometryPath;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pubFullLaserCloud;
    rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr pubGps;
    
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr subFullCloud;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr sub_imu;
    
    std::shared_ptr<tf2_ros::TransformBroadcaster> tfBroadcaster;
    
    std::thread process_thread;
    
    // Member variables
    int WINDOWSIZE;
    bool LidarIMUInited = false;
    std::shared_ptr<std::list<Estimator::LidarFrame>> lidarFrameList;
    pcl::PointCloud<PointType>::Ptr laserCloudFullRes;
    Estimator* estimator;
    
    bool newfullCloud = false;
    Eigen::Matrix4d transformAftMapped = Eigen::Matrix4d::Identity();
    
    std::mutex _mutexLidarQueue;
    std::queue<sensor_msgs::msg::PointCloud2::SharedPtr> _lidarMsgQueue;
    std::mutex _mutexIMUQueue;
    std::queue<sensor_msgs::msg::Imu::SharedPtr> _imuMsgQueue;
    Eigen::Matrix4d exTlb;
    Eigen::Matrix3d exRlb, exRbl;
    Eigen::Vector3d exPlb, exPbl;
    Eigen::Vector3d GravityVector;
    float filter_parameter_corner = 0.2;
    float filter_parameter_surf = 0.4;
    int IMU_Mode = 2;
    sensor_msgs::msg::NavSatFix gps;
    int pushCount = 0;
    double startTime = 0;
    
    nav_msgs::msg::Path laserOdoPath;

    /** \brief publish odometry infomation
      * \param[in] newPose: pose to be published
      * \param[in] timefullCloud: time stamp
      */
    void pubOdometry(const Eigen::Matrix4d& newPose, double& timefullCloud){
        nav_msgs::msg::Odometry laserOdometry;

        Eigen::Matrix3d Rcurr = newPose.topLeftCorner(3, 3);
        Eigen::Quaterniond newQuat(Rcurr);
        Eigen::Vector3d newPosition = newPose.topRightCorner(3, 1);
        laserOdometry.header.frame_id = "world";
        laserOdometry.child_frame_id = "livox_frame";
        laserOdometry.header.stamp = rclcpp::Time(static_cast<int64_t>(timefullCloud * 1e9));
        laserOdometry.pose.pose.orientation.x = newQuat.x();
        laserOdometry.pose.pose.orientation.y = newQuat.y();
        laserOdometry.pose.pose.orientation.z = newQuat.z();
        laserOdometry.pose.pose.orientation.w = newQuat.w();
        laserOdometry.pose.pose.position.x = newPosition.x();
        laserOdometry.pose.pose.position.y = newPosition.y();
        laserOdometry.pose.pose.position.z = newPosition.z();
        pubLaserOdometry->publish(laserOdometry);

        geometry_msgs::msg::PoseStamped laserPose;
        laserPose.header = laserOdometry.header;
        laserPose.pose = laserOdometry.pose.pose;
        laserOdoPath.header.stamp = laserOdometry.header.stamp;
        laserOdoPath.poses.push_back(laserPose);
        laserOdoPath.header.frame_id = "world";
        pubLaserOdometryPath->publish(laserOdoPath);

        geometry_msgs::msg::TransformStamped laserOdometryTrans;
        laserOdometryTrans.header.frame_id = "world";
        laserOdometryTrans.child_frame_id = "livox_frame";
        laserOdometryTrans.header.stamp = rclcpp::Time(static_cast<int64_t>(timefullCloud * 1e9));
        laserOdometryTrans.transform.translation.x = newPosition.x();
        laserOdometryTrans.transform.translation.y = newPosition.y();
        laserOdometryTrans.transform.translation.z = newPosition.z();
        laserOdometryTrans.transform.rotation.x = newQuat.x();
        laserOdometryTrans.transform.rotation.y = newQuat.y();
        laserOdometryTrans.transform.rotation.z = newQuat.z();
        laserOdometryTrans.transform.rotation.w = newQuat.w();
        tfBroadcaster->sendTransform(laserOdometryTrans);

        gps.header.stamp = rclcpp::Time(static_cast<int64_t>(timefullCloud * 1e9));
        gps.header.frame_id = "world";
        gps.latitude = newPosition.x();
        gps.longitude = newPosition.y();
        gps.altitude = newPosition.z();
        gps.position_covariance = {
                        Rcurr(0, 0), Rcurr(1, 0), Rcurr(2, 0),
                        Rcurr(0, 1), Rcurr(1, 1), Rcurr(2, 1),
                        Rcurr(0, 2), Rcurr(1, 2), Rcurr(2, 2)
        };
        pubGps->publish(gps);
    }

    void fullCallBack(const sensor_msgs::msg::PointCloud2::SharedPtr msg){
        // push lidar msg to queue
        std::unique_lock<std::mutex> lock(_mutexLidarQueue);
        _lidarMsgQueue.push(msg);
    }

    void imu_callback(const sensor_msgs::msg::Imu::SharedPtr imu_msg){
        // push IMU msg to queue
        std::unique_lock<std::mutex> lock(_mutexIMUQueue);
        _imuMsgQueue.push(imu_msg);
    }

    /** \brief get IMU messages in a certain time interval
      * \param[in] startTime: left boundary of time interval
      * \param[in] endTime: right boundary of time interval
      * \param[in] vimuMsg: store IMU messages
      */
    bool fetchImuMsgs(double startTime, double endTime, std::vector<sensor_msgs::msg::Imu::SharedPtr> &vimuMsg){
        std::unique_lock<std::mutex> lock(_mutexIMUQueue);
        double current_time = 0;
        vimuMsg.clear();
        while(true)
        {
            if(_imuMsgQueue.empty())
                break;
            if(_imuMsgQueue.back()->header.stamp.sec + _imuMsgQueue.back()->header.stamp.nanosec * 1e-9 < endTime ||
               _imuMsgQueue.front()->header.stamp.sec + _imuMsgQueue.front()->header.stamp.nanosec * 1e-9 >= endTime)
                break;
            sensor_msgs::msg::Imu::SharedPtr& tmpimumsg = _imuMsgQueue.front();
            double time = tmpimumsg->header.stamp.sec + tmpimumsg->header.stamp.nanosec * 1e-9;
            if(time<=endTime && time>startTime){
                vimuMsg.push_back(tmpimumsg);
                current_time = time;
                _imuMsgQueue.pop();
                if(time == endTime) break;
            } else{
                if(time<=startTime){
                    _imuMsgQueue.pop();
                } else{
                    double dt_1 = endTime - current_time;
                    double dt_2 = time - endTime;
                    assert(dt_1 >= 0);
                    assert(dt_2 >= 0);
                    assert(dt_1 + dt_2 > 0);
                    double w1 = dt_2 / (dt_1 + dt_2);
                    double w2 = dt_1 / (dt_1 + dt_2);
                    auto theLastIMU = std::make_shared<sensor_msgs::msg::Imu>();
                    theLastIMU->linear_acceleration.x = w1 * vimuMsg.back()->linear_acceleration.x + w2 * tmpimumsg->linear_acceleration.x;
                    theLastIMU->linear_acceleration.y = w1 * vimuMsg.back()->linear_acceleration.y + w2 * tmpimumsg->linear_acceleration.y;
                    theLastIMU->linear_acceleration.z = w1 * vimuMsg.back()->linear_acceleration.z + w2 * tmpimumsg->linear_acceleration.z;
                    theLastIMU->angular_velocity.x = w1 * vimuMsg.back()->angular_velocity.x + w2 * tmpimumsg->angular_velocity.x;
                    theLastIMU->angular_velocity.y = w1 * vimuMsg.back()->angular_velocity.y + w2 * tmpimumsg->angular_velocity.y;
                    theLastIMU->angular_velocity.z = w1 * vimuMsg.back()->angular_velocity.z + w2 * tmpimumsg->angular_velocity.z;
                    theLastIMU->header.stamp = rclcpp::Time(static_cast<int64_t>(endTime * 1e9));
                    vimuMsg.emplace_back(theLastIMU);
                    break;
                }
            }
        }
        return !vimuMsg.empty();
    }

/** \brief Remove Lidar Distortion
  * \param[in] cloud: lidar cloud need to be undistorted
  * \param[in] dRlc: delta rotation
  * \param[in] dtlc: delta displacement
  */
void RemoveLidarDistortion(pcl::PointCloud<PointType>::Ptr& cloud,
                           const Eigen::Matrix3d& dRlc, const Eigen::Vector3d& dtlc){
  int PointsNum = cloud->points.size();
  for (int i = 0; i < PointsNum; i++) {
    Eigen::Vector3d startP;
    float s = cloud->points[i].normal_x;
    Eigen::Quaterniond qlc = Eigen::Quaterniond(dRlc).normalized();
    Eigen::Quaterniond delta_qlc = Eigen::Quaterniond::Identity().slerp(s, qlc).normalized();
    const Eigen::Vector3d delta_Plc = s * dtlc;
    startP = delta_qlc * Eigen::Vector3d(cloud->points[i].x,cloud->points[i].y,cloud->points[i].z) + delta_Plc;
    Eigen::Vector3d _po = dRlc.transpose() * (startP - dtlc);

    cloud->points[i].x = _po(0);
    cloud->points[i].y = _po(1);
    cloud->points[i].z = _po(2);
    cloud->points[i].normal_x = 1.0;
  }
}


bool TryMAPInitialization() {

  Eigen::Vector3d average_acc = -lidarFrameList->begin()->imuIntegrator.GetAverageAcc();
  double info_g = std::fabs(9.805 - average_acc.norm());
  average_acc = average_acc * 9.805 / average_acc.norm();

  // calculate the initial gravity direction
  double para_quat[4];
  para_quat[0] = 1;
  para_quat[1] = 0;
  para_quat[2] = 0;
  para_quat[3] = 0;


  ceres::Manifold *quatParam = new ceres::QuaternionManifold();
  ceres::Problem problem_quat;
  
  problem_quat.AddParameterBlock(para_quat, 4, quatParam);

  problem_quat.AddResidualBlock(Cost_Initial_G::Create(average_acc),
                                nullptr,
                                para_quat);

  ceres::Solver::Options options_quat;
  ceres::Solver::Summary summary_quat;
  ceres::Solve(options_quat, &problem_quat, &summary_quat);

  Eigen::Quaterniond q_wg(para_quat[0], para_quat[1], para_quat[2], para_quat[3]);


  //build prior factor of LIO initialization
  Eigen::Vector3d prior_r = Eigen::Vector3d::Zero();
  Eigen::Vector3d prior_ba = Eigen::Vector3d::Zero();
  Eigen::Vector3d prior_bg = Eigen::Vector3d::Zero();
  std::vector<Eigen::Vector3d> prior_v;
  int v_size = lidarFrameList->size();
  for(int i = 0; i < v_size; i++) {
    prior_v.push_back(Eigen::Vector3d::Zero());
  }
  Sophus::SO3d SO3_R_wg(q_wg.toRotationMatrix());
  prior_r = SO3_R_wg.log();
  
  for (int i = 1; i < v_size; i++){
    auto iter = lidarFrameList->begin();
    auto iter_next = lidarFrameList->begin();
    std::advance(iter, i-1);
    std::advance(iter_next, i);

    Eigen::Vector3d velo_imu = (iter_next->P - iter->P + iter_next->Q*exPlb - iter->Q*exPlb) / (iter_next->timeStamp - iter->timeStamp);
    prior_v[i] = velo_imu;
  }
  prior_v[0] = prior_v[1];

  double para_v[v_size][3];
  double para_r[3];
  double para_ba[3];
  double para_bg[3];

  for(int i = 0; i < 3; i++) {
    para_r[i] = 0;
    para_ba[i] = 0;
    para_bg[i] = 0;
  }

  for(int i = 0; i < v_size; i++) {
    for(int j = 0; j < 3; j++) {
      para_v[i][j] = prior_v[i][j];
    }
  }

  Eigen::Matrix<double, 3, 3> sqrt_information_r = 2000.0 * Eigen::Matrix<double, 3, 3>::Identity();
  Eigen::Matrix<double, 3, 3> sqrt_information_ba = 1000.0 * Eigen::Matrix<double, 3, 3>::Identity();
  Eigen::Matrix<double, 3, 3> sqrt_information_bg = 4000.0 * Eigen::Matrix<double, 3, 3>::Identity();
  Eigen::Matrix<double, 3, 3> sqrt_information_v = 4000.0 * Eigen::Matrix<double, 3, 3>::Identity();

  ceres::Problem::Options problem_options;
  ceres::Problem problem(problem_options);
  problem.AddParameterBlock(para_r, 3);
  problem.AddParameterBlock(para_ba, 3);
  problem.AddParameterBlock(para_bg, 3);
  for(int i = 0; i < v_size; i++) {
    problem.AddParameterBlock(para_v[i], 3);
  }
  
  // add CostFunction
  problem.AddResidualBlock(Cost_Initialization_Prior_R::Create(prior_r, sqrt_information_r),
                           nullptr,
                           para_r);
  
  problem.AddResidualBlock(Cost_Initialization_Prior_bv::Create(prior_ba, sqrt_information_ba),
                           nullptr,
                           para_ba);
  problem.AddResidualBlock(Cost_Initialization_Prior_bv::Create(prior_bg, sqrt_information_bg),
                           nullptr,
                           para_bg);

  for(int i = 0; i < v_size; i++) {
    problem.AddResidualBlock(Cost_Initialization_Prior_bv::Create(prior_v[i], sqrt_information_v),
                             nullptr,
                             para_v[i]);
  }

  for(int i = 1; i < v_size; i++) {
    auto iter = lidarFrameList->begin();
    auto iter_next = lidarFrameList->begin();
    std::advance(iter, i-1);
    std::advance(iter_next, i);

    Eigen::Vector3d pi = iter->P + iter->Q*exPlb;
    Sophus::SO3d SO3_Ri(iter->Q*exRlb);
    Eigen::Vector3d ri = SO3_Ri.log();
    Eigen::Vector3d pj = iter_next->P + iter_next->Q*exPlb;
    Sophus::SO3d SO3_Rj(iter_next->Q*exRlb);
    Eigen::Vector3d rj = SO3_Rj.log();

    problem.AddResidualBlock(Cost_Initialization_IMU::Create(iter_next->imuIntegrator,
                                                                   ri,
                                                                   rj,
                                                                   pj-pi,
                                                                   Eigen::LLT<Eigen::Matrix<double, 9, 9>>
                                                                    (iter_next->imuIntegrator.GetCovariance().block<9,9>(0,0).inverse())
                                                                    .matrixL().transpose()),
                             nullptr,
                             para_r,
                             para_v[i-1],
                             para_v[i],
                             para_ba,
                             para_bg);
  }

  ceres::Solver::Options options;
  options.minimizer_progress_to_stdout = false;
  options.linear_solver_type = ceres::DENSE_QR;
  options.num_threads = 6;
  ceres::Solver::Summary summary;
  ceres::Solve(options, &problem, &summary);

  Eigen::Vector3d r_wg(para_r[0], para_r[1], para_r[2]);
  GravityVector = Sophus::SO3d::exp(r_wg) * Eigen::Vector3d(0, 0, -9.805);

  Eigen::Vector3d ba_vec(para_ba[0], para_ba[1], para_ba[2]);
  Eigen::Vector3d bg_vec(para_bg[0], para_bg[1], para_bg[2]);

  if(ba_vec.norm() > 0.5 || bg_vec.norm() > 0.5) {
    LIO_LOG_WARNING << "Too Large Biases! Initialization Failed!";
    return false;
  }

  for(int i = 0; i < v_size; i++) {
    auto iter = lidarFrameList->begin();
    std::advance(iter, i);
    iter->ba = ba_vec;
    iter->bg = bg_vec;
    Eigen::Vector3d bv_vec(para_v[i][0], para_v[i][1], para_v[i][2]);
    if((bv_vec - prior_v[i]).norm() > 2.0) {
      LIO_LOG_WARNING << "Too Large Velocity! Initialization Failed!";
      LIO_LOG_INFO << "delta v norm: " << (bv_vec - prior_v[i]).norm();
      return false;
    }
    iter->V = bv_vec;
  }

  for(size_t i = 0; i < v_size - 1; i++){
    auto laser_trans_i = lidarFrameList->begin();
    auto laser_trans_j = lidarFrameList->begin();
    std::advance(laser_trans_i, i);
    std::advance(laser_trans_j, i+1);
    laser_trans_j->imuIntegrator.PreIntegration(laser_trans_i->timeStamp, laser_trans_i->bg, laser_trans_i->ba);
  }


  // //if IMU success initialized
  WINDOWSIZE = Estimator::SLIDEWINDOWSIZE;
  while(lidarFrameList->size() > WINDOWSIZE){
	  lidarFrameList->pop_front();
  }
	Eigen::Vector3d Pwl = lidarFrameList->back().P;
	Eigen::Quaterniond Qwl = lidarFrameList->back().Q;
	lidarFrameList->back().P = Pwl + Qwl*exPlb;
	lidarFrameList->back().Q = Qwl * exRlb;

	// LIO_LOG_INFO << "\n=============================\n| Initialization Successful |\n=============================";
  
  return true;
}


/** \brief Mapping main thread
  */
void process(){
  double time_last_lidar = -1;
  double time_curr_lidar = -1;
  Eigen::Matrix3d delta_Rl = Eigen::Matrix3d::Identity();
  Eigen::Vector3d delta_tl = Eigen::Vector3d::Zero();
	Eigen::Matrix3d delta_Rb = Eigen::Matrix3d::Identity();
	Eigen::Vector3d delta_tb = Eigen::Vector3d::Zero();
  std::vector<sensor_msgs::msg::Imu::SharedPtr> vimuMsg;
  while(rclcpp::ok()){
    newfullCloud = false;
    laserCloudFullRes.reset(new pcl::PointCloud<PointType>());
	  std::unique_lock<std::mutex> lock_lidar(_mutexLidarQueue);
    if(!_lidarMsgQueue.empty()){
      // get new lidar msg
      time_curr_lidar = _lidarMsgQueue.front()->header.stamp.sec + _lidarMsgQueue.front()->header.stamp.nanosec * 1e-9;
      pcl::fromROSMsg(*_lidarMsgQueue.front(), *laserCloudFullRes);
      _lidarMsgQueue.pop();
      newfullCloud = true;
    }
    lock_lidar.unlock();

    if(newfullCloud){
      LIO_LOG_INFO << "[PoseEstimation] Processing new lidar frame, time: " << time_curr_lidar << ", LidarIMUInited: " << LidarIMUInited;

      nav_msgs::msg::Odometry debugInfo;
      debugInfo.pose.pose.position.x = 0;
      debugInfo.pose.pose.position.y = 0;
      debugInfo.pose.pose.position.z = 0;
      if(IMU_Mode > 0 && time_last_lidar > 0){
        // get IMU msg int the Specified time interval
        vimuMsg.clear();
        int countFail = 0;
        while (!fetchImuMsgs(time_last_lidar, time_curr_lidar, vimuMsg)) {
          countFail++;
          if (countFail > 100){
            LIO_LOG_WARNING << "[PoseEstimation] Failed to fetch IMU data after 100 retries, time range: [" 
                       << std::fixed << std::setprecision(6) << time_last_lidar << ", " << time_curr_lidar << "]";
            break;
          }
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        if(!vimuMsg.empty()) {
          LIO_LOG_DEBUG << "[PoseEstimation] Fetched " << vimuMsg.size() << " IMU messages";
        }
      }
      // this lidar frame init
      Estimator::LidarFrame lidarFrame;
      lidarFrame.laserCloud = laserCloudFullRes;
      lidarFrame.timeStamp = time_curr_lidar;

	    std::shared_ptr<std::list<Estimator::LidarFrame>> lidar_list;
	    if(!vimuMsg.empty()){
	    	if(!LidarIMUInited) {
	    		// if get IMU msg successfully, use gyro integration to update delta_Rl
			    lidarFrame.imuIntegrator.PushIMUMsg(vimuMsg);
			    lidarFrame.imuIntegrator.GyroIntegration(time_last_lidar);
			    delta_Rb = lidarFrame.imuIntegrator.GetDeltaQ().toRotationMatrix();
			    delta_Rl = exTlb.topLeftCorner(3, 3) * delta_Rb * exTlb.topLeftCorner(3, 3).transpose();

			    // predict current lidar pose
			    lidarFrame.P = transformAftMapped.topLeftCorner(3,3) * delta_tb
			                   + transformAftMapped.topRightCorner(3,1);
			    Eigen::Matrix3d m3d = transformAftMapped.topLeftCorner(3,3) * delta_Rb;
			    lidarFrame.Q = m3d;

			    lidar_list.reset(new std::list<Estimator::LidarFrame>);
			    lidar_list->push_back(lidarFrame);
		    }else{
			    // if get IMU msg successfully, use pre-integration to update delta lidar pose
			    lidarFrame.imuIntegrator.PushIMUMsg(vimuMsg);
			    lidarFrame.imuIntegrator.PreIntegration(lidarFrameList->back().timeStamp, lidarFrameList->back().bg, lidarFrameList->back().ba);

			    const Eigen::Vector3d& Pwbpre = lidarFrameList->back().P;
			    const Eigen::Quaterniond& Qwbpre = lidarFrameList->back().Q;
			    const Eigen::Vector3d& Vwbpre = lidarFrameList->back().V;

			    const Eigen::Quaterniond& dQ =  lidarFrame.imuIntegrator.GetDeltaQ();
			    const Eigen::Vector3d& dP = lidarFrame.imuIntegrator.GetDeltaP();
			    const Eigen::Vector3d& dV = lidarFrame.imuIntegrator.GetDeltaV();
			    double dt = lidarFrame.imuIntegrator.GetDeltaTime();

			    lidarFrame.Q = Qwbpre * dQ;
			    lidarFrame.P = Pwbpre + Vwbpre*dt + 0.5*GravityVector*dt*dt + Qwbpre*(dP);
			    lidarFrame.V = Vwbpre + GravityVector*dt + Qwbpre*(dV);
			    lidarFrame.bg = lidarFrameList->back().bg;
			    lidarFrame.ba = lidarFrameList->back().ba;

			    Eigen::Quaterniond Qwlpre = Qwbpre * Eigen::Quaterniond(exRbl);
			    Eigen::Vector3d Pwlpre = Qwbpre * exPbl + Pwbpre;

			    Eigen::Quaterniond Qwl = lidarFrame.Q * Eigen::Quaterniond(exRbl);
			    Eigen::Vector3d Pwl = lidarFrame.Q * exPbl + lidarFrame.P;

			    delta_Rl = Qwlpre.conjugate() * Qwl;
			    delta_tl = Qwlpre.conjugate() * (Pwl - Pwlpre);
			    delta_Rb = dQ.toRotationMatrix();
			    delta_tb = dP;

			    lidarFrameList->push_back(lidarFrame);
			    lidarFrameList->pop_front();
			    lidar_list = lidarFrameList;
	    	}
	    }else{
	    	if(LidarIMUInited){
	    	  // When initialized, we need IMU data to continue
		  LIO_LOG_WARNING << "[PoseEstimation] No IMU data but LidarIMUInited=true, skipping this frame";
	    	  time_last_lidar = time_curr_lidar;
	    	  continue;
	    	}else{
			    // predict current lidar pose
			    lidarFrame.P = transformAftMapped.topLeftCorner(3,3) * delta_tb
			                   + transformAftMapped.topRightCorner(3,1);
			    Eigen::Matrix3d m3d = transformAftMapped.topLeftCorner(3,3) * delta_Rb;
			    lidarFrame.Q = m3d;

			    lidar_list.reset(new std::list<Estimator::LidarFrame>);
			    lidar_list->push_back(lidarFrame);
	    	}
	    }

	    // remove lidar distortion
	    RemoveLidarDistortion(laserCloudFullRes, delta_Rl, delta_tl);

      // optimize current lidar pose with IMU
      LIO_LOG_INFO << "[PoseEstimation] Calling EstimateLidarPose, lidar_list size: " << lidar_list->size();
      estimator->EstimateLidarPose(*lidar_list, exTlb, GravityVector, debugInfo);
      LIO_LOG_INFO << "[PoseEstimation] EstimateLidarPose completed";

      pcl::PointCloud<PointType>::Ptr laserCloudCornerMap(new pcl::PointCloud<PointType>());
      pcl::PointCloud<PointType>::Ptr laserCloudSurfMap(new pcl::PointCloud<PointType>());

	    Eigen::Matrix4d transformTobeMapped = Eigen::Matrix4d::Identity();
	    transformTobeMapped.topLeftCorner(3,3) = lidar_list->front().Q * exRbl;
	    transformTobeMapped.topRightCorner(3,1) = lidar_list->front().Q * exPbl + lidar_list->front().P;

	    // update delta transformation
	    delta_Rb = transformAftMapped.topLeftCorner(3, 3).transpose() * lidar_list->front().Q.toRotationMatrix();
	    delta_tb = transformAftMapped.topLeftCorner(3, 3).transpose() * (lidar_list->front().P - transformAftMapped.topRightCorner(3, 1));
	    Eigen::Matrix3d Rwlpre = transformAftMapped.topLeftCorner(3, 3) * exRbl;
	    Eigen::Vector3d Pwlpre = transformAftMapped.topLeftCorner(3, 3) * exPbl + transformAftMapped.topRightCorner(3, 1);
	    delta_Rl = Rwlpre.transpose() * transformTobeMapped.topLeftCorner(3,3);
	    delta_tl = Rwlpre.transpose() * (transformTobeMapped.topRightCorner(3,1) - Pwlpre);
	    transformAftMapped.topLeftCorner(3,3) = lidar_list->front().Q.toRotationMatrix();
	    transformAftMapped.topRightCorner(3,1) = lidar_list->front().P;

	    // publish odometry rostopic
	    pubOdometry(transformTobeMapped, lidar_list->front().timeStamp);

      // publish lidar points
      int laserCloudFullResNum = lidar_list->front().laserCloud->points.size();
      LIO_LOG_INFO << "[PoseEstimation] Publishing point cloud with " << laserCloudFullResNum << " points";
      pcl::PointCloud<PointType>::Ptr laserCloudAfterEstimate(new pcl::PointCloud<PointType>());
      laserCloudAfterEstimate->reserve(laserCloudFullResNum);
      for (int i = 0; i < laserCloudFullResNum; i++) {
        PointType temp_point;
        MAP_MANAGER::pointAssociateToMap(&lidar_list->front().laserCloud->points[i], &temp_point, transformTobeMapped);
        laserCloudAfterEstimate->push_back(temp_point);
      }
      sensor_msgs::msg::PointCloud2 laserCloudMsg;
      pcl::toROSMsg(*laserCloudAfterEstimate, laserCloudMsg);
      laserCloudMsg.header.frame_id = "/world";
      laserCloudMsg.header.stamp = rclcpp::Time(static_cast<int64_t>(lidar_list->front().timeStamp * 1e9));
      pubFullLaserCloud->publish(laserCloudMsg);
      LIO_LOG_INFO << "[PoseEstimation] Point cloud published to /livox_full_cloud_mapped";

	    // if tightly coupled IMU message, start IMU initialization
	    if(IMU_Mode > 1 && !LidarIMUInited){
		    // update lidar frame pose
		    lidarFrame.P = transformTobeMapped.topRightCorner(3,1);
		    Eigen::Matrix3d m3d = transformTobeMapped.topLeftCorner(3,3);
		    lidarFrame.Q = m3d;

		    // static int pushCount = 0;
		    if(pushCount == 0){
			    lidarFrameList->push_back(lidarFrame);
			    lidarFrameList->back().imuIntegrator.Reset();
			    if(lidarFrameList->size() > WINDOWSIZE)
				    lidarFrameList->pop_front();
		    }else{
			    lidarFrameList->back().laserCloud = lidarFrame.laserCloud;
			    lidarFrameList->back().imuIntegrator.PushIMUMsg(vimuMsg);
			    lidarFrameList->back().timeStamp = lidarFrame.timeStamp;
			    lidarFrameList->back().P = lidarFrame.P;
			    lidarFrameList->back().Q = lidarFrame.Q;
		    }
		    pushCount++;
		    if (pushCount >= 3){
			    pushCount = 0;
			    if(lidarFrameList->size() > 1){
				    auto iterRight = std::prev(lidarFrameList->end());
				    auto iterLeft = std::prev(std::prev(lidarFrameList->end()));
				    iterRight->imuIntegrator.PreIntegration(iterLeft->timeStamp, iterLeft->bg, iterLeft->ba);
			    }

          if (lidarFrameList->size() == int(WINDOWSIZE / 1.5)) {
					  startTime = lidarFrameList->back().timeStamp;
					  LIO_LOG_INFO << "[PoseEstimation] Set startTime to: " << startTime << ", WINDOWSIZE: " << WINDOWSIZE;
			    }

			    if (!LidarIMUInited && lidarFrameList->size() == WINDOWSIZE && lidarFrameList->front().timeStamp >= startTime){
            LIO_LOG_INFO << "**************Start MAP Initialization!!!******************";
            LIO_LOG_INFO << "[PoseEstimation] Starting MAP Initialization, lidarFrameList size: " << lidarFrameList->size();
				if(TryMAPInitialization()){
              LidarIMUInited = true;
              LIO_LOG_INFO << "[PoseEstimation] MAP Initialization SUCCESS!";
					pushCount = 0;
              startTime = 0;
				} else {
              LIO_LOG_WARNING << "[PoseEstimation] MAP Initialization FAILED!";
            }
            LIO_LOG_INFO << "**************Finish MAP Initialization!!!******************";
			    }

		    }
	          }
      time_last_lidar = time_curr_lidar;
      LIO_LOG_INFO << "[PoseEstimation] Finished processing frame";

    } else {
      // No new cloud, sleep briefly to avoid busy waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

};

int main(int argc, char** argv)
{
    // Initialize logging system
    lio_livox::Logger::Initialize("LioLivox", "/home/charles/project/LIO-Livox/logs", "INFO");
    
    rclcpp::init(argc, argv);
    
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    auto node = std::make_shared<PoseEstimationNode>();
    g_node = node.get();
    
    LIO_LOG_INFO << "PoseEstimation node started. Press Ctrl+C to save map and exit.";
    
    rclcpp::spin(node);
    
    // Save map on normal exit
    if (g_estimator != nullptr) {
        LIO_LOG_INFO << "Saving map on normal exit...";
        std::string output_dir = "/home/charles/project/LIO-Livox/mapping_results";
        g_estimator->saveMapToPCD(output_dir);
    }
    
    // Shutdown logging system
    lio_livox::Logger::Shutdown();
    
    rclcpp::shutdown();
    return 0;
}

