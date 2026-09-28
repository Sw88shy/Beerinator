#include "runtime/thread.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

static void captureThreadMain(void* arg)
{
    CaptureThreadArgs* args = static_cast<CaptureThreadArgs*>(arg);
    AppState* app = args->app;
    CameraState* camera = args->camera;

    cv::VideoCapture cap(camera->cameraIndex, cv::CAP_DSHOW);
    if(!cap.isOpened())
    {
        std::cerr << "Failed to open camera device " << camera->cameraIndex << "." << std::endl;
        camera->failed = true;
        return;
    }
    camera->opened = true;

    cv::Mat frame;
    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();
    double fps = 0.0;

    while(app->running)
    {
        if(!cap.read(frame) || frame.empty())
        {
            uv_sleep(1);
            continue;
        }

        auto now = clock::now();
        double dt = std::chrono::duration<double>(now - lastTick).count();
        lastTick = now;
        if(dt > 0.0)
        {
            double instantFps = 1.0 / dt;
            fps = (fps <= 0.0) ? instantFps : (0.90 * fps + 0.10 * instantFps);
        }

        uv_mutex_lock(&camera->frameMutex);
        camera->latestFrame = frame.clone();
        camera->frameSeq++;
        camera->captureFps = fps;
        uv_mutex_unlock(&camera->frameMutex);
    }

    cap.release();
}

static void displayThreadMain(void* arg)
{
    AppState* app = static_cast<AppState*>(arg);
    std::vector<uint64_t> seenSeq(app->cameras.size(), 0);

    for(size_t i = 0; i < app->cameras.size(); ++i)
    {
        cv::namedWindow(app->cameras[i]->windowName, cv::WINDOW_AUTOSIZE);
    }

    while(app->running)
    {
        bool anyOpen = false;
        bool anyPending = false;
        for(size_t i = 0; i < app->cameras.size(); ++i)
        {
            CameraState& cam = *app->cameras[i];
            if(cam.opened)
            {
                anyOpen = true;
            }
            if(!cam.opened && !cam.failed)
            {
                anyPending = true;
            }

            cv::Mat displayFrame;
            double fps = 0.0;
            bool hasNewFrame = false;

            uv_mutex_lock(&cam.frameMutex);
            if(cam.frameSeq != seenSeq[i] && !cam.latestFrame.empty())
            {
                displayFrame = cam.latestFrame.clone();
                seenSeq[i] = cam.frameSeq;
                fps = cam.captureFps;
                hasNewFrame = true;
            }
            uv_mutex_unlock(&cam.frameMutex);

            if(hasNewFrame)
            {
                std::ostringstream oss;
                oss << "Camera " << cam.cameraIndex << "  FPS: " << std::fixed << std::setprecision(1) << fps;
                cv::putText(displayFrame, oss.str(), cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(30, 255, 30), 2, cv::LINE_AA);
                cv::imshow(cam.windowName, displayFrame);
            }
        }

        if(cv::waitKey(1) >= 0)
        {
            app->running = false;
            uv_async_send(&app->stopAsync);
            break;
        }

        if(!anyOpen && !anyPending)
        {
            std::cerr << "No cameras could be opened." << std::endl;
            app->running = false;
            uv_async_send(&app->stopAsync);
            break;
        }

        uv_sleep(1);
    }

    cv::destroyAllWindows();
}