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

#ifndef _SACE_EXCUTOR_H
#define _SACE_EXCUTOR_H

#include <semaphore.h>
#include <vector>
#include <queue>
#include <pthread.h>

#include <SaceTypes.h>
#include <SaceServiceInfo.h>
#include <SaceMessage.h>
#include <SaceLog.h>
#include <sace/SaceParams.h>

#define BASH_PATH "system/bin/sh"

namespace android {

class SaceExcutor {
    static const int DEFAULT_EXCUTOR_TIMEOUT;

    enum SaceMessageHandlerType mMsgType;
    string mName;
    string mThreadName;

    queue<sp<SaceMessageHeader>> mCmdQueue;
    pthread_mutex_t mSyncMutex;
    sem_t mSyncSem;
    bool mExit;
    pthread_t excute_thread;

    static void* excute_command_thread (void*);
    void destroy_excute_thread();

    void sendCommandMessage(sp<SaceMessageHeader>);
    void excuteCommand (sp<SaceMessageHeader>);
public:
    SaceExcutor (enum SaceMessageHandlerType type, const char* name, const char* thread_name) {
        mExit = false;
        mMsgType = type;
        mName = string(name);
        mThreadName = string(thread_name);
    }

    const char* getName() const {
        return mName.c_str();
    }

    const char* getThreadName() const  {
        return mThreadName.c_str();
    }

    bool init();
    void uninit();
    bool excute (sp<SaceMessageHeader>);
    virtual ~SaceExcutor() {}
protected:
    virtual void excuteNormal (sp<SaceMessageHeader>);
    virtual void excuteEvent (sp<SaceMessageHeader>);
    virtual void excuteOther (sp<SaceMessageHeader>);

    virtual bool onInit() { return true; }
    virtual void onUninit() {}

    virtual long receive_msg_timeout () {
        return DEFAULT_EXCUTOR_TIMEOUT; // waiting
    }
    virtual void excuteTimeout(){}
};

// ----------------------------------------------------------------
class SaceServiceExcutor : public SaceExcutor {
class ServiceInfo;
    static const char* THREAD_NAME;
    static const char* NAME;
    static const int   TIMEOUT;

    vector<ServiceInfo*> mRunningService;
    map<uint64_t, ServiceInfo*> mSeqService;
    map<string, ServiceInfo*> mNameService;

    void monitor_service_status();
    void handleServiceInfo (sp<SaceCommand>, sp<SaceWriter>, SaceResult &);

public:
    SaceServiceExcutor():SaceExcutor(SACE_MESSAGE_HANDLER_SERVICE, NAME, THREAD_NAME) {}
    ~SaceServiceExcutor();
protected:
    virtual void excuteNormal (sp<SaceMessageHeader>) override;
    virtual long receive_msg_timeout();
    virtual void excuteTimeout();
    virtual void onUninit();

private:
    class ServiceInfo {
    public:
        string name;
        pid_t pid;
        string startTime;
        enum SaceServiceInfo::ServiceState state;
        string cmdLine;
        vector<sp<SaceWriter>> writer;
        uint64_t label;
        bool request_stop;
        enum SaceServiceFlags flags;

        const string to_string();

        void add_writer (sp<SaceWriter> wr);
        void sendResponse (SaceStatusResponse& response);
    private:
        string sveDescriptor;
    };
};

// -------------------------------------------------------------

class SaceNormalExcutor : public SaceExcutor {
class CommandInfo;
    static const char* THREAD_NAME;
    static const char* NAME;

    vector<CommandInfo*> mRunningCmd;
    vector<CommandInfo*> mFinishedCmd;
    map<uint64_t, CommandInfo*> mSeqCmd;
public:
    SaceNormalExcutor():SaceExcutor(SACE_MESSAGE_HANDLER_NORMAL, NAME, THREAD_NAME) {}
    ~SaceNormalExcutor();
protected:
    virtual void excuteNormal (sp<SaceMessageHeader>) override;
    virtual void onUninit();

private:
    void startNormalCmd (sp<SaceReaderMessage>);
    void closeNormalCmd (sp<SaceReaderMessage>);

    class CommandInfo {
    public:
        int fd;
        string cmdLine;
        uint64_t label;
        sp<SaceWriter> writer;
        pid_t pid;
    };
};

void set_proc_capability (CapSet &);
void handle_command_params (sp<SaceCommandParams>);

static pthread_rwlock_t pidlist_lock = PTHREAD_RWLOCK_INITIALIZER;
static struct pid {
    struct pid *next;
    int fd;
    pid_t pid;
} *pidlist;

int sace_popen(const char *, const char *, sp<SaceCommandParams>);
int sace_pclose (int fd);

}; //namespace android
#endif
