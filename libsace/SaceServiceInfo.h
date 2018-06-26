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

#ifndef _SACE_SERVICE_INFO_H
#define _SACE_SERVICE_INFO_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string>

using namespace std;

#define EXTRA_BUFER_LEN  256
#define CMD_BUFFER_LEN   128
#define SERVICE_NAME_LEN 64

#if CMD_BUFFER_LEN + EXTRA_BUFER_LEN > 1024
    #error "CMD_BUFFER_LEN And EXTRA_BUFER_LEN Larger Than 1k"
#endif

#if CMD_BUFFER_LEN + SERVICE_NAME_LEN + 4*2 >= EXTRA_BUFER_LEN
    #error "EXTRA_BUFER_LEN TOO SMALL"
#endif

namespace android {
class SaceServiceExcutor;

class SaceServiceInfo {
public:
    enum ServiceState {
        SERVICE_PAUSED,             /* Service Stoped By User */
        SERVICE_STOPED,             /* Service Paused By User */
        SERVICE_RUNNING,            /* Service Running */
        SERVICE_DIED,               /* Service Exit By Self Error */
        SERVICE_DIED_SIGNAL,        /* Service Exit By SIGNAL */
        SERVICE_DIED_UNKNOWN,       /* Service Exit By Unknown */
        SERVICE_FINISHED,           /* Service Finished By Itself */
        SERVICE_FINISHING_USER,     /* Service Finishing By User */
        SERVICE_FINISHED_USER,      /* Service Finished By User */
    };

    /* Info Command */
    static const string SERVICE_GET_BY_NAME;
	static const string SERVICE_GET_BY_LABEL;

    static string mapStateStr (enum ServiceState);

    /* sizeof(ServiceInfo) < EXTRA_BUFER_LEN */
    struct ServiceInfo {
        char name[SERVICE_NAME_LEN];
        char cmd[CMD_BUFFER_LEN];
        enum ServiceState state;
        uint64_t label;
    };
};

class ServiceResponse {
public:
    string name;
    string cmdLine;
    enum SaceServiceInfo::ServiceState state;
	uint64_t label;
};

class CommandResponse {
public:
	string cmdLine;
	uint64_t label;
};

}; //namespace android

#endif
