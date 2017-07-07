LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := xdelta3patcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../liblzma/api

LOCAL_SRC_FILES := \
xdelta3.c \
nativelink.c 

LOCAL_CFLAGS := -O2 -std=c99 -DHAVE_CONFIG_H -DHAVE_LZMA_H
LOCAL_SHARED_LIBRARIES := lzma
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)  
