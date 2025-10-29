#include "IMUIntegrator/IMUIntegrator.h"
#include "utils/logger.h"
#include <rclcpp/rclcpp.hpp>
#include <cassert>

IMUIntegrator::IMUIntegrator() {
  Reset();
  noise_.setZero();
  noise_.block<3, 3>(0, 0) = Eigen::Matrix3d::Identity() * gyr_n * gyr_n;
  noise_.block<3, 3>(3, 3) = Eigen::Matrix3d::Identity() * acc_n * acc_n;
  noise_.block<3, 3>(6, 6) = Eigen::Matrix3d::Identity() * gyr_w * gyr_w;
  noise_.block<3, 3>(9, 9) = Eigen::Matrix3d::Identity() * acc_w * acc_w;
}

/** \brief constructor of IMUIntegrator
 * \param[in] vIMU: IMU messages need to be integrated
 */
IMUIntegrator::IMUIntegrator(
    std::vector<sensor_msgs::msg::Imu::SharedPtr> vIMU)
    : vimu_msg_(std::move(vIMU)) {
  Reset();
  noise_.setZero();
  noise_.block<3, 3>(0, 0) = Eigen::Matrix3d::Identity() * gyr_n * gyr_n;
  noise_.block<3, 3>(3, 3) = Eigen::Matrix3d::Identity() * acc_n * acc_n;
  noise_.block<3, 3>(6, 6) = Eigen::Matrix3d::Identity() * gyr_w * gyr_w;
  noise_.block<3, 3>(9, 9) = Eigen::Matrix3d::Identity() * acc_w * acc_w;
}

void IMUIntegrator::Reset() {
  dq_.setIdentity();
  dp_.setZero();
  dv_.setZero();
  dtime_ = 0;
  covariance_.setZero();
  jacobian_.setIdentity();
  linearized_bg_.setZero();
  linearized_ba_.setZero();
}

const Eigen::Quaterniond& IMUIntegrator::GetDeltaQ() const { return dq_; }

const Eigen::Vector3d& IMUIntegrator::GetDeltaP() const { return dp_; }

const Eigen::Vector3d& IMUIntegrator::GetDeltaV() const { return dv_; }

const double& IMUIntegrator::GetDeltaTime() const { return dtime_; }

const Eigen::Vector3d& IMUIntegrator::GetBiasGyr() const {
  return linearized_bg_;
}

const Eigen::Vector3d& IMUIntegrator::GetBiasAcc() const {
  return linearized_ba_;
}

const Eigen::Matrix<double, 15, 15>& IMUIntegrator::GetCovariance() {
  return covariance_;
}

const Eigen::Matrix<double, 15, 15>& IMUIntegrator::GetJacobian() const {
  return jacobian_;
}

void IMUIntegrator::PushIMUMsg(const sensor_msgs::msg::Imu::SharedPtr& imu) {
  vimu_msg_.push_back(imu);
}
void IMUIntegrator::PushIMUMsg(
    const std::vector<sensor_msgs::msg::Imu::SharedPtr>& vimu) {
  vimu_msg_.insert(vimu_msg_.end(), vimu.begin(), vimu.end());
}
const std::vector<sensor_msgs::msg::Imu::SharedPtr>& IMUIntegrator::GetIMUMsg()
    const {
  return vimu_msg_;
}

void IMUIntegrator::GyroIntegration(double lastTime) {
  double current_time = lastTime;
  for (auto& imu : vimu_msg_) {
    Eigen::Vector3d gyr;
    gyr << imu->angular_velocity.x,
            imu->angular_velocity.y,
            imu->angular_velocity.z;
    double dt = (imu->header.stamp.sec + imu->header.stamp.nanosec * 1e-9) - current_time;
    assert(dt >= 0);
    Eigen::Matrix3d dR = Sophus::SO3d::exp(gyr * dt).matrix();
    Eigen::Quaterniond qr(dq_ * dR);
    if (qr.w() < 0) qr.coeffs() *= -1;
    dq_ = qr.normalized();
    current_time = imu->header.stamp.sec + imu->header.stamp.nanosec * 1e-9;
  }
}

