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

#include "../../SaceSender.h"
#include "../../SaceTypes.h"
#include "../../SaceLog.h"
#include "../../SaceServiceInfo.h"

namespace android {

class SaceCmdObj : public RefBase {
    sp<SaceSender> mCmdSender;
public:
    SaceCmdObj (sp<SaceSender> obj) {
        mCmdSender = obj;
    }

protected:
    SaceResult excute (SaceCommand &cmd) {
        return mCmdSender->excuteCommand(cmd);
    }
};

// ----------------------------------------------
class SaceCommandObj : public SaceCmdObj {
    string cmd;
    uint64_t label;
    int fd;
    bool in;
public:
    SaceCommandObj ():SaceCmdObj(nullptr) {
        fd    = -1;
        label = 0;
    }

    SaceCommandObj (sp<SaceSender> obj, uint64_t label, const char* cmd, int fd, bool in):SaceCmdObj(obj) {
        this->label = label;
        this->fd  = fd;
        this->cmd = string(cmd);
        this->in  = in;
    }

    ~SaceCommandObj () {
        close();
    }

    string getCmd() { return cmd; }
    int read(char *buf, int len);
    int write(char *buf, int len);
    bool isOk();
    void close();
    void flush();
};

// ----------------------------------------------
class SaceManager;
class SaceServiceObj : public SaceCmdObj {
    uint64_t label;
    string name;
    string command;
	SaceCommand mCmd;
	SaceResult  mRlt;

public:
    SaceServiceObj ():SaceCmdObj(nullptr) {
        label = 0;
        name  = string("unknown");
        command = string("unknown");
    }

    SaceServiceObj (sp<SaceSender> obj, uint64_t label, const char* name, const char* cmd):SaceCmdObj(obj) {
        this->label = label;
        this->name  = string(name);
        command = string(cmd);
    }

    bool stop();
    bool pause();
    bool restart();
    string getName() { return name; }
    string getCmd() { return command; }
    enum SaceServiceInfo::ServiceState getState();
};

}; //namespace android

#endif
