#include <dlfcn.h>
#include <android/log.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "HwLegacyShim"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

static void* g_real_handle = NULL;

static int (*real_wifi_set_mode)(int) = NULL;
static int (*real_wifi_start_supplicant)(void) = NULL;
static int (*real_wifi_stop_supplicant)(void) = NULL;
static int (*real_wifi_connect_to_supplicant)(void) = NULL;
static int (*real_wifi_close_supplicant_connection)(void) = NULL;
static int (*real_wifi_wait_for_event)(char*, size_t) = NULL;
static int (*real_wifi_command)(const char*, char*, size_t*) = NULL;
static int (*real_wifi_send_command)(const char*, char*, size_t*) = NULL;
static int (*real_wifi_supplicant_connection_active)(void) = NULL;
static int (*real_is_wifi_driver_loaded)(void) = NULL;
static int (*real_wifi_load_driver)(void) = NULL;
static int (*real_wifi_unload_driver)(void) = NULL;
static int (*real_wifi_enable)(void) = NULL;
static int (*real_wifi_disable)(void) = NULL;
static int (*real_wifi_get_mac_addr)(unsigned char*) = NULL;
static int (*real_wifi_get_fwstate)(char*, size_t) = NULL;
static int (*real_wifi_get_fw_path)(char*, size_t) = NULL;
static int (*real_wifi_change_fw_path)(const char*) = NULL;
static int (*real_wifi_set_drvarg)(const char*, const char*) = NULL;
static int (*real_wifi_set_drv_arg)(const char*, const char*) = NULL;
static int (*real_wifi_set_svc_args)(const char*, const char*) = NULL;
static int (*real_wifi_set_scanning_enabled)(int) = NULL;
static int (*real_wifi_get_scanning_enabled)(void) = NULL;
static int (*real_wifi_set_scanning_always_enabled)(int) = NULL;
static int (*real_wifi_get_scanning_always_enabled)(void) = NULL;
static int (*real_wifi_set_scan_interval)(int) = NULL;
static int (*real_wifi_get_scan_interval)(void) = NULL;
static int (*real_wifi_set_fast_scan)(int) = NULL;
static int (*real_wifi_get_fast_scan)(void) = NULL;
static int (*real_wifi_set_band)(int) = NULL;
static int (*real_wifi_get_band)(void) = NULL;
static int (*real_wifi_set_country_code)(const char*) = NULL;
static int (*real_wifi_get_country_code)(char*, size_t) = NULL;
static int (*real_wifi_set_p2p_listen_interval)(int) = NULL;
static int (*real_wifi_set_p2p_device_name)(const char*) = NULL;
static int (*real_wifi_get_p2p_device_name)(char*, size_t) = NULL;
static int (*real_wifi_set_p2p_ssid)(const char*) = NULL;
static int (*real_wifi_get_p2p_ssid)(char*, size_t) = NULL;
static int (*real_wifi_set_p2p_wps_config)(int) = NULL;
static int (*real_wifi_get_p2p_wps_config)(void) = NULL;
static int (*real_wifi_set_p2p_go_intent)(int) = NULL;
static int (*real_wifi_get_p2p_go_intent)(void) = NULL;
static int (*real_wifi_set_p2p_listen_reg_class)(int) = NULL;
static int (*real_wifi_get_p2p_listen_reg_class)(void) = NULL;
static int (*real_wifi_set_p2p_oper_reg_class)(int) = NULL;
static int (*real_wifi_get_p2p_oper_reg_class)(void) = NULL;
static int (*real_wifi_set_p2p_oper_channel)(int) = NULL;
static int (*real_wifi_get_p2p_oper_channel)(void) = NULL;
static int (*real_wifi_set_p2p_go_negotiation_delay)(int) = NULL;
static int (*real_wifi_get_p2p_go_negotiation_delay)(void) = NULL;
static int (*real_wifi_set_ap_interface)(const char*) = NULL;
static int (*real_wifi_get_ap_interface)(char*, size_t) = NULL;
static int (*real_wifi_set_ap_mac)(const unsigned char*) = NULL;
static int (*real_wifi_get_ap_mac)(unsigned char*) = NULL;
static int (*real_wifi_set_nvram)(const char*, const char*) = NULL;
static int (*real_wifi_get_nvram)(const char*, char*, size_t) = NULL;
static int (*real_wifi_set_supplicant_log_level)(int) = NULL;
static int (*real_wifi_get_supplicant_log_level)(void) = NULL;
static int (*real_ensure_entropy_file_exists)(void) = NULL;
static int (*real_ensure_config_file_exists)(void) = NULL;
static int (*real_bt_set_drvarg)(const char*, const char*) = NULL;
static int (*real_bt_set_drv_arg)(const char*, const char*) = NULL;
static int (*real_setDbg)(int) = NULL;

