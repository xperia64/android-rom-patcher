LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := bsdiffpatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../bzip2/include

LOCAL_SRC_FILES := \
bspatch.c \
bsdiff.c \
nativelink.c 

LOCAL_CFLAGS := -O3

LOCAL_SHARED_LIBRARIES := bzip2

include $(BUILD_SHARED_LIBRARY)  