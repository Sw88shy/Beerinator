cmake_minimum_required(VERSION 3.21)

project(Beerinator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(WIN32)
    find_program(BEERINATOR_CLANG_TIDY_EXE
        NAMES clang-tidy
        HINTS
            "C:/Program Files/LLVM/bin"
            "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/bin")

    if(BEERINATOR_CLANG_TIDY_EXE)
        set(CLANG_TIDY_EXE "${BEERINATOR_CLANG_TIDY_EXE}" CACHE FILEPATH "clang-tidy executable" FORCE)
    else()
        set(ENABLE_CLANG_TIDY OFF CACHE BOOL "Disable clang-tidy checks when not available" FORCE)
    endif()
endif()

set(ENABLE_LIBJPEG_TURBO_SIMD OFF CACHE BOOL "Disable JPEG turbo SIMD in vendored OpenCV" FORCE)

if(MSVC)
    set(WITH_MSMF OFF CACHE BOOL "Disable Media Foundation backend for MSVC DSHOW path" FORCE)
    set(WITH_DSHOW ON CACHE BOOL "Enable DirectShow backend" FORCE)
elseif(MINGW)
    set(WITH_MSMF OFF CACHE BOOL "Disable Media Foundation backend for MinGW toolchain" FORCE)
    set(WITH_DSHOW ON CACHE BOOL "Enable DirectShow backend" FORCE)
endif()

set(VIDEOIO_ENABLE_PLUGINS OFF CACHE BOOL "Disable OpenCV videoio plugins" FORCE)
set(HIGHGUI_ENABLE_PLUGINS OFF CACHE BOOL "Disable OpenCV highgui plugins" FORCE)
set(PARALLEL_ENABLE_PLUGINS OFF CACHE BOOL "Disable OpenCV core parallel plugins" FORCE)

if(MINGW)
    add_compile_options(-Wa,-mbig-obj)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv)

add_executable(Beerinator 
    src/main.cpp
    src/runtime/runtime.cpp
    src/runtime/thread.cpp
)

foreach(ocv_module ${OPENCV_MODULES_BUILD})
    target_include_directories(Beerinator PRIVATE "${OPENCV_MODULE_${ocv_module}_LOCATION}/include")
endforeach()
target_include_directories(Beerinator PRIVATE "${CMAKE_BINARY_DIR}")

target_link_libraries(Beerinator PRIVATE
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_videoio
    uv
)

set(BEERINATOR_RELEASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/releases")

add_custom_command(TARGET Beerinator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BEERINATOR_RELEASE_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Beerinator>" "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Beerinator> "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

if(TARGET opencv_videoio_ffmpeg)
    add_custom_command(TARGET Beerinator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:opencv_videoio_ffmpeg>" "${BEERINATOR_RELEASE_OUTPUT_DIR}/"
        VERBATIM
    )
endif()