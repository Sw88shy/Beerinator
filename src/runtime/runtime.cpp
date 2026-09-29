#include "runtime.h"

#define CAPTURE_METHOD cv::CAP_MSMF //CAP_DSHOW

std::vector<int> findAvailableCameras(int maxIndex)
{
    std::vector<int> indices;
    for(int i = 0; i <= maxIndex; ++i)
    {
        cv::VideoCapture probe(i, CAPTURE_METHOD);
        if(probe.isOpened())
        {
            indices.push_back(i);
            probe.release();
        }
    }
    return indices;
}

void onStopAsync(uv_async_t* handle)
{
    uv_stop(handle->loop);
}