void IMUIntegrator::PreIntegration(double lastTime,
                                   const Eigen::Vector3d& bg,
                                   const Eigen::Vector3d& ba) {
  Reset();
  linearized_bg_ = bg;
  linearized_ba_ = ba;
  double current_time = lastTime;
  for (auto& imu : vimu_msg_) {
    Eigen::Vector3d gyr;
    gyr <<  imu->angular_velocity.x,
            imu->angular_velocity.y,
            imu->angular_velocity.z;
    Eigen::Vector3d acc;
    acc << imu->linear_acceleration.x * kGNorm,
           imu->linear_acceleration.y * kGNorm,
           imu->linear_acceleration.z * kGNorm;
    double dt = (imu->header.stamp.sec + imu->header.stamp.nanosec * 1e-9) - current_time;
    if(dt <= 0 )
      LIO_LOG_WARNING << "dt <= 0";
    gyr -= bg;
    acc -= ba;
    double dt2 = dt * dt;
    Eigen::Vector3d gyr_dt = gyr * dt;
    Eigen::Matrix3d dR = Sophus::SO3d::exp(gyr_dt).matrix();
    Eigen::Matrix3d Jr = Eigen::Matrix3d::Identity();
    double gyr_dt_norm = gyr_dt.norm();
    if (gyr_dt_norm > 0.00001) {
      Eigen::Vector3d k = gyr_dt.normalized();
      Eigen::Matrix3d K = Sophus::SO3d::hat(k);
      Jr = Eigen::Matrix3d::Identity() -
           (1 - cos(gyr_dt_norm)) / gyr_dt_norm * K +
           (1 - sin(gyr_dt_norm) / gyr_dt_norm) * K * K;
    }
    Eigen::Matrix<double, 15, 15> A =
        Eigen::Matrix<double, 15, 15>::Identity();
    A.block<3, 3>(0, 3) = -0.5 * dq_.matrix() * Sophus::SO3d::hat(acc) * dt2;
    A.block<3, 3>(0, 6) = Eigen::Matrix3d::Identity() * dt;
    A.block<3, 3>(0, 12) = -0.5 * dq_.matrix() * dt2;
    A.block<3, 3>(3, 3) = dR.transpose();
    A.block<3, 3>(3, 9) = -Jr * dt;
    A.block<3, 3>(6, 3) = -dq_.matrix() * Sophus::SO3d::hat(acc) * dt;
    A.block<3, 3>(6, 12) = -dq_.matrix() * dt;
    Eigen::Matrix<double, 15, 12> B = Eigen::Matrix<double, 15, 12>::Zero();
    B.block<3, 3>(0, 3) = 0.5 * dq_.matrix() * dt2;
    B.block<3, 3>(3, 0) = Jr * dt;
    B.block<3, 3>(6, 3) = dq_.matrix() * dt;
    B.block<3, 3>(9, 6) = Eigen::Matrix3d::Identity() * dt;
    B.block<3, 3>(12, 9) = Eigen::Matrix3d::Identity() * dt;
    jacobian_ = A * jacobian_;
    covariance_ = A * covariance_ * A.transpose() + B * noise_ * B.transpose();
    dp_ += dv_ * dt + 0.5 * dq_.matrix() * acc * dt2;
    dv_ += dq_.matrix() * acc * dt;
    Eigen::Matrix3d m3dR = dq_.matrix() * dR;
    Eigen::Quaterniond qtmp(m3dR);
    if (qtmp.w() < 0) qtmp.coeffs() *= -1;
    dq_ = qtmp.normalized();
    dtime_ += dt;
    current_time = imu->header.stamp.sec + imu->header.stamp.nanosec * 1e-9;
  }
}

Eigen::Vector3d IMUIntegrator::GetAverageAcc() {
  int i = 0;
  Eigen::Vector3d sum_acc(0, 0, 0);
  for (auto& imu : vimu_msg_) {
    Eigen::Vector3d acc;
    acc << imu->linear_acceleration.x * kGNorm,
           imu->linear_acceleration.y * kGNorm,
           imu->linear_acceleration.z * kGNorm;
    sum_acc += acc;
    i++;
    if (i > 30) break;
  }
  return sum_acc / i;
}

