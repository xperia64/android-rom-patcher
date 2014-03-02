LOCAL_PATH := $(call my-dir)



include $(CLEAR_VARS)

LOCAL_MODULE := xdelta1patcher

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../glib
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../glib/glib
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../glib/android
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libedsio

SRCDIR := $(shell readlink $(LOCAL_PATH))

LOCAL_SRC_FILES := \
$(SRCDIR)/getopt.c \
$(SRCDIR)/getopt1.c \
$(SRCDIR)/xd_edsio.c \
$(SRCDIR)/xdapply.c \
$(SRCDIR)/xdelta.c \
$(SRCDIR)/xdmain.c \
$(SRCDIR)/nativelink.c 

LOCAL_CFLAGS := -O3
LOCAL_SHARED_LIBRARIES := libglib-2.0 edsio
LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)  