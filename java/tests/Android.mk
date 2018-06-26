LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_LIBRARIES := sace
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sace_test
LOCAL_CERTIFICATE := platform
include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sace_jt
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := sace_jt
LOCAL_REQUIRED_MODULES := sace_test
include $(BUILD_PREBUILT)
