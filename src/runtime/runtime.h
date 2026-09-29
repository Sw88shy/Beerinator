#pragma once

#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"

#include "uv.h"

#include <atomic>
#include <vector>
#include <memory>
#include <string>

struct CameraState
{
    int cameraIndex = -1;
    std::string windowName;

    uv_mutex_t frameMutex{};
    cv::Mat latestFrame;
    uint64_t frameSeq = 0;
    double captureFps = 0.0;

    std::atomic<bool> opened{false};
    std::atomic<bool> failed{false};
};

struct AppState
{
    std::atomic<bool> running{true};

    uv_loop_t* loop = nullptr;
    uv_async_t stopAsync{};

    std::vector<std::unique_ptr<CameraState>> cameras;
};

struct CaptureThreadArgs
{
    AppState* app = nullptr;
    CameraState* camera = nullptr;
};

std::vector<int> findAvailableCameras(int maxIndex);
void onStopAsync(uv_async_t* handle);