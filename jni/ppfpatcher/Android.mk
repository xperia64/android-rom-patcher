LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := ppfpatcher

LOCAL_CPP_INCLUDES := $(LOCAL_PATH)

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/nativelink.cpp

LOCAL_CFLAGS := -O3 -Wmultichar

LOCAL_CPPFLAGS := -O3 -Wmultichar

include $(BUILD_SHARED_LIBRARY)  