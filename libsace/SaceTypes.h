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

#ifndef _SACE_TYPE_H
#define _SACE_TYPE_H

#include <binder/Parcelable.h>
#include <binder/Parcel.h>
#include <utils/RefBase.h>

#include "SaceServiceInfo.h"
#include <sace/SaceParams.h>

using namespace std;

#define SACE_RESULT_BUF_SIZE  1024

/* Note : we consider ENUM as uint8_t when parcelable
 */
namespace android {

enum SaceCommandType {
    SACE_TYPE_NORMAL,
    SACE_TYPE_SERVICE,
    SACE_TYPE_EVENT,
};

class SaceCommandHeader : public Parcelable, public RefBase {
private:
    static uint16_t mSequence;
public:
    /* record receive dataSize while tranfer */
    uint32_t len;
    enum SaceCommandType type;
    uint32_t sequence;

    SaceCommandHeader () {
        init();
    }

    SaceCommandHeader (const SaceCommandHeader &command) {
        len  = command.len;
        type = command.type;
        sequence = command.sequence;
    }

    void init () {
        len  = 0;
        type = SACE_TYPE_NORMAL;
        sequence = gettid() << 16 | ++mSequence;
    }

    SaceCommandHeader& operator= (const SaceCommandHeader &header) {
        len  = header.len;
        type = header.type;
        sequence = header.sequence;

        return *this;
    }

    // must invoke after SubClass
    virtual status_t writeToParcel (Parcel *data) const override;
    // must invoke before SubClass
    virtual status_t readFromParcel (const Parcel *data) override;
    // must invoke before SubClass
    void reserveSpace (Parcel *data) const;

    static uint32_t parcelSize();
    static string mapCmdTypeStr (enum SaceCommandType type);
};

enum SaceServiceCommandType {
    SACE_SERVICE_CMD_START,
    SACE_SERVICE_CMD_STOP,
    SACE_SERVICE_CMD_PAUSE,
    SACE_SERVICE_CMD_RESTART,
    SACE_SERVICE_CMD_INFO,
};

enum SaceNormalCommandType {
    SACE_NORMAL_CMD_START,
    SACE_NORMAL_CMD_CLOSE,
};

enum SaceEventType {
    SACE_EVENT_TYPE_ADD,
    SACE_EVENT_TYPE_DEL,
    SACE_EVENT_TYPE_INFO,
};

enum SaceServiceFlags {
    SACE_SERVICE_FLAG_NORMAL,
    SACE_SERVICE_FLAG_EVENT,
};

enum SaceCommandFlags {
    SACE_CMD_FLAG_IN,
    SACE_CMD_FLAG_OUT,
};

enum SaceEventFlags {
    SACE_EVENT_FLAG_NONE,
    SACE_EVENT_FLAG_RESTART,
};

class SaceCommand : public SaceCommandHeader {
public:
    uint64_t label;
    string name;
    string command;
    shared_ptr<SaceCommandParams> command_params;
    uint8_t  extra[EXTRA_BUFER_LEN];
    uint32_t extraLen;

    union {
        /* SACE_TYPE_SERVICE */
        struct {
            enum SaceServiceCommandType serviceCmdType;
            enum SaceServiceFlags serviceFlags;
        };

        /* SACE_TYPE_NORMAL */
        struct {
            enum SaceNormalCommandType normalCmdType;
            enum SaceCommandFlags flags;
        };

        /* SACE_TYPE_EVENT */
        struct {
            enum SaceEventType  eventType;
            enum SaceEventFlags eventFlags;
        };
    };

    void init() {
        SaceCommandHeader::init();

        label = 0;
        name = ::to_string(getpid()).append(":").append(::to_string(gettid()));
        memset(extra, 0x00, sizeof(extra));
        command = "";
        command_params = nullptr;
        extraLen = 0;
        normalCmdType = SACE_NORMAL_CMD_START;
        flags = SACE_CMD_FLAG_IN;
    }

    SaceCommand ():SaceCommandHeader() {
        init();
    }

    SaceCommand (const SaceCommand &cmd):SaceCommandHeader(cmd) {
        label = cmd.label;
        name  = cmd.name;
        command = cmd.command;
        command_params = cmd.command_params;
        extraLen = cmd.extraLen;
        memcpy(extra, cmd.extra, extraLen);

        if (type == SACE_TYPE_SERVICE) {
            serviceCmdType = cmd.serviceCmdType;
            serviceFlags   = cmd.serviceFlags;
        }
        else if (type == SACE_TYPE_NORMAL) {
            normalCmdType = cmd.normalCmdType;
            flags = cmd.flags;
        }
        else if (type == SACE_TYPE_EVENT) {
            eventType  = cmd.eventType;
            eventFlags = cmd.eventFlags;
        }
    }

    SaceCommand& operator= (const SaceCommand &cmd) {
        SaceCommandHeader::operator=(cmd);

        label    = cmd.label;
        name     = cmd.name;
        command  = cmd.command;
        extraLen = cmd.extraLen;
        memcpy(extra, cmd.extra, extraLen);
        command_params = cmd.command_params;

        if (type == SACE_TYPE_SERVICE) {
            serviceCmdType = cmd.serviceCmdType;
            serviceFlags   = cmd.serviceFlags;
        }
        else if (type == SACE_TYPE_NORMAL) {
            normalCmdType = cmd.normalCmdType;
            flags = cmd.flags;
        }
        else if (type == SACE_TYPE_EVENT) {
            eventType  = cmd.eventType;
            eventFlags = cmd.eventFlags;
        }

        return *this;
    }

