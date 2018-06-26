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

#include "SaceCommandDispatcher.h"
#include "SaceLog.h"
#include "SaceWriter.h"
#include "SaceEvent.h"

namespace android {

void MessageDistributable::post (sp<SaceMessageHeader> msg) {
    sp<MessageHandlerWrapper> wrapper = new MessageHandlerWrapper(this, msg);
    mLooper->sendMessage(wrapper, Message(MessageHandlerWrapper::MESSAGE_HANDLER_ID));
}

const int MessageDistributable::MessageHandlerWrapper::MESSAGE_HANDLER_ID = 0x01;
void MessageDistributable::MessageHandlerWrapper::handleMessage (const Message &message) {
    if (message.what == MESSAGE_HANDLER_ID)
        self->mDispatcher->handleMessage(msg);
    else
        SACE_LOGW("MessageHandlerWrapper Invalide MessageType %d", message.what);
}

// ----------------------------------------------------------------------------
const int   SaceCommandDispatcher::POLL_WAIT_TIMEOUT = 1000; //1s
const char* SaceCommandDispatcher::NAME = "SaceCommandDispatcher";

shared_ptr<SaceCommandDispatcher> SaceCommandDispatcher::mInstance = make_shared<SaceCommandDispatcher>();

SaceCommandDispatcher::SaceCommandDispatcher () {
    mLooper = new Looper(false);
}

shared_ptr<SaceCommandDispatcher> SaceCommandDispatcher::getInstance () {
    Mutex mutex;
    if (mInstance.get() == nullptr) {
        mutex.lock();
        if (mInstance.get() == nullptr)
            mInstance = make_shared<SaceCommandDispatcher>();
        mutex.unlock();
    }

    return mInstance;
}

void* SaceCommandDispatcher::dispatch_thread (void *data) {
    SaceCommandDispatcher *dispatcher = reinterpret_cast<SaceCommandDispatcher*>(data);
    sp<Looper> looper = dispatcher->mLooper;

    while (true) {
        int ret = looper->pollOnce(SaceCommandDispatcher::POLL_WAIT_TIMEOUT);
        if (ret == Looper::POLL_ERROR) {
            SACE_LOGE("%s pollOnce fail errno=%d errstr=%s", SaceCommandDispatcher::NAME, errno, strerror(errno));
            continue;
        }
        else if (ret == Looper::POLL_WAKE) {
            SACE_LOGE("%s pollOnce awake errno=%d errstr=%s", SaceCommandDispatcher::NAME, errno, strerror(errno));
            continue;
        }
        else if (ret == Looper::POLL_TIMEOUT) {
            if (!dispatcher->mExit)
                continue;

            break;
        }
    }

	return 0;
}

void SaceCommandDispatcher::handleMessage (sp<SaceMessageHeader> msg) {
    bool handle = false;

    for (vector<SaceExcutor*>::iterator it = mExcutor.begin(); it != mExcutor.end(); it++) {
        SaceExcutor *excutor = *it;
        if (excutor->excute(msg))
            handle = true;
    }

    if (!handle)
        handleDefaultMessage(msg);
}

void SaceCommandDispatcher::handleDefaultMessage (sp<SaceMessageHeader> msg) {
    SACE_LOGI("handleDefaultMessage %s", SaceMessageHeader::mapIdToName(msg->msgHandler).c_str());
}

bool SaceCommandDispatcher::start () {
    if (pthread_create(&dispatch_thread_t, nullptr, dispatch_thread, (void*)this) < 0) {
        SACE_LOGE("%s Start dispatch_thread fail errno=%d errstr=%s", NAME, errno, strerror(errno));
        return false;
    }

    /* It's dangerous to change the order. */
    mExcutor.push_back(new SaceServiceExcutor());
    mExcutor.push_back(new SaceNormalExcutor());
    mExcutor.push_back(new SaceEvent());

    for (vector<SaceExcutor*>::iterator it = mExcutor.begin(); it != mExcutor.end(); it++) {
        SaceExcutor *excutor = *it;
        excutor->init();
    }

    return true;
}

void SaceCommandDispatcher::stop () {
    for (vector<SaceExcutor*>::reverse_iterator it = mExcutor.rbegin(); it != mExcutor.rend(); it++) {
        SaceExcutor *excutor = *it;
        excutor->uninit();
    }

    mExit = true;
    pthread_join(dispatch_thread_t, nullptr);

    for (vector<SaceExcutor*>::reverse_iterator it = mExcutor.rbegin(); it != mExcutor.rend(); it++)
        delete *it;
    mExcutor.clear();
}

}; //namespace android
