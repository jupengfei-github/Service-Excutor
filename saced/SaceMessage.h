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

#ifndef _SACE_QUEUE_H
#define _SACE_QUEUE_H

#include <pthread.h>
#include <utils/Looper.h>
#include <cutils/list.h>
#include <utils/RefBase.h>

#include <SaceTypes.h>

namespace android {
class SaceWriter;

// ------------------------------------------------------------
enum SaceMessageHandlerType {
    SACE_MESSAGE_HANDLER_UNKOWN,
    SACE_MESSAGE_HANDLER_NORMAL,
    SACE_MESSAGE_HANDLER_SERVICE,
    SACE_MESSAGE_HANDLER_EVENT,
    SACE_MESSAGE_HANDLER_ALL,
};

enum SaceMessageType {
    SACE_MESSAGE_TYPE_NORMAL,
    SACE_MESSAGE_TYPE_EVENT,
};

class SaceMessageHeader : public RefBase {
public:
    enum SaceMessageHandlerType msgHandler;
    enum SaceMessageType msgType;

    SaceMessageHeader (enum SaceMessageType type) {
        msgType = type;
    }

    const string to_string ();

    static string mapIdToName (enum SaceMessageHandlerType handler);
    static string mapTypeToName (enum SaceMessageType type);
private:
    string msgDescriptor;
};

/* Normal Message */
class SaceReaderMessage : public SaceMessageHeader {
public:
    sp<SaceCommand> msgCmd;
    sp<SaceWriter>  msgWriter;

    SaceReaderMessage ();
    const string to_string ();
private:
    string msgDescriptor;
};

enum SaceEventMessageType {
    SACE_EVENT_TYPE_SIGCHLD,
    SACE_EVENT_TYPE_UNKOWN,
};

/* Event Message */
class SaceEventMessage : public SaceMessageHeader {
public:
    enum SaceEventMessageType msgEvent;

    SaceEventMessage ():SaceMessageHeader(SACE_MESSAGE_TYPE_EVENT) {}

    const string to_string ();
    static string mapEventToName (enum SaceEventMessageType type);
private:
    string msgDescriptor;
};

}; //namespace android

#endif
