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

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sace/SaceObj.h>

namespace android {

int SaceCommandObj::read (char* buf, int len) throw(UnsupportedOperation, RemoteException) {
    if (!in)
        throw UnsupportedOperation("only read support");

    if (getError() == ERR_EXIT)
        throw RemoteException(cmd + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(cmd + " Has Exit By User");

    return ::read(fd, buf, len);
}

int SaceCommandObj::write (char *buf, int len) throw(UnsupportedOperation, RemoteException) {
    if (in)
        throw UnsupportedOperation("only write support");

    if (getError() == ERR_EXIT)
        throw RemoteException(cmd + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(cmd + " Has Exit By User");

    return ::write(fd, buf, len);
}

void SaceCommandObj::close () {
    if (getError() == ERR_EXIT || getError() == ERR_EXIT_USER) {
        SACE_LOGI("%s Has Closed", cmd.c_str());
        return;
    }

    if (fd >= 0) {
        ::fsync(fd);
        ::close(fd);
    }

    SaceCommand command;
    command.label = label;
    command.type  = SACE_TYPE_NORMAL;
    command.normalCmdType = SACE_NORMAL_CMD_CLOSE;
    SaceResult result = excute(command);

    if (result.resultStatus == SACE_RESULT_STATUS_OK)
        setError(ERR_EXIT_USER);
    else
        SACE_LOGE("close %s fail", cmd.c_str());
}

// -------------------------------------------------
bool SaceServiceObj::stop () throw(RemoteException) {
    if (getError() == ERR_EXIT)
        throw RemoteException(name + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(name + " Has Exit By User");

    if (!label) {
        SACE_LOGE("Service %s not Exists", name.c_str());
        return false;
    }

    mCmd.init();
    mCmd.label = label;
    mCmd.type  = SACE_TYPE_SERVICE;
    mCmd.serviceCmdType = SACE_SERVICE_CMD_STOP;

    mRlt = excute(mCmd);
    return mRlt.resultStatus == SACE_RESULT_STATUS_OK;
}

bool SaceServiceObj::pause () throw(RemoteException) {
    if (getError() == ERR_EXIT)
        throw RemoteException(name + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(name + " Has Exit By User");

    if (!label) {
        SACE_LOGE("Service %s not Exists", name.c_str());
        return false;
    }

    mCmd.init();
    mCmd.label = label;
    mCmd.type  = SACE_TYPE_SERVICE;
    mCmd.serviceCmdType  = SACE_SERVICE_CMD_PAUSE;

    mRlt = excute(mCmd);
    return mRlt.resultStatus == SACE_RESULT_STATUS_OK;
}

bool SaceServiceObj::restart () throw(RemoteException) {
    if (getError() == ERR_EXIT)
        throw RemoteException(name + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(name + " Has Exit By User");

    if (!label) {
        SACE_LOGE("Service %s not Exists", name.c_str());
        return false;
    }

    mCmd.init();
    mCmd.label = label;
    mCmd.type  = SACE_TYPE_SERVICE;
    mCmd.serviceCmdType  = SACE_SERVICE_CMD_RESTART;

    mRlt = excute(mCmd);
    return mRlt.resultStatus == SACE_RESULT_STATUS_OK;
}

enum SaceServiceInfo::ServiceState SaceServiceObj::getState() throw(RemoteException,InvalidOperation) {
    if (getError() == ERR_EXIT)
        throw RemoteException(name + " Exit Abnormally");

    if (getError() == ERR_EXIT_USER)
        throw InvalidOperation(name + " Has Exit By User");

    mCmd.init();
    mCmd.type  = SACE_TYPE_SERVICE;
    mCmd.label = label;
    mCmd.serviceCmdType = SACE_SERVICE_CMD_INFO;
    mCmd.command.assign(SaceServiceInfo::SERVICE_GET_BY_LABEL);

    mRlt = excute(mCmd);
    if (mRlt.resultStatus == SACE_RESULT_STATUS_OK) {
        SaceServiceInfo::ServiceInfo info;
        memcpy(&info, mRlt.resultExtra, mRlt.resultExtraLen);
        return info.state;
    }
    else
        return SaceServiceInfo::SERVICE_DIED_UNKNOWN;
}

}; //namespace android
