#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # Get the package directory
    pkg_share = get_package_share_directory('lio_livox')
    
    # Declare launch arguments
    config_file_arg = DeclareLaunchArgument(
        'config_file',
        default_value=os.path.join(pkg_share, 'config', 'horizon_config.yaml'),
        description='Path to the config file'
    )
    
    msg_type_arg = DeclareLaunchArgument(
        'msg_type',
        default_value='0',
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
    
    # ScanRegistration node
    scan_registration_node = Node(
        package='lio_livox',
        executable='ScanRegistration',
        name='scan_registration',
        output='screen',
        parameters=[{
            'config_file': LaunchConfiguration('config_file'),
            'msg_type': LaunchConfiguration('msg_type')
        }]
    )
    
    # PoseEstimation node
    pose_estimation_node = Node(
        package='lio_livox',
        executable='PoseEstimation',
        name='pose_estimation',
        output='screen',
        parameters=[{
            'imu_mode': LaunchConfiguration('imu_mode'),
            'filter_parameter_corner': LaunchConfiguration('filter_parameter_corner'),
            'filter_parameter_surf': LaunchConfiguration('filter_parameter_surf'),
            'extrinsic_tlb': LaunchConfiguration('extrinsic_tlb')
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
        scan_registration_node,
        pose_estimation_node,
        rviz_node
    ])
