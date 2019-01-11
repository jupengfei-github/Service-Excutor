LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CPPFLAGS += -fexceptions
LIB_SACE_C_INCLUDE := $(LOCAL_PATH)/../libsace/include
LOCAL_SRC_FILES  := test_cmd.cpp
LOCAL_C_INCLUDES := $(LIB_SACE_C_INCLUDE)
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libbinder libsace
LOCAL_MODULE := test_cmd
include $(BUILD_EXECUTABLE)
