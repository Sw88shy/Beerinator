#include "runtime.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

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

static std::vector<int> findAvailableCameras(int maxIndex)
{
    std::vector<int> indices;
    for(int i = 0; i <= maxIndex; ++i)
    {
        cv::VideoCapture probe(i, cv::CAP_DSHOW);
        if(probe.isOpened())
        {
            indices.push_back(i);
            probe.release();
        }
    }
    return indices;
}

static void onStopAsync(uv_async_t* handle)
{
    uv_stop(handle->loop);
}