/*
 * Copyright (C) 2018-2024 The Service-And-Command Excutor Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SACE_LOG_H
#define _SACE_LOG_H

#include <log/log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "SACE"
#endif

#define SACE_LOGE(...)  ALOGE(__VA_ARGS__)
#define SACE_LOGD(...)  ALOGD(__VA_ARGS__)
#define SACE_LOGI(...)  ALOGI(__VA_ARGS__)
#define SACE_LOGW(...)  ALOGW(__VA_ARGS__)

#endif
