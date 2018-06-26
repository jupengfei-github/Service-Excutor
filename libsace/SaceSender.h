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

#ifndef _SACE_SENDER_H
#define _SACE_SENDER_H

#include <semaphore.h>
#include <utils/RefBase.h>

#include "ISaceListener.h"
#include "ISaceManager.h"
#include "SaceLog.h"

namespace android {

class SaceSender : public RefBase {
public:
    class Callback : public RefBase {
    public:
        virtual void onResponse (const SaceStatusResponse &response) = 0;
    };

    virtual SaceResult excuteCommand (const SaceCommand &) = 0;
    virtual ~SaceSender() {}

    void setCallback (sp<Callback> callback) {
        mCallback = callback;
    }
protected:
    /* invoke in other thread */
    void onCommandResponse (const SaceStatusResponse &response) {
        if (mCallback != nullptr)
            mCallback->onResponse(response);
    }

    sp<Callback> mCallback;
};

// ----------------------------------------------------
class SaceBinderSender : public SaceSender {
    static const char *NAME;

    sp<ISaceListener> listener;
    sp<ISaceManager>  manager;
	SaceResult mRlt;

   /* Callback */
    class SaceListenerService : public BnSaceListener {
        SaceBinderSender *self;
    public:
        SaceListenerService (SaceBinderSender *sender) {
            self = sender;
        }

        void onResponsed (const SaceStatusResponse &response) {
            self->onCommandResponse(response);
        }
    };

    bool init();
public:
    virtual SaceResult excuteCommand (const SaceCommand &);

    virtual ~SaceBinderSender() {
        if (manager != nullptr)
            manager->unregisterListener();
    }
};

// ----------------------------------------------------
class SaceSocketSender : public SaceSender {
    static const char *THREAD_NAME;
    static const char *NAME;
    static const int   SEM_WAIT_TIMEOUT;
    static const uint64_t RECV_THREAD_EXIT;

    pthread_t recv_thread;
    pid_t recv_thread_id;

    /* synchronized with recv_thread_run */
    pthread_cond_t syncCond;
    pthread_mutex_t syncMutex;
    uint32_t syncSequence;
    int event_fd;

    string mSockName;
    int mSockType;
    int sockfd;
    SaceStatusResponse mResponse;
    SaceResult mResult;
    bool initlized;

private:
    bool init();
    void uninit();
    void handleResponse (const SaceStatusResponse &response);
    void handleResult (const SaceResult &result);
    static void* recv_thread_run (void *data);

public:
    explicit SaceSocketSender (const char* sock_name, int sock_type) {
        mSockName = string(sock_name);
        mSockType = sock_type;
		initlized = false;
    }

    ~SaceSocketSender () {
        if (initlized)
            uninit();
    }

    SaceResult excuteCommand (const SaceCommand &);
};

}; //namespace android

#endif
