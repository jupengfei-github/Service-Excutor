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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <cutils/sockets.h>

#include "SaceReader.h"
#include "SaceWriter.h"
#include "SaceLog.h"

namespace android {
#define MAX_SOCKET_BUF 1024

enum SaceMessageHandlerType typeCmdToMsg (enum SaceCommandType type) {
    switch (type) {
        case SACE_TYPE_NORMAL:
            return SACE_MESSAGE_HANDLER_NORMAL;
        case SACE_TYPE_SERVICE:
            return SACE_MESSAGE_HANDLER_SERVICE;
        case SACE_TYPE_EVENT:
            return SACE_MESSAGE_HANDLER_EVENT;
        default:
            return SACE_MESSAGE_HANDLER_UNKOWN;
    }
}

bool secured_by_uid_pid (uid_t uid __unused, pid_t pid __unused) {
    return true;
}

SaceResult resultBySecure () {
    SaceResult result;
    result.resultType   = SACE_RESULT_TYPE_NONE;
    result.resultStatus = SACE_RESULT_STATUS_SECURE;
    result.resultExtraLen = 0;
    result.resultFd = -1;
    return result;
}

SaceResult resultByFailure () {
    SaceResult result;
    result.resultType   = SACE_RESULT_TYPE_NONE;
    result.resultStatus = SACE_RESULT_STATUS_FAIL;
    result.resultExtraLen = 0;
    result.resultFd = -1;
    return result;
}

// --------------------------------------------------------------------------------
const char* SaceSocketReader::NAME        = "SRSocket";
const char* SaceSocketReader::THREAD_NAME = "SRSocket.MT";
const char* SaceSocketReader::WRITER_NAME = "SRSocket.SaceWriter";
const int   SaceSocketReader::MONITOR_TIMEOUT = 1; //1s

status_t SaceSocketReader::MonitorThread::readyToRun () {
    SACE_LOGI("%s Starting %d:%d", mReader->getName(), getpid(), gettid());
    return NO_ERROR;
}

bool SaceSocketReader::MonitorThread::threadLoop () {
    bool ret;
    do {
        ret = mReader->recv_data_or_connection();
    } while(!exitPending() || ret);

    return false;
}

void SaceSocketReader::handle_socket_msg (ClientMsg &climsg) {
    sp<SaceSocketWriter> writer = new SaceSocketWriter(NAME, climsg.pid, climsg.fd);

    SACE_LOGI("%s handle command : %s", getName(), climsg.command->to_string().c_str());
    if (secured_by_uid_pid(climsg.uid, climsg.pid)) {
        sp<SaceReaderMessage> saceMsg = new SaceReaderMessage;
        saceMsg->msgHandler = typeCmdToMsg(climsg.command->type);
        saceMsg->msgCmd    = climsg.command;
        saceMsg->msgWriter = writer;
        post(saceMsg);
    }
    else {
        SaceResult rslt = resultBySecure();
        writer->sendResult(rslt);
    }
}

bool SaceSocketReader::recv_data_or_connection () {
    fd_set fds_read;
    struct stat fd_stat;
    int max_sockfd = -1;
    int i, ret;

    FD_ZERO(&fds_read);
    /* original socket monitor connection */
    if (mSockFd >= 0) {
        FD_SET(mSockFd, &fds_read);
        max_sockfd = mSockFd;
    }

    /* connected clients */
    for(i = 0; i < MAX_CLIENT_NUM; i++) {
        if (mClients[i].fd > 0) {
            FD_SET(mClients[i].fd, &fds_read);
            max_sockfd = max(max_sockfd, mClients[i].fd);
        }
    }

    /* no Valide fd */
    if (max_sockfd < 0)
        return false;

    struct timeval timeout = {MONITOR_TIMEOUT, 0};
    ret = select(max_sockfd + 1, &fds_read, nullptr, nullptr, &timeout);
    if (ret == 0)
        return false;
    else if (ret < 0) {
        SACE_LOGE("%s Monitor Clients fail %s", getName(), strerror(errno));
        for (int i = 0; i < MAX_CLIENT_NUM; i++) {
            if (mClients[i].fd > 0 && fstat(mClients[i].fd, &fd_stat) < 0)
                mClients[i].fd = -1;
        }

        if (fstat(mSockFd, &fd_stat) < 0)
            mSockFd = -1;

        return true;
    }
    else if (!ret) {
        SACE_LOGE("%s Monitor Clients Timeout", getName());
        return true;
    }

    if (FD_ISSET(mSockFd, &fds_read)) {
        struct sockaddr addr;
        socklen_t slen = sizeof(addr);

        int client_fd = accept(mSockFd, &addr, &slen);
        if (client_fd < 0) {
            SACE_LOGE("%s Accept Client %d fail %s", getName(), mSockFd, strerror(errno));
            return true;
        }

        for (int i = 0; i < MAX_CLIENT_NUM; i++) {
            if (mClients[i].fd < 0) {
                mClients[i].fd = client_fd;
                break;
            }
        }
    }
    else {
        struct msghdr msg;
        struct iovec iov[1];
        char temp[MAX_SOCKET_BUF] = {0};

        iov[0].iov_base = temp;
        iov[0].iov_len  = MAX_SOCKET_BUF;

        msg.msg_name    = nullptr;
        msg.msg_namelen = 0;
        msg.msg_iov    = iov;
        msg.msg_iovlen = 1;
        msg.msg_control    = nullptr;
        msg.msg_controllen = 0;

        for (i = 0; i < MAX_CLIENT_NUM; i++) {
            int fd = mClients[i].fd;
            if (fd < 0 || !FD_ISSET(fd, &fds_read))
                continue;

            ret = TEMP_FAILURE_RETRY(read(fd, temp, MAX_SOCKET_BUF));
            if (ret <= 0) {
                if (ret < 0)
                    SACE_LOGE("%s Receive Incomming Command fail %d : %s", getName(), fd, strerror(errno));
                else
                    SACE_LOGE("%s Close Socket %d", getName(), fd);

                close(mClients[i].fd);
                mClients[i].fd = -1;
                continue;
            }

            if ((size_t)ret < SaceCommandHeader::parcelSize()) {
                SACE_LOGE("%s - %d Invalide SaceCommandHeader. Size %d, Required %d", getName(), fd, ret, SaceCommandHeader::parcelSize());
                continue;
            }

            Parcel parcel;
            parcel.setData((uint8_t*)temp, ret);

            SaceCommandHeader header;
            header.readFromParcel(&parcel);
            if (parcel.dataSize() < header.len) {
                SACE_LOGE("%s - %d Invalide SaceCommand. Size %d, Required %d", getName(), fd, (uint32_t)parcel.dataSize(), header.len);
                continue;
            }

            struct ucred cred;
            socklen_t len = sizeof(struct ucred);
            getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len);

            ClientMsg climsg;
            climsg.fd = fd;
            climsg.pid = cred.pid;
            climsg.uid = cred.uid;

            parcel.setDataPosition(0);
            climsg.command->readFromParcel(&parcel);

            handle_socket_msg(climsg);
        }
    }

