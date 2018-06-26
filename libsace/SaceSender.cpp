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
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <cutils/sockets.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <unistd.h>

#include "SaceSender.h"
#include "SaceLog.h"

namespace android {

// -------------------------------------------------------------------------- {
const char *SaceBinderSender::NAME = "SSBinder";

bool SaceBinderSender::init () {
    sp<IBinder> binder = defaultServiceManager()->getService(String16("SaceService"));

    manager = new BpSaceManager(binder);
    if ((listener = new SaceListenerService(this)) == nullptr) {
        SACE_LOGE("%s alloc SaceListener errno=%d errstr=%s", NAME, errno, strerror(errno));
        return false;
    }

    ProcessState::self()->startThreadPool();
    /* register callback */
    manager->registerListener(listener);
    return true;
}

SaceResult SaceBinderSender::excuteCommand (const SaceCommand &cmd) {
    mRlt.resultType   = SACE_RESULT_TYPE_NONE;
    mRlt.resultStatus = SACE_RESULT_STATUS_FAIL;

    if (manager == nullptr && !init())
        return mRlt;
    else
        return manager->sendCommand(cmd);
} //}

// ---------------------------------------------------------------------------- {
const int SaceSocketSender::SEM_WAIT_TIMEOUT = 3;
const uint64_t SaceSocketSender::RECV_THREAD_EXIT = 0x01;

const char *SaceSocketSender::THREAD_NAME = "SSSocket.MT";
const char *SaceSocketSender::NAME = "SSSocket";

bool SaceSocketSender::init () {
    /* setup socket */
    if ((sockfd = socket_local_client(mSockName.c_str(), ANDROID_SOCKET_NAMESPACE_ABSTRACT, mSockType)) < 0) {
        SACE_LOGE("%s socket_local_client errno=%d errstr=%s", NAME, errno, strerror(errno));
        goto err;
    }

    if ((event_fd = eventfd(0, 0)) < 0) {
        SACE_LOGE("%s eventfd errno=%d errstr=%s", NAME, errno, strerror(errno));
        goto err1;
    }

    if (pthread_mutex_init(&syncMutex, nullptr) < 0) {
        SACE_LOGE("%s pthread_mutex_init errno=%d errstr=%s", NAME, errno, strerror(errno));
        goto err2;
    }

    if (pthread_cond_init(&syncCond, nullptr) < 0) {
        SACE_LOGE("%s pthread_cond_init errno=%d errstr=%s", NAME, errno, strerror(errno));
        goto err3;
    }

    /* setup receive thread */
    if (pthread_create(&recv_thread, nullptr, recv_thread_run, (void*)this) < 0) {
        SACE_LOGE("%s pthread_create errno=%d errstr=%s", NAME, errno, strerror(errno));
        goto err4;
    }

    return (initlized = true);

err4:
    pthread_cond_destroy(&syncCond);
err3:
    pthread_mutex_destroy(&syncMutex);
err2:
    close(event_fd);
err1:
    close(sockfd);
err:
    return false;
}

void SaceSocketSender::uninit () {
    close(sockfd);

    /* exit recv_thread_run */
    uint64_t value = RECV_THREAD_EXIT;
    int ret = write(event_fd, &value, sizeof(value));

    pthread_cond_destroy(&syncCond);
    pthread_mutex_destroy(&syncMutex);

    initlized = false;
}

void* SaceSocketSender::recv_thread_run (void *data) {
    SaceSocketSender *self = (SaceSocketSender*)data;
    fd_set fd_reads;
    char buf[SACE_RESULT_BUF_SIZE];

    prctl(PR_SET_NAME, THREAD_NAME);
    self->recv_thread_id = gettid();
    SACE_LOGI("%s recv_thread_run %d", THREAD_NAME, gettid());
    while (true) {
        FD_ZERO(&fd_reads);
        FD_SET(self->sockfd, &fd_reads);
        FD_SET(self->event_fd, &fd_reads);

        int max_fd = max(self->sockfd, self->event_fd);
        int ret = select(max_fd + 1, &fd_reads, nullptr, nullptr, nullptr);
        if (ret <= 0) {
            SACE_LOGE("%s exit select errno=%d errstr=%s", THREAD_NAME, errno, strerror(errno));
            return 0;
        }

        if (FD_ISSET(self->sockfd, &fd_reads)) {
            struct msghdr msg;
            uint32_t size;

            union {
                struct cmsghdr cm;
                char control[CMSG_SPACE(sizeof(int))];
            } control_un;
            struct cmsghdr *pcmsg = nullptr;

            struct iovec iov[1];
            iov[0].iov_base = buf;
            iov[0].iov_len  = sizeof(buf);

            msg.msg_name    = nullptr;
            msg.msg_namelen = 0;
            msg.msg_iov    = iov;
            msg.msg_iovlen = 1;
            msg.msg_control    = control_un.control;
            msg.msg_controllen = sizeof(control_un.control);

            ret = TEMP_FAILURE_RETRY(recvmsg(self->sockfd, &msg, 0));
            if (ret <= 0) {
                if (ret == 0)
                    SACE_LOGE("%s exit peer close", THREAD_NAME);
                else
                    SACE_LOGE("%s exit recvmsg errno=%d errstr=%s", THREAD_NAME, errno, strerror(errno));
                return 0;
            }

            if ((size_t)ret < SaceResultHeader::parcelSize()) {
                SACE_LOGE("%s smaller Req : %d Real : %d", THREAD_NAME, SaceResultHeader::parcelSize(), ret);
                continue;
            }

            // transform from bytes
            Parcel parcel;
            parcel.setData((uint8_t*)buf, ret);

            SaceResultHeader headerRslt;
            headerRslt.readFromParcel(&parcel);

            // reset
            parcel.setDataPosition(0);

            if (headerRslt.type == SACE_BASE_RESULT_TYPE_NORMAL) {
                if (parcel.dataSize() < headerRslt.len) {
                    SACE_LOGE("%s receive invalid result Req : %d Real : %d", THREAD_NAME, headerRslt.len, (uint32_t)parcel.dataSize());
                    continue;
                }
                else {
                    SaceResult rslt;
                    rslt.readFromParcel(&parcel);

                    pcmsg = CMSG_FIRSTHDR(&msg);
                    if (pcmsg != nullptr && pcmsg->cmsg_len == CMSG_LEN(sizeof(int)) &&
                        pcmsg->cmsg_level == SOL_SOCKET && pcmsg->cmsg_type == SCM_RIGHTS)
                        rslt.resultFd = *((int*)CMSG_DATA(pcmsg));
                    else
                        rslt.resultFd = -1;

                    self->handleResult(rslt);
                }
            }
            else if (headerRslt.type == SACE_BASE_RESULT_TYPE_RESPONSE) {
                if (parcel.dataSize() < headerRslt.len) {
                    SACE_LOGE("%s receive invalid response Req : %d Real : %d", THREAD_NAME, headerRslt.len, (uint32_t)parcel.dataSize());
                    continue;
                }
                else {
                    SaceStatusResponse response;
                    response.readFromParcel(&parcel);
                    self->handleResponse(response);
                }
            }
            else
                SACE_LOGW("%s receive invalid type %d", THREAD_NAME, headerRslt.type);
        }

        if (FD_ISSET(self->event_fd, &fd_reads)) {
            close(self->event_fd);
            SACE_LOGI("%s exit recv_thread_run %s", NAME, THREAD_NAME);
            pthread_exit(0);
        }
    }
}

SaceResult SaceSocketSender::excuteCommand (const SaceCommand &cmd) {
    SaceResult result;
    result.resultType   = SACE_RESULT_TYPE_NONE;
    result.resultStatus = SACE_RESULT_STATUS_FAIL;

    /* excute error for initialize fail */
    if (!initlized && !init())
        return result;

    /* translate to bytes */
    Parcel parcel;
    cmd.writeToParcel(&parcel);

    int ret = TEMP_FAILURE_RETRY(write(sockfd, (char*)parcel.data(), parcel.dataSize()));
    if (ret <= 0) {
        SACE_LOGE("%s excuteCommand errno=%d errstr=%s", NAME, errno, strerror(errno));
        uninit();
        return result;
    }

    if ((size_t)ret != parcel.dataSize()) {
        SACE_LOGE("%s excuteCommand Require %d Real %d", NAME, (uint32_t)parcel.dataSize(), ret);
        return result;
    }

    pthread_mutex_lock(&syncMutex);
    syncSequence = cmd.sequence;

	struct timeval now;
    struct timespec timeout;
    gettimeofday(&now, nullptr);
    timeout.tv_sec  = now.tv_sec + SEM_WAIT_TIMEOUT;
    timeout.tv_nsec = now.tv_usec * 1000;
	ret = pthread_cond_timedwait(&syncCond, &syncMutex, &timeout);

    syncSequence = 0;
    pthread_mutex_unlock(&syncMutex);

    if (ret != 0) {
        if (ret == ETIMEDOUT) {
            SACE_LOGI("%s TIMEOUT RESULT %s", NAME, cmd.to_string().c_str());
            result.resultStatus = SACE_RESULT_STATUS_TIMEOUT;
        }
        else {
            SACE_LOGI("%s WAIT RESULT errno=%d", NAME, ret);
            result.resultStatus = SACE_RESULT_STATUS_FAIL;
        }
        return result;
    }
    else
        return mResult;
}

void SaceSocketSender::handleResponse (const SaceStatusResponse &response) {
    SACE_LOGI("%s handleResponse %s", NAME, response.to_string().c_str());
	onCommandResponse(response);
}

void SaceSocketSender::handleResult (const SaceResult &result) {
    SACE_LOGI("%s handle result %s", NAME, result.to_string().c_str());

    pthread_mutex_lock(&syncMutex);
    if (syncSequence) {
		if (syncSequence != result.sequence) {
			SACE_LOGE("%s May handle older Result %s", NAME, result.to_string().c_str());
			return;
		}
        mResult = result;
        pthread_cond_signal(&syncCond);
    }
    pthread_mutex_unlock(&syncMutex);
} //}

}; //namespace android
