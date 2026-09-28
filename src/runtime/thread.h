#pragma once

#include "runtime.h"
#include <opencv2/opencv.hpp>

static void captureThreadMain(void* arg);
static void displayThreadMain(void* arg);