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

#ifndef _SACE_WRITER_H
#define _SACE_WRITER_H

#include <string>
#include <utils/RefBase.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <ISaceListener.h>
#include <SaceTypes.h>
#include <SaceLog.h>

namespace android {

class SaceWriter : public virtual RefBase {
    string mName;
    pid_t  mId;
protected:
    const char* getName () {
        return mName.c_str();
    }
public:
    explicit SaceWriter (const char *name, const pid_t pid) {
        mName = string(name);
        mId = pid;
    }

    explicit SaceWriter (const char* name, const pid_t pid, int type):SaceWriter(name, pid) {
        mId = (type << 8) | mId;
    }

    virtual ~SaceWriter() {}

    virtual void sendResult (const SaceResult &) = 0;
    virtual void sendResponse (const SaceStatusResponse &) = 0;

    bool operator == (SaceWriter* writer) const {
        return writer == nullptr? false : mId == writer->mId;
    }

    bool operator == (sp<SaceWriter> writer) const {
        return operator==(writer.get());
    }
};

// ---------------------------------------------------------
class SaceSocketWriter : public SaceWriter {
    int sockfd;
public:
    explicit SaceSocketWriter (const char* name, pid_t pid, int fd):SaceWriter(name, pid) {
        sockfd = fd;
    }
    virtual ~SaceSocketWriter() {}

    virtual void sendResult (const SaceResult &);
    virtual void sendResponse (const SaceStatusResponse &);
};

// ---------------------------------------------------------
class SaceBinderWriter : public SaceWriter {
    ISaceListener *listener;
    SaceResult    *result;
    sem_t         sync_sem;
public:
    explicit SaceBinderWriter (const char* name, pid_t pid, ISaceListener *listener, SaceResult *rslt):SaceWriter(name, pid) {
        this->listener = listener;
        this->result   = rslt;

        if (sem_init(&sync_sem, 0, 0) < 0)
            SACE_LOGE("%s sem_init errno=%d errstr=%s", getName(), errno, strerror(errno));
    }
    virtual ~SaceBinderWriter() {}

    virtual void sendResult (const SaceResult &);
    virtual void sendResponse (const SaceStatusResponse &);

    void waitResult();
};

}; //namespace android

#endif
