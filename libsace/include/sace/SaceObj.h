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

#ifndef _SACE_OBJ_H
#define _SACE_OBJ_H

#include <stdio.h>
#include <string>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include "../../SaceSender.h"
#include "../../SaceTypes.h"
#include "../../SaceLog.h"
#include "../../SaceServiceInfo.h"
#include "SaceError.h"

namespace android {

enum ErrorCode {
    ERR_OK,
    ERR_TIMEOUT,
    ERR_EXIT,
    ERR_EXIT_USER,
    ERR_UNKNOWN,
};

class SaceCmdObj : public RefBase {
    sp<SaceSender> mCmdSender;
    enum ErrorCode mError;
    Mutex error_mutex;

public:
    SaceCmdObj (sp<SaceSender> obj) {
        mCmdSender = obj;
        mError = ERR_OK;
    }

    enum ErrorCode getError () {
        AutoMutex _lock(error_mutex);
        return mError;
    }

protected:
    SaceResult excute (SaceCommand &cmd) {
        return mCmdSender->excuteCommand(cmd);
    }

    void setError (enum ErrorCode code) {
        AutoMutex _lock(error_mutex);
        mError = code;
    }
};

// ----------------------------------------------
class SaceManager;
class SaceCommandObj : public SaceCmdObj {
    string cmd;
    uint64_t label;
    int fd;
    bool in;

    friend class SaceManager;
public:
    SaceCommandObj (sp<SaceSender> obj, uint64_t label, const char* cmd, int fd, bool in):SaceCmdObj(obj) {
        this->label = label;
        this->fd  = fd;
        this->cmd = cmd;
        this->in  = in;
    }

    ~SaceCommandObj () {
        close();
    }

    string getCmd() { 
        return cmd; 
    }

    int read (char *buf, int len) throw (UnsupportedOperation, RemoteException);
    int write (char *buf, int len) throw (UnsupportedOperation, RemoteException);
    void close();
};

// ----------------------------------------------

class SaceServiceObj : public SaceCmdObj {
    uint64_t label;
    string name;
    string command;
    SaceCommand mCmd;
    SaceResult  mRlt;

    friend class SaceManager;
public:
    SaceServiceObj (sp<SaceSender> obj, uint64_t label, const char* name, const char* cmd):SaceCmdObj(obj) {
        this->label = label;
        this->name  = string(name);
        command = string(cmd);
    }

    bool stop() throw (RemoteException);
    bool pause() throw (RemoteException);
    bool restart() throw (RemoteException);

    string getName() {
        return name;
    }

    string getCmd() {
        return command;
    }

    enum SaceServiceInfo::ServiceState getState();
};

}; //namespace android

#endif
