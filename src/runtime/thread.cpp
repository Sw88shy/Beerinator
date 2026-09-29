#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "thread.h"

#include <thread>

#define CAPTURE_METHOD cv::CAP_MSMF //CAP_DSHOW

void captureThreadMain(void* arg)
{
    CaptureThreadArgs* args = static_cast<CaptureThreadArgs*>(arg);
    AppState* app = args->app;
    CameraState* camera = args->camera;

    cv::VideoCapture cap(camera->cameraIndex, CAPTURE_METHOD);
    if(!cap.isOpened())
    {
        std::cerr << "Failed to open camera device " << camera->cameraIndex << "." << std::endl;
        camera->failed = true;
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FPS, 60);
    std::cerr << "Camera " << camera->cameraIndex << " capture mode: "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << cap.get(cv::CAP_PROP_FPS) << " FPS" << std::endl;

    camera->opened = true;

    cv::Mat frame;
    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();
    double reportedFps = cap.get(cv::CAP_PROP_FPS);
    double maxCaptureFps = reportedFps > 0.0 && reportedFps <= 120.0 ? reportedFps : 30.0;
    auto frameInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(1.0 / maxCaptureFps));
    auto nextFrameDeadline = lastTick;
    double fps = 0.0;
    bool hasPreviousFrame = false;

    while(app->running)
    {
        if(!cap.read(frame) || frame.empty())
        {
            uv_sleep(1);
            continue;
        }

        auto now = clock::now();
        double dt = hasPreviousFrame ? std::chrono::duration<double>(now - lastTick).count() : 0.0;
        lastTick = now;
        hasPreviousFrame = true;
        if(dt > 0.0)
        {
            double instantFps = 1.0 / dt;
            if(instantFps > maxCaptureFps)
            {
                instantFps = maxCaptureFps;
            }
            fps = (fps <= 0.0) ? instantFps : (0.90 * fps + 0.10 * instantFps);
        }

        uv_mutex_lock(&camera->frameMutex);
        camera->latestFrame = frame.clone();
        camera->frameSeq++;
        camera->captureFps = fps;
        uv_mutex_unlock(&camera->frameMutex);

        nextFrameDeadline += frameInterval;
        if(nextFrameDeadline < now)
        {
            nextFrameDeadline = now;
        }
        std::this_thread::sleep_until(nextFrameDeadline);
    }

    cap.release();
}

void displayThreadMain(void* arg)
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