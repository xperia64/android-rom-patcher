LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := dldipatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)


LOCAL_SRC_FILES := \
dlditool.c \
nativelink.c 

LOCAL_CFLAGS := -O3

include $(BUILD_SHARED_LIBRARY)  