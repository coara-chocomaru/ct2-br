#define LOG_TAG "CameraParamsShim"
#define _GNU_SOURCE 1

#include <utils/Log.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>

namespace android {

class CameraParameters {
public:
    static const char PIXEL_FORMAT_YUV420P_I420[];
    __attribute__((visibility("default")))
    const char *get(const char *key) const;
};

const char CameraParameters::PIXEL_FORMAT_YUV420P_I420[] = "yuv420p";

}

typedef const char *(*camera_parameters_get_t)(const void *thiz, const char *key);

static camera_parameters_get_t g_orig_camera_parameters_get = nullptr;
static pthread_once_t g_resolve_once = PTHREAD_ONCE_INIT;

static void camera_params_shim_resolve_original(void)
{
    g_orig_camera_parameters_get = reinterpret_cast<camera_parameters_get_t>(
        dlsym(RTLD_NEXT, "_ZNK7android15CameraParameters3getEPKc"));

    if (g_orig_camera_parameters_get == nullptr) {
        ALOGE("CameraParametersShim: failed to resolve original CameraParameters::get symbol: %s", dlerror());
    } else {
        ALOGI("CameraParametersShim: resolved original CameraParameters::get symbol");
    }
}

namespace android {

const char *CameraParameters::get(const char *key) const
{
    pthread_once(&g_resolve_once, camera_params_shim_resolve_original);

    const char *original_value = nullptr;

    if (g_orig_camera_parameters_get != nullptr) {
        original_value = g_orig_camera_parameters_get(this, key);
    }

    if (original_value != nullptr && strcmp(original_value, "yuv422i-uyvy") == 0) {
        ALOGI("CameraParametersShim: remapping unsupported color format \"yuv422i-uyvy\" to \"yuv422i-yuyv\" for key \"%s\"", key != nullptr ? key : "(null)");
        return "yuv422i-yuyv";
    }

    return original_value;
}

}

extern "C" __attribute__((constructor)) void camera_params_shim_ctor(void)
{
    ALOGI("libcamera_params_shim loaded: android::CameraParameters::PIXEL_FORMAT_YUV420P_I420 resolved");
}
