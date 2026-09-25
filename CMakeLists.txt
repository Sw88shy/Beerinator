cmake_minimum_required(VERSION 3.21) # required for $<TARGET_RUNTIME_DLLS:...>

project(Beerinator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv)

add_executable(Beerinator src/main.cpp)

# OpenCV's own module targets only expose their include dirs as PRIVATE,
# so consumers must add each module's include path manually.
foreach(ocv_module ${OPENCV_MODULES_BUILD})
    target_include_directories(Beerinator PRIVATE "${OPENCV_MODULE_${ocv_module}_LOCATION}/include")
endforeach()
target_include_directories(Beerinator PRIVATE "${CMAKE_BINARY_DIR}")

target_link_libraries(Beerinator PRIVATE
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
)

set(BEERINATOR_RELEASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/releases")

add_custom_command(TARGET Beerinator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BEERINATOR_RELEASE_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Beerinator>" "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Beerinator> "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

# videoio's ffmpeg backend is a plugin loaded at runtime, not a link dependency
if(TARGET opencv_videoio_ffmpeg)
    add_custom_command(TARGET Beerinator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:opencv_videoio_ffmpeg>" "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
        VERBATIM
    )
endif()