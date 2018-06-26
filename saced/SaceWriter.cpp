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


#include <sys/socket.h>
#include <cutils/sockets.h>
#include "SaceWriter.h"

namespace android {

void SaceSocketWriter::sendResult (const SaceResult &result) {
    struct iovec  iov[1];
    struct msghdr msg;
    struct cmsghdr *pcmsg;

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;

    Parcel parcel;
    result.writeToParcel(&parcel);

    iov[0].iov_base = (void*)parcel.data();
    iov[0].iov_len  = parcel.dataSize();

    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov    = iov;
    msg.msg_iovlen = 1;
    msg.msg_control    = nullptr;
    msg.msg_controllen = 0;

    if (result.resultType == SACE_RESULT_TYPE_FD) {
        msg.msg_control    = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        pcmsg = CMSG_FIRSTHDR(&msg);
        pcmsg->cmsg_len   = CMSG_LEN(sizeof(int));
        pcmsg->cmsg_level = SOL_SOCKET;
        pcmsg->cmsg_type  = SCM_RIGHTS;
        *((int*)CMSG_DATA(pcmsg)) = result.resultFd;
    }

    int ret = TEMP_FAILURE_RETRY(sendmsg(sockfd, &msg, MSG_WAITALL));
    if (ret <= 0)
        SACE_LOGE("%s handle result fail %s errno=%d errstr=%s", getName(), result.to_string().c_str(), errno, strerror(errno));
}

void SaceSocketWriter::sendResponse (const SaceStatusResponse &response) {
    struct iovec  iov[1];
    struct msghdr msg;

    SACE_LOGI("%s writeResponse %s", getName(), response.to_string().c_str());
    Parcel parcel;
    response.writeToParcel(&parcel);

    iov[0].iov_base = (void*)parcel.data();
    iov[0].iov_len  = parcel.dataSize();

    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov    = iov;
    msg.msg_iovlen = 1;
    msg.msg_control    = nullptr;
    msg.msg_controllen = 0;

    int ret = TEMP_FAILURE_RETRY(sendmsg(sockfd, &msg, 0));
    if (ret <= 0)
        SACE_LOGE("%s response fail. %s", getName(), strerror(errno));
}

// ----------------------------------------------------------
void SaceBinderWriter::sendResult (const SaceResult &result) {
    *(this->result) = result;
    if (sem_post(&sync_sem) < 0)
        SACE_LOGE("%s sem_post errno%d errstr=%s", getName(), errno, strerror(errno));
}

void SaceBinderWriter::sendResponse (const SaceStatusResponse &response) {
    if (listener != nullptr)
        listener->onResponsed(response);
    else
        SACE_LOGE("%s writeResponse %s", getName(), response.to_string().c_str());
}

void SaceBinderWriter::waitResult () {
    if (TEMP_FAILURE_RETRY(sem_wait(&sync_sem)) < 0)
        SACE_LOGE("%s sem_wait errno=%d errstr=%s", getName(), errno, strerror(errno));
}

}; //namespace android
