LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:= libstc-rpc
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SRC_FILES:= \
	stc_rpc.c

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils

LOCAL_CFLAGS :=

include $(BUILD_STATIC_LIBRARY)
