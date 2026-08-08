LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libcamera_ion_shim
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := camera_ion_shim.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_CFLAGS := -Wall -Wextra -fvisibility=default
LOCAL_PRELINK_MODULE := false
LOCAL_MULTILIB := both
include $(BUILD_SHARED_LIBRARY)
