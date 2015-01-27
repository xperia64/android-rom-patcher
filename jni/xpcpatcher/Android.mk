LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := xpcpatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
xpcapply.c \
nativelink.c 

LOCAL_CFLAGS := -O3

include $(BUILD_SHARED_LIBRARY)  