    virtual status_t writeToParcel (Parcel *data) const override;
    virtual status_t readFromParcel (const Parcel *data) override;

    static string mapCmdTypeStr(enum SaceCommandType type);
    static string mapServiceCmdTypeStr(enum SaceServiceCommandType type);
    static string mapServiceFlagStr (enum SaceServiceFlags flag);
    static string mapNormalCmdTypeStr(enum SaceNormalCommandType type);
    static string mapNormalCmdFlagStr(enum SaceCommandFlags flag);
    static string mapEventFlagStr (enum SaceEventFlags flag);
    static string mapEventTypeStr (enum SaceEventType type);

    const string to_string() const;
};

// ----------------------------------------------------
enum SaceResultHeaderType {
    SACE_BASE_RESULT_TYPE_NORMAL,
    SACE_BASE_RESULT_TYPE_RESPONSE,
};

class SaceResultHeader : public Parcelable {
public:
    enum SaceResultHeaderType type;
    /* record receive dataSize while tranfer */
    uint32_t len;

    SaceResultHeader (enum SaceResultHeaderType type = SACE_BASE_RESULT_TYPE_NORMAL) {
        this->type = type;
        this->len  = 0;
    }

    SaceResultHeader (const SaceResultHeader &header):Parcelable(header) {
        type = header.type;
        len  = header.len;
    }

    // must invoke after SubClass
    virtual status_t writeToParcel (Parcel *data) const override;
    // must invoke before SubClass
    virtual status_t readFromParcel (const Parcel *data) override;
    // must invoke before SubClass
    void reserveSpace (Parcel *data) const;

    static uint32_t parcelSize();
};

enum SaceResultStatus {
    SACE_RESULT_STATUS_OK,
    SACE_RESULT_STATUS_TIMEOUT,
    SACE_RESULT_STATUS_FAIL,
    SACE_RESULT_STATUS_SECURE,
    SACE_RESULT_STATUS_EXISTS,
};

enum SaceResultType {
    SACE_RESULT_TYPE_NONE,
    SACE_RESULT_TYPE_FD,
    SACE_RESULT_TYPE_EXTRA,
    SACE_RESULT_TYPE_LABEL,
};

class SaceResult : public SaceResultHeader {
public:
    uint32_t sequence;   // Corresponding to command
    string name;         // Command|Service name
    enum SaceResultType   resultType;
    enum SaceResultStatus resultStatus;
    uint8_t  resultExtra[EXTRA_BUFER_LEN];
    uint32_t resultExtraLen;
    int resultFd;

    virtual status_t writeToParcel (Parcel *data) const override;
    virtual status_t readFromParcel (const Parcel *data) override;

    static string mapResultTypeStr (enum SaceResultType type);
    static string mapResultStatusStr (enum SaceResultStatus status);

    SaceResult ():SaceResultHeader(SACE_BASE_RESULT_TYPE_NORMAL) {
        sequence = 0;
        name = "";
        resultType   = SACE_RESULT_TYPE_NONE;
        resultStatus = SACE_RESULT_STATUS_OK;
        resultExtraLen = 0;
        memset(resultExtra, 0, EXTRA_BUFER_LEN);
    }

    SaceResult (const SaceResult &rslt):SaceResultHeader(rslt) {
        sequence = rslt.sequence;
        name = rslt.name;
        resultType   = rslt.resultType;
        resultStatus = rslt.resultStatus;
        resultFd = rslt.resultFd;
        resultExtraLen = rslt.resultExtraLen;
        memcpy(resultExtra, rslt.resultExtra, resultExtraLen);
    }

    const string to_string() const;
};

// ----------------------------------------------------
enum SaceResponseStatus {
    SACE_RESPONSE_STATUS_EXIT,      // Exit By Self Error
    SACE_RESPONSE_STATUS_SIGNAL,    // Exit By Signal
    SACE_RESPONSE_STATUS_USER,      // User By User
    SACE_RESPONSE_STATUS_UNKNOWN,   // Exit Unknown
};

enum SaceResponseType {
    SACE_RESPONSE_TYPE_NORMAL,
    SACE_RESPONSE_TYPE_SERVICE,
};

class SaceStatusResponse : public SaceResultHeader {
public:
    uint64_t label;             // Corresponding to Service
    string name;                // Service name
    enum SaceResponseType type;
    enum SaceResponseStatus status;
    uint8_t  extra[EXTRA_BUFER_LEN];
    uint32_t extraLen;

    virtual status_t writeToParcel (Parcel *data) const override;
    virtual status_t readFromParcel (const Parcel *data) override;

    static string mapStatusStr (enum SaceResponseStatus status);
    static string mapTypeStr (enum SaceResponseType type);

    SaceStatusResponse ():SaceResultHeader(SACE_BASE_RESULT_TYPE_RESPONSE) {
        label    = 0;
        name     = "";
        type     = SACE_RESPONSE_TYPE_SERVICE;
        status   = SACE_RESPONSE_STATUS_EXIT;
        extraLen = 0;
        memset(extra, 0, EXTRA_BUFER_LEN);
    }

    SaceStatusResponse (const SaceStatusResponse &response):SaceResultHeader(response) {
        label    = response.label;
        name     = response.name;
        type     = response.type;
        status   = response.status;
        extraLen = response.extraLen;
        memcpy(extra, response.extra, extraLen);
    }

    const string to_string() const;
};

}; //namespace android
#endif
