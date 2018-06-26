
# Copyright (C) 2018-2024 The Service-And-Command Excutor Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# libsace.a
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=        \
    SaceTypes.cpp         \
    SaceServiceInfo.cpp   \
    SaceParams.cpp        \
    ISaceListener.cpp     \
    ISaceManager.cpp      \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := liblog libcutils libutils libbinder
LOCAL_MODULE := libsace
include $(BUILD_STATIC_LIBRARY)

# libsace.so
include $(CLEAR_VARS)
LOCAL_SRC_FILES :=        \
    ISaceListener.cpp     \
    ISaceManager.cpp      \
    SaceManager.cpp       \
    SaceSender.cpp        \
	SaceObj.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libbinder
LOCAL_STATIC_LIBRARIES := libsace
LOCAL_MODULE := libsace
include $(BUILD_SHARED_LIBRARY)
