LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := xdelta3patcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/xdelta3.c \
$(SRCDIR)/nativelink.c 

LOCAL_CFLAGS := -O3

include $(BUILD_SHARED_LIBRARY)  