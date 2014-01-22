LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := bsdiffpatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../bzip2/include

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/bspatch.c \
$(SRCDIR)/bsdiff.c \
$(SRCDIR)/nativelink.c 

LOCAL_CFLAGS := -O3

LOCAL_SHARED_LIBRARIES := bzip2

include $(BUILD_SHARED_LIBRARY)  