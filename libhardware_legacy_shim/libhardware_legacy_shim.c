#include <dlfcn.h>
#include <android/log.h>
#include <stddef.h>

#define LOG_TAG "HwLegacyShim"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

static void* g_real_handle = NULL;
static int (*g_real_wifi_set_mode)(int) = NULL;

__attribute__((constructor)) void shim_init(void) {
    const char* real_lib = "libhardware_legacy.so";
    g_real_handle = dlopen(real_lib, RTLD_NOW | RTLD_GLOBAL);
    if (!g_real_handle) {
        LOGE("dlopen(%s) failed: %s", real_lib, dlerror());
        return;
    }
    g_real_wifi_set_mode = (int (*)(int))dlsym(g_real_handle, "wifi_set_mode");
    if (g_real_wifi_set_mode) {
        LOGI("Found real wifi_set_mode");
    } else {
        LOGI("wifi_set_mode not found, using stub");
    }
}

__attribute__((visibility("default"))) int wifi_set_mode(int mode) {
    if (g_real_wifi_set_mode) {
        return g_real_wifi_set_mode(mode);
    }
    return 0;
}
