#include <android/log.h>

#define LOG_TAG "HwLegacyShim"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

__attribute__((visibility("default")))
int wifi_set_mode(int mode)
{
    LOGI("wifi_set_mode(shim) mode=%d", mode);
    return 0;
}
