LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := bpspatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/bps
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/dsp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/emulation
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/mosaic
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/stream
LOCAL_C_INCLUDES += $(LOCAL_PATH)/nall/string

LOCAL_SRC_FILES := \
nativelink.cpp

LOCAL_CFLAGS := -O3
LOCAL_CPPFLAGS := -std=c++11 -frtti -fexceptions
LOCAL_CPPFLAGS += -O3

include $(BUILD_SHARED_LIBRARY)  