LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := settings_country
LOCAL_SRC_FILES := settings_country.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall -Werror
include $(BUILD_EXECUTABLE)
