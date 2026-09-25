cmake_minimum_required(VERSION 3.16)

project(Beerinator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv)

add_executable(Beerinator src/main.cpp)

target_include_directories(Beerinator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv/include
)

target_link_libraries(Beerinator PRIVATE
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
)
