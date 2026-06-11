cmake_minimum_required(VERSION 3.16)
project(NestboxMonitoring VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(camera_node src/main.cpp src/CameraNode.cpp)

target_link_libraries(camera_node Qt6::Core Qt6::Network)
