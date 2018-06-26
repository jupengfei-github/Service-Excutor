
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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIB_SACE_C_INCLUDE := $(LOCAL_PATH)/../../libsace/include
LOCAL_SRC_FILES := com_android_sace_SaceService.cpp \
	com_android_sace_SaceCommand.cpp \
	com_android_sace_SaceManager.cpp

LOCAL_C_INCLUDES := $(LIB_SACE_C_INCLUDE)
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libbinder libsace
LOCAL_MODULE := libsace_jni
include $(BUILD_SHARED_LIBRARY)
