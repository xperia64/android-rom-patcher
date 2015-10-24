LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := asarpatcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
arch-65816.cpp \
arch-spc700.cpp \
arch-superfx.cpp \
assembleblock.cpp \
interface-cli.cpp \
libcon.cpp \
libsmw.cpp \
libstr.cpp \
macro.cpp \
main.cpp \
math.cpp \
mathlib.cpp \
nativelink.cpp


LOCAL_CFLAGS := -O3 -Wno-write-strings -Wno-format
LOCAL_CPPFLAGS += -O3 -Wno-write-strings -Wno-format -std=c++11 -DINTERFACE_CLI -fexceptions
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)  