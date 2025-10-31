#!/usr/bin/env python3

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    # Get the package directory
    pkg_share = get_package_share_directory('lio_livox')

    # Declare launch arguments
    config_file_arg = DeclareLaunchArgument(
        'config_file',
        default_value=os.path.join(pkg_share, 'config', 'mid360_config.yaml'),
        description='Path to the config file'
    )

    msg_type_arg = DeclareLaunchArgument(
        'msg_type',
        default_value='1',
        description='Message type: 0-custom msg, 1-ros sensor msg pointcloud2 msg'
    )

    imu_mode_arg = DeclareLaunchArgument(
        'imu_mode',
        default_value='2',
        description='IMU mode: 0-Not Use IMU, 1-Use IMU remove Rotation Distort, 2-Tightly Coupled IMU'
    )

    filter_parameter_corner_arg = DeclareLaunchArgument(
        'filter_parameter_corner',
        default_value='0.2',
        description='Voxel Filter Size Use to Downsize Map Cloud for corners'
    )

    filter_parameter_surf_arg = DeclareLaunchArgument(
        'filter_parameter_surf',
        default_value='0.4',
        description='Voxel Filter Size Use to Downsize Map Cloud for surfaces'
    )

    extrinsic_tlb_arg = DeclareLaunchArgument(
        'extrinsic_tlb',
        default_value='[0.9999161, 0.0026676, 0.0126707, -0.011, -0.0025826, 0.9999741, -0.0067201, -0.0234, -0.0126883, 0.0066868, 0.9998971, 0.044, 0.0, 0.0, 0.0, 1.0]',
        description='Extrinsic Parameter between Lidar & IMU'
    )

    # Unified LioLivoxUnified node (combines both ScanRegistration and PoseEstimation)
    # Note: Both ScanRegistrationNode and PoseEstimationNode will read parameters from the same namespace
    # ScanRegistrationNode needs: config_file, msg_type
    # PoseEstimationNode needs: IMU_Mode, filter_parameter_corner, filter_parameter_surf, Extrinsic_Tlb
    lio_livox_unified_node = Node(
        package='lio_livox',
        executable='LioLivoxUnified',
        name='lio_livox_unified',
        output='screen',
        parameters=[{
            # Parameters for ScanRegistrationNode
            'config_file': LaunchConfiguration('config_file'),
            'msg_type': LaunchConfiguration('msg_type'),
            # Parameters for PoseEstimationNode (note: IMU_Mode not imu_mode)
            'IMU_Mode': LaunchConfiguration('imu_mode'),  # Map imu_mode to IMU_Mode
            'filter_parameter_corner': LaunchConfiguration('filter_parameter_corner'),
            'filter_parameter_surf': LaunchConfiguration('filter_parameter_surf'),
            'Extrinsic_Tlb': [0.9999161, 0.0026676, 0.0126707, -0.011, -0.0025826, 0.9999741, -0.0067201, -0.0234, -0.0126883, 0.0066868, 0.9998971, 0.044, 0.0, 0.0, 0.0, 1.0]
        }]
    )

    # RViz node
    rviz_config_file = os.path.join(pkg_share, 'rviz_cfg', 'lio.rviz')
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        output='screen'
    )

    return LaunchDescription([
        config_file_arg,
        msg_type_arg,
        imu_mode_arg,
        filter_parameter_corner_arg,
        filter_parameter_surf_arg,
        extrinsic_tlb_arg,
        lio_livox_unified_node,
        rviz_node
    ])