#define GET_REAL_FUNC(ptr, name) \
    do { \
        ptr = (typeof(ptr))dlsym(g_real_handle, name); \
        if (!ptr) { \
            LOGE("dlsym(%s) failed: %s", name, dlerror()); \
        } else { \
            LOGI("Found real function: %s", name); \
        } \
    } while(0)

__attribute__((constructor)) void shim_init(void) {
    const char* real_lib = "libhardware_legacy.so";
    g_real_handle = dlopen(real_lib, RTLD_NOW | RTLD_GLOBAL);
    if (!g_real_handle) {
        LOGE("dlopen(%s) failed: %s", real_lib, dlerror());
        return;
    }
    LOGI("Loaded real library: %s", real_lib);
    GET_REAL_FUNC(real_wifi_set_mode, "wifi_set_mode");
    GET_REAL_FUNC(real_wifi_start_supplicant, "wifi_start_supplicant");
    GET_REAL_FUNC(real_wifi_stop_supplicant, "wifi_stop_supplicant");
    GET_REAL_FUNC(real_wifi_connect_to_supplicant, "wifi_connect_to_supplicant");
    GET_REAL_FUNC(real_wifi_close_supplicant_connection, "wifi_close_supplicant_connection");
    GET_REAL_FUNC(real_wifi_wait_for_event, "wifi_wait_for_event");
    GET_REAL_FUNC(real_wifi_command, "wifi_command");
    GET_REAL_FUNC(real_wifi_send_command, "wifi_send_command");
    GET_REAL_FUNC(real_wifi_supplicant_connection_active, "wifi_supplicant_connection_active");
    GET_REAL_FUNC(real_is_wifi_driver_loaded, "is_wifi_driver_loaded");
    GET_REAL_FUNC(real_wifi_load_driver, "wifi_load_driver");
    GET_REAL_FUNC(real_wifi_unload_driver, "wifi_unload_driver");
    GET_REAL_FUNC(real_wifi_enable, "wifi_enable");
    GET_REAL_FUNC(real_wifi_disable, "wifi_disable");
    GET_REAL_FUNC(real_wifi_get_mac_addr, "wifi_get_mac_addr");
    GET_REAL_FUNC(real_wifi_get_fwstate, "wifi_get_fwstate");
    GET_REAL_FUNC(real_wifi_get_fw_path, "wifi_get_fw_path");
    GET_REAL_FUNC(real_wifi_change_fw_path, "wifi_change_fw_path");
    GET_REAL_FUNC(real_wifi_set_drvarg, "wifi_set_drvarg");
    GET_REAL_FUNC(real_wifi_set_drv_arg, "wifi_set_drv_arg");
    GET_REAL_FUNC(real_wifi_set_svc_args, "wifi_set_svc_args");
    GET_REAL_FUNC(real_wifi_set_scanning_enabled, "wifi_set_scanning_enabled");
    GET_REAL_FUNC(real_wifi_get_scanning_enabled, "wifi_get_scanning_enabled");
    GET_REAL_FUNC(real_wifi_set_scanning_always_enabled, "wifi_set_scanning_always_enabled");
    GET_REAL_FUNC(real_wifi_get_scanning_always_enabled, "wifi_get_scanning_always_enabled");
    GET_REAL_FUNC(real_wifi_set_scan_interval, "wifi_set_scan_interval");
    GET_REAL_FUNC(real_wifi_get_scan_interval, "wifi_get_scan_interval");
    GET_REAL_FUNC(real_wifi_set_fast_scan, "wifi_set_fast_scan");
    GET_REAL_FUNC(real_wifi_get_fast_scan, "wifi_get_fast_scan");
    GET_REAL_FUNC(real_wifi_set_band, "wifi_set_band");
    GET_REAL_FUNC(real_wifi_get_band, "wifi_get_band");
    GET_REAL_FUNC(real_wifi_set_country_code, "wifi_set_country_code");
    GET_REAL_FUNC(real_wifi_get_country_code, "wifi_get_country_code");
    GET_REAL_FUNC(real_wifi_set_p2p_listen_interval, "wifi_set_p2p_listen_interval");
    GET_REAL_FUNC(real_wifi_set_p2p_device_name, "wifi_set_p2p_device_name");
    GET_REAL_FUNC(real_wifi_get_p2p_device_name, "wifi_get_p2p_device_name");
    GET_REAL_FUNC(real_wifi_set_p2p_ssid, "wifi_set_p2p_ssid");
    GET_REAL_FUNC(real_wifi_get_p2p_ssid, "wifi_get_p2p_ssid");
    GET_REAL_FUNC(real_wifi_set_p2p_wps_config, "wifi_set_p2p_wps_config");
    GET_REAL_FUNC(real_wifi_get_p2p_wps_config, "wifi_get_p2p_wps_config");
    GET_REAL_FUNC(real_wifi_set_p2p_go_intent, "wifi_set_p2p_go_intent");
    GET_REAL_FUNC(real_wifi_get_p2p_go_intent, "wifi_get_p2p_go_intent");
    GET_REAL_FUNC(real_wifi_set_p2p_listen_reg_class, "wifi_set_p2p_listen_reg_class");
    GET_REAL_FUNC(real_wifi_get_p2p_listen_reg_class, "wifi_get_p2p_listen_reg_class");
    GET_REAL_FUNC(real_wifi_set_p2p_oper_reg_class, "wifi_set_p2p_oper_reg_class");
    GET_REAL_FUNC(real_wifi_get_p2p_oper_reg_class, "wifi_get_p2p_oper_reg_class");
    GET_REAL_FUNC(real_wifi_set_p2p_oper_channel, "wifi_set_p2p_oper_channel");
    GET_REAL_FUNC(real_wifi_get_p2p_oper_channel, "wifi_get_p2p_oper_channel");
    GET_REAL_FUNC(real_wifi_set_p2p_go_negotiation_delay, "wifi_set_p2p_go_negotiation_delay");
    GET_REAL_FUNC(real_wifi_get_p2p_go_negotiation_delay, "wifi_get_p2p_go_negotiation_delay");
    GET_REAL_FUNC(real_wifi_set_ap_interface, "wifi_set_ap_interface");
    GET_REAL_FUNC(real_wifi_get_ap_interface, "wifi_get_ap_interface");
    GET_REAL_FUNC(real_wifi_set_ap_mac, "wifi_set_ap_mac");
    GET_REAL_FUNC(real_wifi_get_ap_mac, "wifi_get_ap_mac");
    GET_REAL_FUNC(real_wifi_set_nvram, "wifi_set_nvram");
    GET_REAL_FUNC(real_wifi_get_nvram, "wifi_get_nvram");
    GET_REAL_FUNC(real_wifi_set_supplicant_log_level, "wifi_set_supplicant_log_level");
    GET_REAL_FUNC(real_wifi_get_supplicant_log_level, "wifi_get_supplicant_log_level");
    GET_REAL_FUNC(real_ensure_entropy_file_exists, "ensure_entropy_file_exists");
    GET_REAL_FUNC(real_ensure_config_file_exists, "ensure_config_file_exists");
    GET_REAL_FUNC(real_bt_set_drvarg, "bt_set_drvarg");
    GET_REAL_FUNC(real_bt_set_drv_arg, "bt_set_drv_arg");
    GET_REAL_FUNC(real_setDbg, "setDbg");
}