    return true;
}

void SaceSocketReader::close_socket () {
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        if (mClients[i].fd > 0)
            close(mClients[i].fd);
        mClients[i].fd = -1;
    }
}

int SaceSocketReader::setup_socket () {
    int socket_id;

    socket_id = socket_local_server(mSockName.c_str(), ANDROID_SOCKET_NAMESPACE_ABSTRACT, mSockType);
    if (socket_id < 0) {
        SACE_LOGE("%s create socket %s fail %s", getName(), mSockName.c_str(), strerror(errno));
        return 1;
    }

    if (listen(socket_id, MAX_CLIENT_NUM) < 0) {
        SACE_LOGE("%s initialize socket client number fail %s", getName(), strerror(errno));
        return 1;
    }

    mSockFd = socket_id;
    return 0;
}

bool SaceSocketReader::startRead () {
    if (mSockFd < 0 && setup_socket()) {
        SACE_LOGE("%s setup_socket failed", getName());
        return false;
    }

    if (mThread == nullptr)
        mThread = new MonitorThread(this);

    if (mThread == nullptr) {
        close(mSockFd);
        SACE_LOGE("%s initialize MonitorThread failed", getName());
        return false;
    }

    mThread->run(THREAD_NAME);
    return true;
}

void SaceSocketReader::stopRead () {
    close(mSockFd);

    SACE_LOGI("%s Stoping... ", getName());
    for (int i = 0; i < MAX_CLIENT_NUM; i++)
        if (mClients[i].fd > 0) shutdown(mClients[i].fd, SHUT_WR);

    if (mThread != nullptr) {
        if (mThread->isRunning())
            mThread->requestExit();
    }
}

