LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := asmpatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
xkas.cpp \
nativelink.cpp


LOCAL_CFLAGS := -O3 -Wno-write-strings -Wno-format
LOCAL_CPPFLAGS += -O3 -Wno-write-strings -Wno-format -std=c++11
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)  