__attribute__((visibility("default"))) int wifi_set_mode(int mode) {
    return real_wifi_set_mode ? real_wifi_set_mode(mode) : 0;
}
__attribute__((visibility("default"))) int wifi_start_supplicant(void) {
    return real_wifi_start_supplicant ? real_wifi_start_supplicant() : -1;
}
__attribute__((visibility("default"))) int wifi_stop_supplicant(void) {
    return real_wifi_stop_supplicant ? real_wifi_stop_supplicant() : -1;
}
__attribute__((visibility("default"))) int wifi_connect_to_supplicant(void) {
    return real_wifi_connect_to_supplicant ? real_wifi_connect_to_supplicant() : -1;
}
__attribute__((visibility("default"))) int wifi_close_supplicant_connection(void) {
    return real_wifi_close_supplicant_connection ? real_wifi_close_supplicant_connection() : -1;
}
__attribute__((visibility("default"))) int wifi_wait_for_event(char* buf, size_t len) {
    return real_wifi_wait_for_event ? real_wifi_wait_for_event(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_command(const char* cmd, char* reply, size_t* reply_len) {
    return real_wifi_command ? real_wifi_command(cmd, reply, reply_len) : -1;
}
__attribute__((visibility("default"))) int wifi_send_command(const char* cmd, char* reply, size_t* reply_len) {
    return real_wifi_send_command ? real_wifi_send_command(cmd, reply, reply_len) : -1;
}
__attribute__((visibility("default"))) int wifi_supplicant_connection_active(void) {
    return real_wifi_supplicant_connection_active ? real_wifi_supplicant_connection_active() : 0;
}

__attribute__((visibility("default"))) int is_wifi_driver_loaded(void) {
    return real_is_wifi_driver_loaded ? real_is_wifi_driver_loaded() : 0;
}
__attribute__((visibility("default"))) int wifi_load_driver(void) {
    return real_wifi_load_driver ? real_wifi_load_driver() : -1;
}
__attribute__((visibility("default"))) int wifi_unload_driver(void) {
    return real_wifi_unload_driver ? real_wifi_unload_driver() : -1;
}
__attribute__((visibility("default"))) int wifi_enable(void) {
    return real_wifi_enable ? real_wifi_enable() : -1;
}
__attribute__((visibility("default"))) int wifi_disable(void) {
    return real_wifi_disable ? real_wifi_disable() : -1;
}

__attribute__((visibility("default"))) int wifi_get_mac_addr(unsigned char* mac) {
    return real_wifi_get_mac_addr ? real_wifi_get_mac_addr(mac) : -1;
}
__attribute__((visibility("default"))) int wifi_get_fwstate(char* buf, size_t len) {
    return real_wifi_get_fwstate ? real_wifi_get_fwstate(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_get_fw_path(char* buf, size_t len) {
    return real_wifi_get_fw_path ? real_wifi_get_fw_path(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_change_fw_path(const char* path) {
    return real_wifi_change_fw_path ? real_wifi_change_fw_path(path) : -1;
}
__attribute__((visibility("default"))) int wifi_set_drvarg(const char* key, const char* value) {
    return real_wifi_set_drvarg ? real_wifi_set_drvarg(key, value) : -1;
}
__attribute__((visibility("default"))) int wifi_set_drv_arg(const char* key, const char* value) {
    return real_wifi_set_drv_arg ? real_wifi_set_drv_arg(key, value) : -1;
}
__attribute__((visibility("default"))) int wifi_set_svc_args(const char* key, const char* value) {
    return real_wifi_set_svc_args ? real_wifi_set_svc_args(key, value) : -1;
}

__attribute__((visibility("default"))) int wifi_set_scanning_enabled(int enabled) {
    return real_wifi_set_scanning_enabled ? real_wifi_set_scanning_enabled(enabled) : -1;
}
__attribute__((visibility("default"))) int wifi_get_scanning_enabled(void) {
    return real_wifi_get_scanning_enabled ? real_wifi_get_scanning_enabled() : -1;
}
__attribute__((visibility("default"))) int wifi_set_scanning_always_enabled(int enabled) {
    return real_wifi_set_scanning_always_enabled ? real_wifi_set_scanning_always_enabled(enabled) : -1;
}
__attribute__((visibility("default"))) int wifi_get_scanning_always_enabled(void) {
    return real_wifi_get_scanning_always_enabled ? real_wifi_get_scanning_always_enabled() : -1;
}
__attribute__((visibility("default"))) int wifi_set_scan_interval(int interval) {
    return real_wifi_set_scan_interval ? real_wifi_set_scan_interval(interval) : -1;
}
__attribute__((visibility("default"))) int wifi_get_scan_interval(void) {
    return real_wifi_get_scan_interval ? real_wifi_get_scan_interval() : -1;
}
__attribute__((visibility("default"))) int wifi_set_fast_scan(int enabled) {
    return real_wifi_set_fast_scan ? real_wifi_set_fast_scan(enabled) : -1;
}
__attribute__((visibility("default"))) int wifi_get_fast_scan(void) {
    return real_wifi_get_fast_scan ? real_wifi_get_fast_scan() : -1;
}
__attribute__((visibility("default"))) int wifi_set_band(int band) {
    return real_wifi_set_band ? real_wifi_set_band(band) : -1;
}
__attribute__((visibility("default"))) int wifi_get_band(void) {
    return real_wifi_get_band ? real_wifi_get_band() : -1;
}
__attribute__((visibility("default"))) int wifi_set_country_code(const char* code) {
    return real_wifi_set_country_code ? real_wifi_set_country_code(code) : -1;
}
__attribute__((visibility("default"))) int wifi_get_country_code(char* buf, size_t len) {
    return real_wifi_get_country_code ? real_wifi_get_country_code(buf, len) : -1;
}

__attribute__((visibility("default"))) int wifi_set_p2p_listen_interval(int interval) {
    return real_wifi_set_p2p_listen_interval ? real_wifi_set_p2p_listen_interval(interval) : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_device_name(const char* name) {
    return real_wifi_set_p2p_device_name ? real_wifi_set_p2p_device_name(name) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_device_name(char* buf, size_t len) {
    return real_wifi_get_p2p_device_name ? real_wifi_get_p2p_device_name(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_ssid(const char* ssid) {
    return real_wifi_set_p2p_ssid ? real_wifi_set_p2p_ssid(ssid) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_ssid(char* buf, size_t len) {
    return real_wifi_get_p2p_ssid ? real_wifi_get_p2p_ssid(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_wps_config(int config) {
    return real_wifi_set_p2p_wps_config ? real_wifi_set_p2p_wps_config(config) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_wps_config(void) {
    return real_wifi_get_p2p_wps_config ? real_wifi_get_p2p_wps_config() : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_go_intent(int intent) {
    return real_wifi_set_p2p_go_intent ? real_wifi_set_p2p_go_intent(intent) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_go_intent(void) {
    return real_wifi_get_p2p_go_intent ? real_wifi_get_p2p_go_intent() : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_listen_reg_class(int reg_class) {
    return real_wifi_set_p2p_listen_reg_class ? real_wifi_set_p2p_listen_reg_class(reg_class) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_listen_reg_class(void) {
    return real_wifi_get_p2p_listen_reg_class ? real_wifi_get_p2p_listen_reg_class() : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_oper_reg_class(int reg_class) {
    return real_wifi_set_p2p_oper_reg_class ? real_wifi_set_p2p_oper_reg_class(reg_class) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_oper_reg_class(void) {
    return real_wifi_get_p2p_oper_reg_class ? real_wifi_get_p2p_oper_reg_class() : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_oper_channel(int channel) {
    return real_wifi_set_p2p_oper_channel ? real_wifi_set_p2p_oper_channel(channel) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_oper_channel(void) {
    return real_wifi_get_p2p_oper_channel ? real_wifi_get_p2p_oper_channel() : -1;
}
__attribute__((visibility("default"))) int wifi_set_p2p_go_negotiation_delay(int delay) {
    return real_wifi_set_p2p_go_negotiation_delay ? real_wifi_set_p2p_go_negotiation_delay(delay) : -1;
}
__attribute__((visibility("default"))) int wifi_get_p2p_go_negotiation_delay(void) {
    return real_wifi_get_p2p_go_negotiation_delay ? real_wifi_get_p2p_go_negotiation_delay() : -1;
}

__attribute__((visibility("default"))) int wifi_set_ap_interface(const char* iface) {
    return real_wifi_set_ap_interface ? real_wifi_set_ap_interface(iface) : -1;
}
__attribute__((visibility("default"))) int wifi_get_ap_interface(char* buf, size_t len) {
    return real_wifi_get_ap_interface ? real_wifi_get_ap_interface(buf, len) : -1;
}
__attribute__((visibility("default"))) int wifi_set_ap_mac(const unsigned char* mac) {
    return real_wifi_set_ap_mac ? real_wifi_set_ap_mac(mac) : -1;
}
__attribute__((visibility("default"))) int wifi_get_ap_mac(unsigned char* mac) {
    return real_wifi_get_ap_mac ? real_wifi_get_ap_mac(mac) : -1;
}

__attribute__((visibility("default"))) int wifi_set_nvram(const char* key, const char* value) {
    return real_wifi_set_nvram ? real_wifi_set_nvram(key, value) : -1;
}
__attribute__((visibility("default"))) int wifi_get_nvram(const char* key, char* buf, size_t len) {
    return real_wifi_get_nvram ? real_wifi_get_nvram(key, buf, len) : -1;
}

__attribute__((visibility("default"))) int wifi_set_supplicant_log_level(int level) {
    return real_wifi_set_supplicant_log_level ? real_wifi_set_supplicant_log_level(level) : -1;
}
__attribute__((visibility("default"))) int wifi_get_supplicant_log_level(void) {
    return real_wifi_get_supplicant_log_level ? real_wifi_get_supplicant_log_level() : -1;
}

__attribute__((visibility("default"))) int ensure_entropy_file_exists(void) {
    return real_ensure_entropy_file_exists ? real_ensure_entropy_file_exists() : -1;
}
__attribute__((visibility("default"))) int ensure_config_file_exists(void) {
    return real_ensure_config_file_exists ? real_ensure_config_file_exists() : -1;
}

__attribute__((visibility("default"))) int bt_set_drvarg(const char* key, const char* value) {
    return real_bt_set_drvarg ? real_bt_set_drvarg(key, value) : -1;
}
__attribute__((visibility("default"))) int bt_set_drv_arg(const char* key, const char* value) {
    return real_bt_set_drv_arg ? real_bt_set_drv_arg(key, value) : -1;
}

__attribute__((visibility("default"))) int setDbg(int level) {
    return real_setDbg ? real_setDbg(level) : -1;
}
