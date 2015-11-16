cmake_minimum_required(VERSION 2.6)
project(voxel)

add_executable(voxel main.cpp)

SET(CMAKE_CXX_FLAGS "-Wall")

find_package(SDL REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
target_link_libraries(voxel SDLmain ${SDL_LIBRARY})


find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
target_link_libraries(voxel glut ${GLUT_LIBRARY_DIRS})
add_definitions(${GLUT_DEFINITIONS})
if(NOT GLUT_FOUND)
    message(ERROR " GLUT not found!")
endif(NOT GLUT_FOUND)


find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
target_link_libraries(voxel GL ${OpenGL_LIBRARY_DIRS})
target_link_libraries(voxel GLU ${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)