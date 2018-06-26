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

#ifndef _SACE_CONTROL_CENTER_H
#define _SACE_CONTROL_CENTER_H

#include <cutils/list.h>
#include <utils/Looper.h>
#include <utils/RefBase.h>
#include <vector>

#include "SaceMessage.h"
#include "SaceExcutor.h"

namespace android {

// Dispatch Command -----------------------------------------------------
class SaceCommandDispatcher {
    static const int   POLL_WAIT_TIMEOUT;
    static const char* NAME;

    static shared_ptr<SaceCommandDispatcher> mInstance;
    sp<Looper> mLooper;
    vector<SaceExcutor*> mExcutor;
    bool mExit;
    pthread_t dispatch_thread_t;

public:
    SaceCommandDispatcher ();

    static shared_ptr<SaceCommandDispatcher> getInstance ();
    friend class MessageDistributable;

    void handleMessage (sp<SaceMessageHeader> msg);
    void handleDefaultMessage (sp<SaceMessageHeader> msg);
    bool start();
    void stop();

private:
    static void *dispatch_thread (void *data);
};

// Put Message -----------------------------------------------------------------------
class MessageDistributable {
    sp<Looper> mLooper;
    shared_ptr<SaceCommandDispatcher> mDispatcher;

public:
    explicit MessageDistributable () {
        mDispatcher = SaceCommandDispatcher::getInstance();
        mLooper     = mDispatcher->mLooper;
    }

protected:
    void post(sp<SaceMessageHeader> msg);

private:
    class MessageHandlerWrapper : public MessageHandler {
        MessageDistributable *self;
        sp<SaceMessageHeader> msg;
        void handleMessage (const Message &message);
    public:
        static const int MESSAGE_HANDLER_ID;
        MessageHandlerWrapper (MessageDistributable *self, sp<SaceMessageHeader> message) {
            this->self = self;
            this->msg  = message;
        }
    };
};

}; //namespace android

#endif
