LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libhardware_legacx
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := libhardware_legacy_shim.c
LOCAL_CFLAGS := -Wall -Werror -fvisibility=hidden -O2 -fPIC
LOCAL_SHARED_LIBRARIES := libdl liblog
LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
