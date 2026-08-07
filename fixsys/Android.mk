LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := perfd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := perfd.cpp
LOCAL_CFLAGS := -Wall -Wextra -O2
LOCAL_CPPFLAGS := -std=c++11
LOCAL_SHARED_LIBRARIES := liblog libc libstlport
LOCAL_C_INCLUDES += external/stlport/stlport
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_EXECUTABLE)
