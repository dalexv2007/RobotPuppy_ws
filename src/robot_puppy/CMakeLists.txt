cmake_minimum_required(VERSION 3.8)
project(robot_puppy)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BallLocation.msg"
)

ament_export_dependencies(rosidl_default_runtime)

add_executable(robot_puppy_node src/RobotPuppy.cpp)
ament_target_dependencies(robot_puppy_node
  rclcpp
  geometry_msgs
  sensor_msgs
  cv_bridge
)

target_include_directories(robot_puppy_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(robot_puppy_node ${cpp_typesupport_target})

install(TARGETS robot_puppy_node
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS scripts/BallFinder.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME BallFinder
)

ament_package()
