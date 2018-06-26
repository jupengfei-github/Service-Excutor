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

#ifndef _SACE_READER_H
#define _SACE_READER_H

#include <string>
#include <utils/Thread.h>
#include <map>

#include "SaceMessage.h"
#include "SaceCommandDispatcher.h"
#include <ISaceManager.h>
#include <ISaceListener.h>

#define MAX_CLIENT_NUM 5

namespace android {

/* monitor message from outer */
class SaceReader {
    string mName;
public:
    explicit SaceReader (const char *name) {
        mName = string(name);
    }

    virtual bool startRead() = 0;
    virtual void stopRead() = 0;
    virtual ~SaceReader() {}
protected:
    const char* getName() const {
        return mName.c_str();
    }
};

// ---------------------------------------
class SaceSocketReader : public SaceReader, public MessageDistributable {
    static const char *NAME;
    static const char *THREAD_NAME;
    static const char *WRITER_NAME;
    static const int  MONITOR_TIMEOUT;

    int mSockFd;
    int mSockType;
    Thread *mThread;
    string mSockName;

public:
    SaceSocketReader (const char *sock_name, const int sock_type):SaceReader(NAME) {
        mSockName = string(sock_name);
        mSockType = sock_type;

        for (int i = 0; i < MAX_CLIENT_NUM; i++) {
            memset((void*)&mClients[i], 0x0, sizeof(ClientSocket));
            mClients[i].fd = -1;
        }

        mSockFd = -1;
    }

    virtual bool startRead();
    virtual void stopRead();

    ~SaceSocketReader() {
        for (int i = 0; i < MAX_CLIENT_NUM; i++)
            if (mClients[i].fd > 0) close(mClients[i].fd);
    }
private:
    /* listen client message */
    class MonitorThread : public Thread {
        SaceSocketReader *mReader;
    public:
        MonitorThread (SaceSocketReader *reader) {
            mReader = reader;
        }
    protected:
        status_t readyToRun();
        bool threadLoop();
    };

    struct ClientSocket {
        int fd;
    };

    struct ClientMsg {
        int  fd;
        pid_t pid;
        uid_t uid;
        sp<SaceCommand> command;

        ClientMsg () {
            command = new SaceCommand();
        }

        ClientMsg (const ClientMsg &msg) {
            fd  = msg.fd;
            pid = msg.pid;
            uid = msg.uid;
            command = msg.command;
        }
    };

    /* child thread */
    ClientSocket mClients[MAX_CLIENT_NUM];

    int setup_socket();
    void close_socket();
    void handle_socket_msg(ClientMsg &);
    bool recv_data_or_connection();
};

// --------------------------------------
class SaceBinderReader : public SaceReader {
class SaceManagerService;

    static const char *NAME;
    int mBinderFd;
    sp<SaceManagerService> mSaceManager;
public:
    explicit SaceBinderReader ():SaceReader(NAME) {
        mSaceManager = new SaceManagerService();
    }

    virtual bool startRead();
    virtual void stopRead();

    ~SaceBinderReader() {
        mSaceManager = nullptr;
        mBinderFd    = -1;
    }
private:
    void handle_client_msg (SaceMessageHeader msg);
    void publish (const sp<SaceManagerService> &service);

    /* SACE Binder Service */
    class SaceManagerService : public BnSaceManager, public MessageDistributable {
        bool mExit;
        map<pid_t, ISaceListener*> mPidListener;
    public:
        ~SaceManagerService () {
            for (map<pid_t, ISaceListener*>::iterator it = mPidListener.begin(); it != mPidListener.end(); it++)
                delete it->second;
        }

        virtual SaceResult sendCommand (SaceCommand);
        virtual void registerListener (sp<ISaceListener> listener);
        virtual void unregisterListener ();

		void stop () {
            mExit = true;
        }
    };
};

}; //namespace android

#endif
