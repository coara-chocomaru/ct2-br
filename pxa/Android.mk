LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pxa_fastboot_trigger
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := pxa_fastboot_trigger.cpp
LOCAL_C_INCLUDES += system/core/include
LOCAL_CPPFLAGS += -std=gnu++11 -Os -fno-exceptions -fno-rtti -Wall -Wextra
LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)
