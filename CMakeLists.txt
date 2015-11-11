cmake_minimum_required(VERSION 2.6)
project(voxel)

add_executable(voxel main.cpp)

install(TARGETS voxel RUNTIME DESTINATION bin)
