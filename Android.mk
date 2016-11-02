
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES:= udooserver.c
LOCAL_MODULE := udoofota
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
