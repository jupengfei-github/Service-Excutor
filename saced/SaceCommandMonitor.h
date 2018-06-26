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

#ifndef _SACE_COMMAND_MONITOR_H
#define _SACE_COMMAND_MONITOR_H

#include <sys/socket.h>
#include <vector>

#include "SaceReader.h"
#include "SaceMessage.h"

namespace android {

class SaceCommandMonitor {
    static const char *SOCKET_NAME;
    std::vector<SaceReader*> mReaders;

public:
    SaceCommandMonitor () {
        mReaders.push_back(new SaceSocketReader(SOCKET_NAME, SOCK_STREAM));
        mReaders.push_back(new SaceBinderReader());
    }

    ~SaceCommandMonitor ();

    bool startListen();
    void stopListen();
};

}; //namespace android

#endif
