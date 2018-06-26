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

#include "SaceMessage.h"
#include "SaceLog.h"
#include "SaceWriter.h"

namespace android {

// -------------------- SaceMessageHandler --------------
string SaceMessageHeader::mapIdToName (enum SaceMessageHandlerType handler) {
    switch (handler) {
        case SACE_MESSAGE_HANDLER_NORMAL:
            return string("SACE_MESSAGE_HANDLER_NORMAL");
        case SACE_MESSAGE_HANDLER_SERVICE:
            return string("SACE_MESSAGE_HANDLER_SERVICE");
        case SACE_MESSAGE_HANDLER_EVENT:
            return string("SACE_MESSAGE_HANDLER_EVENT");
        case SACE_MESSAGE_HANDLER_ALL:
            return string("SACE_MESSAGE_HANDLER_ALL");
        default:
            return string("UNKNOWN");
    }
}

string SaceMessageHeader::mapTypeToName (enum SaceMessageType type) {
    switch (type) {
        case SACE_MESSAGE_TYPE_NORMAL:
            return string("SACE_MESSAGE_HANDLER_NORMAL");
        case SACE_MESSAGE_TYPE_EVENT:
            return string("SACE_MESSAGE_HANDLER_EVENT");
        default:
            return string("UNKNOWN");
    }
}

const string SaceMessageHeader::to_string () {
    if (!msgDescriptor.empty())
        return msgDescriptor;

    char buf[100];
    snprintf(buf, sizeof(buf), "SaceMessageHeader={ msgHandler=%s msgType=%s }",
        SaceMessageHeader::mapIdToName(msgHandler).c_str(),
        SaceMessageHeader::mapTypeToName(msgType).c_str());

    msgDescriptor = string(buf);
    return msgDescriptor;
}

// ----------------- SaceReaderMessage ----------------
const string SaceReaderMessage::to_string () {
    if (!msgDescriptor.empty())
        return msgDescriptor;

    char buf[1024];
    snprintf(buf, sizeof(buf), "SaceReaderMessage={ msgHandler=%s msgType=%s msgCmd=%s }",
		SaceMessageHeader::mapIdToName(msgHandler).c_str(), 
        SaceMessageHeader::mapTypeToName(msgType).c_str(), msgCmd->to_string().c_str());

    msgDescriptor = string(buf);
    return msgDescriptor;
}

// ------------- SaceEventMessage ------------------
SaceReaderMessage::SaceReaderMessage ():SaceMessageHeader(SACE_MESSAGE_TYPE_NORMAL) {}

const string SaceEventMessage::to_string () {
    if (!msgDescriptor.empty())
        return msgDescriptor;

    char buf[1024];
    snprintf(buf, sizeof(buf), "SaceEventMessage={ msgHandler=%s msgType=%s msgEvent=%s }",
        SaceMessageHeader::mapIdToName(msgHandler).c_str(),
        SaceMessageHeader::mapTypeToName(msgType).c_str(),
        SaceEventMessage::mapEventToName(msgEvent).c_str());

    msgDescriptor = string(buf);
    return msgDescriptor;
}

string SaceEventMessage::mapEventToName (enum SaceEventMessageType type) {
    switch (type) {
        case SACE_EVENT_TYPE_SIGCHLD:
            return "SACE_EVENT_TYPE_SIGCHLD";
        case SACE_EVENT_TYPE_UNKOWN:
            return "SACE_EVENT_TYPE_UNKOWN";
        default:
            return "UNKNOWN";
    }
}

}; //namespace android

