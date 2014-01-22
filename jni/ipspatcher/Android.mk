LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := ipspatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/ips.c \
$(SRCDIR)/nativelink.c 

LOCAL_CFLAGS := -O3

include $(BUILD_SHARED_LIBRARY)  