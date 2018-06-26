
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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_INIT_RC := sace.rc
#PRODUCT_COPY_FILES += $(LOCAL_PATH)/sace_event.ini:system/etc/sace_event.ini

LIB_SACE_INCLUDE = $(LOCAL_PATH)/../libsace
LOCAL_SRC_FILES :=               \
	SaceCommandDispatcher.cpp    \
	SaceCommandMonitor.cpp       \
	SaceEvent.cpp 				 \
	SaceExcutor.cpp				 \
	sace_main.cpp				 \
	SaceMessage.cpp				 \
	SaceReader.cpp				 \
	SaceWriter.cpp				 \

LOCAL_C_INCLUDES := $(LIB_SACE_INCLUDE)
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libbinder libselinux libcap
LOCAL_STATIC_LIBRARIES := libsace
LOCAL_MODULE := saced
include $(BUILD_EXECUTABLE)
