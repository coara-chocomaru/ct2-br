LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libcamera_parameters_shim
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := camera_parameters_shim.cpp
LOCAL_CFLAGS := -Wall -Werror -fvisibility=hidden -Wno-error=attributes
LOCAL_CPPFLAGS := -std=gnu++11
LOCAL_LDFLAGS := -Wl,--exclude-libs,ALL
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_SHARED_LIBRARY)