// ------------------------------------------------------------------
const char* SaceBinderReader::NAME = "SRBinder";

SaceResult SaceBinderReader::SaceManagerService::sendCommand (SaceCommand command) {
    if (mExit) {
        SACE_LOGI("%s handle command : %s Ignored For Exited", NAME, command.to_string().c_str());
        return resultByFailure();
    }
    else
        SACE_LOGI("%s handle command : %s", NAME, command.to_string().c_str());

    if (!secured_by_uid_pid(IPCThreadState::self()->getCallingUid(), IPCThreadState::self()->getCallingPid())) {
        SaceResult rslt = resultBySecure();
        return rslt;
    }

    SaceResult *result = new SaceResult();
    map<pid_t, ISaceListener*>::iterator it = mPidListener.find(IPCThreadState::self()->getCallingPid());
    sp<SaceBinderWriter> writer = new SaceBinderWriter(NAME, IPCThreadState::self()->getCallingPid(), it->second, result);

    sp<SaceReaderMessage> saceMsg = new SaceReaderMessage;
    saceMsg->msgHandler = typeCmdToMsg(command.type);
    saceMsg->msgCmd  = new SaceCommand(command);
    saceMsg->msgWriter = writer;

    post(saceMsg);
    writer->waitResult();

    return *result;
}

void SaceBinderReader::SaceManagerService::unregisterListener () {
    if (mExit) {
        SACE_LOGI("SaceManagerService %d:%d unregisterListener Ignored For Exited", IPCThreadState::self()->getCallingUid(),
            IPCThreadState::self()->getCallingPid());
        return;
    }
    else
        SACE_LOGI("SaceManagerService %d:%d unregisterListener", IPCThreadState::self()->getCallingUid(), IPCThreadState::self()->getCallingPid());

    map<pid_t, ISaceListener*>::iterator it = mPidListener.find(IPCThreadState::self()->getCallingPid());
    if (it != mPidListener.end())
        mPidListener.erase(it);
}

void SaceBinderReader::SaceManagerService::registerListener (sp<ISaceListener> listener) {
    if (mExit) {
        SACE_LOGI("SaceManagerService %d:%d registerListener Ignored For Exited", IPCThreadState::self()->getCallingUid(),
            IPCThreadState::self()->getCallingPid());
        return;
    }
    else
        SACE_LOGI("SaceManagerService %d:%d registerListener", IPCThreadState::self()->getCallingUid(), IPCThreadState::self()->getCallingPid());

    if (listener != nullptr)
        mPidListener.insert(pair<pid_t, ISaceListener*>(IPCThreadState::self()->getCallingPid(), listener.get()));
    else
        SACE_LOGW("SaceManagerService NULL FeedBack Pid:%d", IPCThreadState::self()->getCallingPid());
}

bool SaceBinderReader::startRead () {
    SACE_LOGI("%s Starting %d:%d", getName(), getpid(), gettid());
    defaultServiceManager()->addService(String16("SaceService"), mSaceManager);
    ProcessState::self()->startThreadPool();
    return true;
}

void SaceBinderReader::stopRead () {
    SACE_LOGI("%s Stoping... ", getName());
    mSaceManager->stop();
}

}; //namespace android
