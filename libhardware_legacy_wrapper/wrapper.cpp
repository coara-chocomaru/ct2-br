#include <dlfcn.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "HwLegacyShim"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void* g_real_handle = NULL;
static int (*g_real_wifi_set_mode)(int) = NULL;

__attribute__((constructor)) void shim_init() {
    const char* real_lib = "libhardware_legacy_real.so";
    g_real_handle = dlopen(real_lib, RTLD_NOW | RTLD_GLOBAL);
    if (!g_real_handle) {
        LOGE("Failed to dlopen %s: %s", real_lib, dlerror());
        return;
    }
    g_real_wifi_set_mode = (int (*)(int))dlsym(g_real_handle, "wifi_set_mode");
    if (g_real_wifi_set_mode) {
        LOGI("Found real wifi_set_mode in %s", real_lib);
    } else {
        LOGI("wifi_set_mode not found in %s, using stub", real_lib);
    }
}

extern "C" {

__attribute__((visibility("default"))) int wifi_set_mode(int mode) {
    if (g_real_wifi_set_mode) {
        return g_real_wifi_set_mode(mode);
    }
    return 0;
}

}
