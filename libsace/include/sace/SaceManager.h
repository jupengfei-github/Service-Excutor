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

#ifndef _SACE_MANAAGER_H
#define _SACE_MANAAGER_H

#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <memory>

#include "../../SaceSender.h"
#include "SaceObj.h"

#define SACE_SENDER_SOCKEET 0x01
#define SACE_SENDER_BINDER  0x02

namespace android {

class SaceManagerCallback : public RefBase {
public:
    virtual void handleServiceResponse (sp<SaceServiceObj> sve, const ServiceResponse &sve_response) = 0;
    virtual void handleCommandResponse (sp<SaceCommandObj> cmd, const CommandResponse &cmd_response) = 0;
};

class SaceManager : public SaceSender::Callback {
    map<uint64_t, sp<SaceServiceObj>> mServices;
    map<uint64_t, sp<SaceCommandObj>> mCommands;
    sp<SaceSender> mSender;
    Mutex mMutex;
    SaceCommand mCmd;
    SaceResult  mRlt;
    sp<SaceManagerCallback> mCallback;
    static SaceManager *mInstance;
    shared_ptr<SaceCommandParams> cmd_param;
    shared_ptr<SaceCommandParams> service_param;
    shared_ptr<SaceEventParams>   event_param;

    SaceManager ();
    ~SaceManager ();

    sp<SaceServiceObj> queryService (const char* name);
    sp<SaceServiceObj> queryEventService (const char* name);
public:
    static SaceManager* getInstance ();

    void setCallback (sp<SaceManagerCallback> callback) {
        mCallback = callback;
    }

    sp<SaceCommandObj> runCommand (const char* cmd, shared_ptr<SaceCommandParams> = nullptr, bool in = true);
    sp<SaceServiceObj> checkService (const char* name, const char* cmd = nullptr, shared_ptr<SaceCommandParams> params = nullptr);
    int addEvent (const char* name, const char* cmd, shared_ptr<SaceEventParams> param = nullptr);
    int deleteEvent (const char* name, bool stop = true);
protected:
    virtual void onResponse (const SaceStatusResponse &response);
};

}; //namespace android

#endif
