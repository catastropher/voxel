cmake_minimum_required(VERSION 2.6)
project(voxel)

add_executable(voxel main.cpp)

SET(CMAKE_CXX_FLAGS "-Wall")

find_package(SDL REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})