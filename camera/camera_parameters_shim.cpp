#define LOG_TAG "CameraParamsShim"

#include <utils/Log.h>

namespace android {

class CameraParameters {
public:
    static const char PIXEL_FORMAT_YUV420P_I420[];
};

const char CameraParameters::PIXEL_FORMAT_YUV420P_I420[] = "yuv420p";

}

extern "C" __attribute__((constructor)) void camera_params_shim_ctor(void)
{
    ALOGI("libcamera_params_shim loaded: android::CameraParameters::PIXEL_FORMAT_YUV420P_I420 resolved");
}
