// OpenCV
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "uv.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "runtime/runtime.h"
#include "runtime/thread.h"



int main(int, char**)
{
    std::vector<int> cameras = findAvailableCameras(9);
    std::cout << "Detected camera count: " << cameras.size() << std::endl;
    if(cameras.empty())
    {
        std::cerr << "No camera devices available." << std::endl;
        return 1;
    }

    AppState state;
    state.cameras.reserve(cameras.size());
    for(size_t i = 0; i < cameras.size(); ++i)
    {
        auto cam = std::make_unique<CameraState>();
        cam->cameraIndex = cameras[i];
        cam->windowName = "camera-" + std::to_string(cameras[i]);
        uv_mutex_init(&cam->frameMutex);
        state.cameras.push_back(std::move(cam));
    }

    uv_loop_t loop;
    state.loop = &loop;
    uv_loop_init(&loop);
    uv_async_init(&loop, &state.stopAsync, onStopAsync);

    std::vector<uv_thread_t> captureThreads(state.cameras.size());
    std::vector<CaptureThreadArgs> captureArgs(state.cameras.size());
    uv_thread_t displayThread;
    for(size_t i = 0; i < state.cameras.size(); ++i)
    {
        captureArgs[i].app = &state;
        captureArgs[i].camera = state.cameras[i].get();
        uv_thread_create(&captureThreads[i], captureThreadMain, &captureArgs[i]);
    }
    uv_thread_create(&displayThread, displayThreadMain, &state);

    uv_run(&loop, UV_RUN_DEFAULT);

    state.running = false;
    for(size_t i = 0; i < captureThreads.size(); ++i)
    {
        uv_thread_join(&captureThreads[i]);
    }
    uv_thread_join(&displayThread);

    uv_close(reinterpret_cast<uv_handle_t*>(&state.stopAsync), nullptr);
    uv_run(&loop, UV_RUN_DEFAULT);
    for(size_t i = 0; i < state.cameras.size(); ++i)
    {
        uv_mutex_destroy(&state.cameras[i]->frameMutex);
    }
    uv_loop_close(&loop);

    return 0;
}