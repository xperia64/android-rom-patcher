LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := upspatcher

LOCAL_CPP_INCLUDES := $(LOCAL_PATH)
LOCAL_CPP_INCLUDES += $(LOCAL_PATH)/nall
LOCAL_CPP_INCLUDES += $(LOCAL_PATH)/nall/string

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/libups.cpp \
$(SRCDIR)/nativelink.cpp

LOCAL_CFLAGS := -O3

LOCAL_CPPFLAGS := -O3

include $(BUILD_SHARED_LIBRARY)  