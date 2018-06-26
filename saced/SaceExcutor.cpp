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

#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <selinux/android.h>

#include "SaceExcutor.h"
#include "SaceWriter.h"
#include <SaceLog.h>

using namespace std;

namespace android {

template<typename T>
void remove_vecotr_item (vector<T> list, T item) {
    typename vector<T>::iterator it;
    for (it = list.begin(); it != list.end(); it++)
        if (*it == item) break;

    if (it != list.end())
        list.erase(it);
}

void set_proc_capability (CapSet &to_keep) {
    cap_t caps = cap_init();
    auto deleter = [](cap_t* p) { cap_free(*p); };
    unique_ptr<cap_t, decltype(deleter)> ptr_caps(&caps, deleter);

    cap_clear(caps);
    cap_value_t value[1];
    for (size_t cap = 0; cap < to_keep.size(); ++cap) {
        if (to_keep.test(cap)) {
            value[0] = cap;
            if (cap_set_flag(caps, CAP_PERMITTED, sizeof(value)/sizeof(value[0]), value, CAP_SET) != 0 ||
                cap_set_flag(caps, CAP_EFFECTIVE, sizeof(value)/sizeof(value[0]), value, CAP_SET) != 0) {
                SACE_LOGE("cap_set_flag(PERMITED|EFFECTIVE, %d ) failed", CAP_SETPCAP);
            }
        }
    }

    if (cap_set_proc(caps) != 0) {
        SACE_LOGE("cap_set_proc(%ld) failed", to_keep.to_ulong());
    }
}

void handle_child_params (sp<CommandParams> params) {
    if (!params.get())
        return;

    /* drop rlimit */
    for (auto rlt : params->rlimits) {
        setrlimit(rlt.first, &rlt.second);
    }

    /* drop capability */
    if (params->capabilities.size() > 0)
        set_proc_capability(params->capabilities);

    /* drop gid */
    if (params->gid > 0 && !setgid(params->gid))
        SACE_LOGE("setgid[%d] failed", params->gid);

    /* drop gids */
    vector<gid_t>& supp_gids = params->supp_gids;
    if (supp_gids.size() > 0 && !setgroups(supp_gids.size(), &supp_gids[0]))
        SACE_LOGE("setgroups failed");

    /* drop uid */
    if (params->uid > 0 && !setuid(params->uid))
        SACE_LOGE("setuid[%d] failed", params->uid);

    /* drop seclabel */
    if (!params->seclabel.empty()) {
        if (setexeccon((params->seclabel.c_str())) < 0)
            SACE_LOGE("setexeccon(%s) fail %s", params->seclabel.c_str(), strerror(errno));
    }
}

int sace_popen(const char *cmd, const char *xtype, sp<CommandParams> param, pid_t *out_pid) {
    struct pid *cur = nullptr, *old = nullptr;
    int pdes[2], serrno;
    pid_t pid;

    if (cmd == nullptr || xtype == nullptr)
        SACE_LOGE("sace_popen failed cmd=%s, xtype=%s", cmd, xtype);

    xtype = strchr(xtype, 'w')? "w" : "r";
    if (pipe2(pdes, 0) < 0)
        return -1;

    if ((cur = (struct pid*)malloc(sizeof(struct pid))) == nullptr) {
        close(pdes[0]);
        close(pdes[1]);
        errno = ENOMEM;
        return -1;
    }

    pthread_rwlock_rdlock(&pidlist_lock);
    switch (pid = vfork()) {
    case -1:
       serrno = errno;
        pthread_rwlock_unlock(&pidlist_lock);
        free(cur);
        close(pdes[0]);
        close(pdes[1]);
        errno = serrno;
        return -1;
    case 0:
        for (old = pidlist; old; old = old->next)
            close(old->fd);

        if (*xtype == 'r') {
            close(pdes[0]);
            if (pdes[1] != STDOUT_FILENO) {
                dup2(pdes[1], STDOUT_FILENO);
                close(pdes[1]);
            }
        }
        else {
            close(pdes[1]);
            if (pdes[0] != STDIN_FILENO) {
                dup2(pdes[0], STDIN_FILENO);
                close(pdes[0]);
            }
        }

        handle_child_params(param);
        prctl(PR_SET_NAME, cmd);
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        execl(BASH_PATH, "sh", "-c", cmd, NULL);
        _exit(127);
        /* NOTRETACHED */
    }

    if (*xtype == 'r') {
        cur->fd = pdes[0];
        close(pdes[1]);
    }
    else {
        cur->fd = pdes[1];
        close(pdes[0]);
    }

    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;
    pthread_rwlock_unlock(&pidlist_lock);

    *out_pid = pid;
    return cur->fd;
}

int sace_pclose (int fd) {
    struct pid *cur = nullptr, *last = nullptr;
    pid_t pid;
    int pstat;

    if (fd < 0) {
        SACE_LOGE("sace_pclose failed");
        return -1;
    }

    pthread_rwlock_wrlock(&pidlist_lock);
    for (cur = pidlist; cur; last = cur, cur = cur->next) {
        if (cur->fd == fd)
            break;
    }

    if (cur == nullptr) {
        pthread_rwlock_unlock(&pidlist_lock);
        return -1;
    }
    close(fd);

    if (last == nullptr)
        pidlist = cur->next;
    else
        last->next = cur->next;
    pthread_rwlock_unlock(&pidlist_lock);

    do {
        pid = waitpid(cur->pid, &pstat, 0);
    }while (pid == -1 && errno == EINTR);

    free(cur);

    return (pid == -1)? -1 : pstat;
}

// ---------------------------------------------------------- {
const int SaceExcutor::DEFAULT_EXCUTOR_TIMEOUT = -1;

void SaceExcutor::destroy_excute_thread () {
    pthread_mutex_lock(&mSyncMutex);
    mExit = true;
    pthread_mutex_unlock(&mSyncMutex);

    /* wait excute_command_thread exit */
    sem_post(&mSyncSem);
    if (pthread_join(excute_thread, nullptr) < 0) {
        SACE_LOGE("%s pthread_join errno=%d errstr=%s", getName(), errno, strerror(errno));
        pthread_kill(excute_thread, SIGKILL);
    }
}

bool SaceExcutor::init () {
    if (pthread_mutex_init(&mSyncMutex, nullptr) < 0) {
        SACE_LOGE("%s pthread_mutex_init errno=%d errstr=%s", getName(), errno, strerror(errno));
        goto mutex;
    }

    if (sem_init(&mSyncSem, 0, 0) < 0) {
        SACE_LOGE("%s sem_init errno=%d errstr=%s", getName(), errno, strerror(errno));
        goto sem;
    }

    if (pthread_create(&excute_thread, nullptr, excute_command_thread, (void*)this) < 0) {
        SACE_LOGE("%s create excute_command_thread errno=%d, errstr=%s", getName(), errno, strerror(errno));
        goto thread;
    }

    if (!onInit())
        goto init;

    return true;
init:
    destroy_excute_thread();
thread:
    sem_destroy(&mSyncSem);
sem:
    pthread_mutex_destroy(&mSyncMutex);
mutex:
    return false;
}

void SaceExcutor::uninit () {
    onUninit();

    SACE_LOGI("%s Stoping...", getName());
    destroy_excute_thread();
    sem_destroy(&mSyncSem);
    pthread_mutex_destroy(&mSyncMutex);
}

bool SaceExcutor::excute (sp<SaceMessageHeader> msg) {
    if (msg->msgHandler == mMsgType) {
        sendCommandMessage(msg);
        return true;
    }

    return false;
}

void SaceExcutor::sendCommandMessage (sp<SaceMessageHeader> msg) {
    pthread_mutex_lock(&mSyncMutex);
    mCmdQueue.push(msg);
    pthread_mutex_unlock(&mSyncMutex);

    sem_post(&mSyncSem);
}

void SaceExcutor::excuteCommand (sp<SaceMessageHeader> msg) {
    switch (msg->msgType) {
        case SACE_MESSAGE_TYPE_NORMAL:
            excuteNormal(msg);
            break;
        case SACE_MESSAGE_TYPE_EVENT:
            excuteEvent(msg);
            break;
        default:
            excuteOther(msg);
            break;
    }
}

void SaceExcutor::excuteNormal (sp<SaceMessageHeader> msg) {
    SACE_LOGI("%s Ingore excuteNormal %s", getName(), msg->to_string().c_str());
}

void SaceExcutor::excuteEvent (sp<SaceMessageHeader> msg) {
    SACE_LOGI("%s Ignore excuteEvent %s", getName(), msg->to_string().c_str());
}

void SaceExcutor::excuteOther (sp<SaceMessageHeader> msg) {
    SACE_LOGI("%s Ignore excuteOther %s", getName(), msg->to_string().c_str());
}

void *SaceExcutor::excute_command_thread (void *data) {
    bool exit = false;
    struct timespec timeout = {0, 0};

    SaceExcutor *self = (SaceNormalExcutor*)data;
    sp<SaceMessageHeader> saceMsg;

    SACE_LOGI("%s Starting %d:%d", self->getName(), getpid(), gettid());
    prctl(PR_SET_NAME, self->getThreadName());

    long out_time = self->receive_msg_timeout();
    while (true) {
        if (out_time > 0) {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += out_time;

            int ret = sem_timedwait(&self->mSyncSem, &timeout);
            if (ret < 0 && errno == ETIMEDOUT) {
                self->excuteTimeout();
                continue;
            }
        }
        else
            sem_wait(&self->mSyncSem);

        pthread_mutex_lock(&self->mSyncMutex);
        exit = self->mExit;
        if (!self->mCmdQueue.empty()) {
            saceMsg = self->mCmdQueue.front();
            self->mCmdQueue.pop();
        }
        else
            saceMsg = nullptr;
        pthread_mutex_unlock(&self->mSyncMutex);

        if (saceMsg != nullptr)
            self->excuteCommand(saceMsg);

        if (exit)
            pthread_exit(0);
    }
} // }

// --------------------------------------------------------------------------- {
const char *SaceServiceExcutor::THREAD_NAME = "SEService.MT";
const char *SaceServiceExcutor::NAME   = "SEService";
const int  SaceServiceExcutor::TIMEOUT = 1;

void SaceServiceExcutor::ServiceInfo::add_writer (sp<SaceWriter> wr) {
    for (auto w : writer)
        if (w->operator==(wr)) return;

    writer.push_back(wr);
}

void SaceServiceExcutor::ServiceInfo::sendResponse (SaceStatusResponse& response) {
    for (auto wr : writer)
        wr->sendResponse(response);
}

const string SaceServiceExcutor::ServiceInfo::to_string () {
    if (!sveDescriptor.empty())
        return sveDescriptor;

    sveDescriptor.append("ServiceInfo {");
    sveDescriptor.append("name=" + name);
    sveDescriptor.append(" pid=" + ::to_string(pid));
    sveDescriptor.append(" cmdLine=" + cmdLine);
    sveDescriptor.append(" state=" + SaceServiceInfo::mapStateStr(state));
    sveDescriptor.append(" startTime=" + startTime);
    sveDescriptor.append("}");

    return sveDescriptor;
}

SaceServiceExcutor::~SaceServiceExcutor () {
    ServiceInfo *sveInfo = nullptr;
    SaceStatusResponse response;
    response.status = SACE_RESPONSE_STATUS_SIGNAL;
    response.type   = SACE_RESPONSE_TYPE_SERVICE;

    vector<ServiceInfo*>::iterator it;
    for (it = mRunningService.begin(); it != mRunningService.end(); it++) {
        sveInfo = *it;
        kill(sveInfo->pid, SIGKILL);

        response.label = sveInfo->label;
        response.name  = sveInfo->name;

        SACE_LOGI("%s Kill Running Service : %s", getName(), sveInfo->to_string().c_str());
        sveInfo->sendResponse(response);

        delete sveInfo;
    }

    mRunningService.clear();
    mSeqService.clear();
    mNameService.clear();
}

void SaceServiceExcutor::onUninit() {
    vector<ServiceInfo*>::iterator it;
    for (it = mRunningService.begin(); it != mRunningService.end(); it++) {
        ServiceInfo *sveInfo = *it;

        SACE_LOGI("%s Stop Running Service : %s", getName(), sveInfo->to_string().c_str());
        kill(sveInfo->pid, SIGINT);
    }

    monitor_service_status();
}

long SaceServiceExcutor::receive_msg_timeout () {
    return TIMEOUT;
}

void SaceServiceExcutor::excuteTimeout () {
    monitor_service_status();
}

void SaceServiceExcutor::excuteNormal (sp<SaceMessageHeader> msg) {
    if (msg->msgType != SACE_MESSAGE_TYPE_NORMAL)
        return;

    sp<SaceReaderMessage> saceMsg = (SaceReaderMessage*)msg.get();
    sp<SaceWriter> writer = saceMsg->msgWriter;
    map<uint64_t, ServiceInfo*>::iterator it;

    pid_t pid;
    sp<SaceCommand> saceCmd = saceMsg->msgCmd;

    SaceResult result;
    result.sequence = saceCmd->sequence;
    result.name = saceCmd->name;
    result.resultStatus = SACE_RESULT_STATUS_FAIL;
    result.resultType   = SACE_RESULT_TYPE_NONE;

    char *argv[] = {(char*)"sh", (char*)"-c", NULL, NULL};
    ServiceInfo *sveInfo = nullptr;

    char cmd[CMD_BUFFER_LEN] = {0};
    if (saceCmd->serviceCmdType == SACE_SERVICE_CMD_START) {
        string name = saceCmd->name;

        /* exists */
        if (mNameService.find(name) != mNameService.end()) {
            SACE_LOGE("%s Starting repeated Service %s", getName(), name.c_str());
            result.resultStatus = SACE_RESULT_STATUS_EXISTS;
            goto end;
        }

        sveInfo = new ServiceInfo();
        sveInfo->state   = SaceServiceInfo::SERVICE_RUNNING;
        sveInfo->cmdLine = saceCmd->command;
        sveInfo->name = saceCmd->name;
        sveInfo->label = (uint64_t)sveInfo;
        sveInfo->flags = saceCmd->serviceFlags;
        sveInfo->add_writer(writer);

        sp<CommandParams> param;
        if (saceCmd->command_params)
            param = saceCmd->command_params->parseCommandParams();

        memcpy(cmd, saceCmd->command.c_str(), saceCmd->command.size());
        if ((pid = fork()) == 0) {
            handle_child_params(param);
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            prctl(PR_SET_NAME, sveInfo->name.c_str());

            argv[2] = cmd;
            execv(BASH_PATH, argv);
            _exit(errno);
        }
        else if (pid > 0) {
            sveInfo->pid = pid;
            mRunningService.push_back(sveInfo);
            mSeqService.insert(pair<uint64_t, ServiceInfo*>(sveInfo->label, sveInfo));
            mNameService.insert(pair<string, ServiceInfo*>(sveInfo->name, sveInfo));

            time_t lt = time(NULL);
            struct tm *time = localtime(&lt);
            char buf[64] = {0};
            strftime(buf, sizeof(buf), "%Y_%m%d_%H%M%S", time);
            sveInfo->startTime = string(buf);

            /* Result */
            result.resultStatus = SACE_RESULT_STATUS_OK;
            result.resultType   = SACE_RESULT_TYPE_LABEL;
            memcpy(result.resultExtra, &sveInfo->label, sizeof(uint64_t));
            result.resultExtraLen = sizeof(uint64_t);
            SACE_LOGI("Starting Service Name=%s Pid=%d", sveInfo->name.c_str(), sveInfo->pid);
        }
        else if (pid < 0) {
            SACE_LOGE("fork process %s fail %s", sveInfo->name.c_str(), saceMsg->to_string().c_str());
            delete sveInfo;
            goto end;
        }
    }
    else if (saceCmd->serviceCmdType == SACE_SERVICE_CMD_STOP) {
        it = mSeqService.find(saceCmd->label);
        if (it == mSeqService.end()) {
            SACE_LOGE("Invalid SACE_SERVICE_CMD_STOP for %s", saceMsg->to_string().c_str());
            goto end;
        }

        sveInfo = it->second;
        if (sveInfo->state != SaceServiceInfo::SERVICE_RUNNING && sveInfo->state != SaceServiceInfo::SERVICE_PAUSED) {
            result.resultStatus = SACE_RESULT_STATUS_OK;
            SACE_LOGE("%s Has Stoped", saceMsg->to_string().c_str());
        }
        else {
            SACE_LOGI("Stoping Service Name=%s Pid=%d", sveInfo->name.c_str(), sveInfo->pid);
            kill(sveInfo->pid, SIGTERM);

            sveInfo->state = SaceServiceInfo::SERVICE_FINISHING_USER;
            result.resultStatus = SACE_RESULT_STATUS_OK;
        }
    }
    else if (saceCmd->serviceCmdType == SACE_SERVICE_CMD_PAUSE) {
        it = mSeqService.find(saceCmd->label);
        if (it == mSeqService.end()) {
            SACE_LOGE("Invalid SACE_SERVICE_CMD_PAUSE for %s", saceMsg->to_string().c_str());
            goto end;
        }

        sveInfo = it->second;
        if (sveInfo->state == SaceServiceInfo::SERVICE_RUNNING) {
            kill(sveInfo->pid, SIGSTOP);
            sveInfo->state = SaceServiceInfo::SERVICE_PAUSED;
            result.resultStatus = SACE_RESULT_STATUS_OK;
        }
        else {
            SACE_LOGW("service %s maybe stoped", sveInfo->to_string().c_str());
        }
    }
    else if (saceCmd->serviceCmdType == SACE_SERVICE_CMD_RESTART) {
        it = mSeqService.find(saceCmd->label);
        if (it == mSeqService.end()) {
            SACE_LOGE("Invalid SACE_SERVICE_CMD_RESTART for %s", saceMsg->to_string().c_str());
            goto end;
        }

        sveInfo = it->second;
        if (sveInfo->state == SaceServiceInfo::SERVICE_PAUSED) {
            kill(sveInfo->pid, SIGCONT);
            sveInfo->state = SaceServiceInfo::SERVICE_RUNNING;
            result.resultStatus = SACE_RESULT_STATUS_OK;

            mRunningService.push_back(sveInfo);
        }
        else {
            SACE_LOGW("service %s maybe stoped", sveInfo->to_string().c_str());
        }
    }
    else if (saceCmd->serviceCmdType == SACE_SERVICE_CMD_INFO) {
        handleServiceInfo(saceCmd, writer, result);
    }

end:
    writer->sendResult(result);
    monitor_service_status();
}

void SaceServiceExcutor::handleServiceInfo (sp<SaceCommand> saceCmd, sp<SaceWriter> writer, SaceResult &result) {
    string &cmd = saceCmd->command;

    if (cmd == SaceServiceInfo::SERVICE_GET_BY_NAME) {
        map<string, ServiceInfo*>::iterator it = mNameService.find(saceCmd->name);
        if (it == mNameService.end()) {
            SACE_LOGE("%s GET_SERVICE_INFO Unkown Service : %s", getName(), saceCmd->name.c_str());
            return;
        }

        ServiceInfo *sveInfo = it->second;
        if (sveInfo->flags != saceCmd->serviceFlags) {
            SACE_LOGE("Invalide Service[%s] Type [%s:%s]", saceCmd->name.c_str(), SaceCommand::mapServiceFlagStr(saceCmd->serviceFlags).c_str(),
                SaceCommand::mapServiceFlagStr(sveInfo->flags).c_str());
            return;
        }

        SaceServiceInfo::ServiceInfo info;
        memset(&info, 0x00, sizeof(info));
        info.label = sveInfo->label;
        info.state = sveInfo->state;
        strncpy(info.name, sveInfo->name.c_str(), sizeof(info.name) - 1);
        strncpy(info.cmd, sveInfo->cmdLine.c_str(), sizeof(info.cmd) - 1);

        result.resultExtraLen = min(sizeof(result.resultExtra), sizeof(info));
        memcpy(result.resultExtra, &info, result.resultExtraLen);

        result.resultStatus = SACE_RESULT_STATUS_OK;
        result.resultType   = SACE_RESULT_TYPE_EXTRA;

        sveInfo->add_writer(writer);
    }
}

void SaceServiceExcutor::monitor_service_status () {
    vector<vector<SaceServiceExcutor::ServiceInfo*>::iterator> gcIterator;
    SaceServiceExcutor::ServiceInfo *sveInfo = nullptr;
    int status;

    for (vector<SaceServiceExcutor::ServiceInfo*>::iterator it = mRunningService.begin();
        it != mRunningService.end(); it++) {
        sveInfo = *it;

        if (sveInfo->state != SaceServiceInfo::SERVICE_PAUSED && sveInfo->state != SaceServiceInfo::SERVICE_RUNNING
            && sveInfo->state != SaceServiceInfo::SERVICE_STOPED && sveInfo->state != SaceServiceInfo::SERVICE_FINISHING_USER)
            continue;

        int ret;
        if ((ret = waitpid(sveInfo->pid, &status, WNOHANG)) > 0) {
            SaceStatusResponse response;

            response.type = SACE_RESPONSE_TYPE_SERVICE;
            /* extra save exit status */
            response.extraLen = sizeof(int32_t);
            response.label = sveInfo->label;
            response.name  = sveInfo->name;

            if (WIFEXITED(status)) {
                int exit_ret = WEXITSTATUS(status);
                sveInfo->state  = exit_ret == 0? SaceServiceInfo::SERVICE_FINISHED : SaceServiceInfo::SERVICE_DIED;
                response.status = SACE_RESPONSE_STATUS_EXIT;
                memcpy(response.extra, &exit_ret, response.extraLen);
                SACE_LOGE("service [%s:%d] exit -> %d : %s", sveInfo->name.c_str(), sveInfo->pid, exit_ret,
                    exit_ret == 0? "no error" : strerror(exit_ret));
            }
            else if (WIFSIGNALED(status)) {
                int signal_ret = WTERMSIG(status);
                if (sveInfo->state == SaceServiceInfo::SERVICE_FINISHING_USER && signal_ret == SIGTERM) {
                    response.status = SACE_RESPONSE_STATUS_USER;
                    sveInfo->state  = SaceServiceInfo::SERVICE_FINISHED_USER;
                    SACE_LOGI("service %s:%d exit by user", sveInfo->name.c_str(), sveInfo->pid);
                }
                else {
                    response.status = SACE_RESPONSE_STATUS_SIGNAL;
                    sveInfo->state  = SaceServiceInfo::SERVICE_DIED_SIGNAL;
                    memcpy(response.extra, &signal_ret, response.extraLen);
                    SACE_LOGE("service %s:%d exit for signal %d", sveInfo->name.c_str(), sveInfo->pid, signal_ret);
                }
            }
            else {
                sveInfo->state = SaceServiceInfo::SERVICE_DIED_UNKNOWN;
                response.status = SACE_RESPONSE_STATUS_UNKNOWN;
                memcpy(response.extra, &status, response.extraLen);
                SACE_LOGE("service %s:%d eixt status = %d", sveInfo->name.c_str(), sveInfo->pid, status);
            }

            gcIterator.push_back(it);
            sveInfo->sendResponse(response);
        }
        else if (ret < 0) {
            if (errno == ECHILD)
                gcIterator.push_back(it);
            SACE_LOGE("waitpid pid=%d errno=%d errstr=%s", sveInfo->pid, errno, strerror(errno));
        }
    }

    for (size_t i = 0; i < gcIterator.size(); i++) {
        sveInfo = *gcIterator[i];
        mSeqService.erase(sveInfo->label);
        mNameService.erase(sveInfo->name);
        mRunningService.erase(gcIterator[i]);
        delete sveInfo;
    }

    gcIterator.clear();
} // }

// ------------------------------------------------------------------ {
const char* SaceNormalExcutor::NAME = "SENormal";
const char* SaceNormalExcutor::THREAD_NAME = "SENormal.MT";

SaceNormalExcutor::~SaceNormalExcutor () {
    SaceStatusResponse response;
    response.type = SACE_RESPONSE_TYPE_NORMAL;
    response.status = SACE_RESPONSE_STATUS_SIGNAL;

    for (vector<CommandInfo*>::iterator it = mRunningCmd.begin(); it != mRunningCmd.end(); it++) {
        CommandInfo *cmd = *it;
        kill(cmd->pid, SIGKILL);

        response.label = cmd->label;
        response.name  = cmd->cmdLine;
        cmd->writer->sendResponse(response);

        SACE_LOGE("%s Stop Running Command : %s", getName(), cmd->cmdLine.c_str());
        delete cmd;
    }

    mRunningCmd.clear();
}

void SaceNormalExcutor::onUninit() {
    for (vector<CommandInfo*>::iterator it = mRunningCmd.begin(); it != mRunningCmd.end(); it++) {
        CommandInfo *cmd = *it;

        SACE_LOGE("%s Stop Running Command : %s", getName(), cmd->cmdLine.c_str());
        kill(cmd->pid, SIGINT);
    }
}

void SaceNormalExcutor::excuteNormal (sp<SaceMessageHeader> msg) {
    sp<SaceReaderMessage> saceMsg = (SaceReaderMessage*)msg.get();
    sp<SaceCommand> saceCmd = saceMsg->msgCmd;

    if (saceCmd->normalCmdType == SACE_NORMAL_CMD_START)
        startNormalCmd(saceMsg);
    else if (saceCmd->normalCmdType == SACE_NORMAL_CMD_CLOSE)
        closeNormalCmd(saceMsg);
    else
        SACE_LOGE("SaceNormalExcutor unkown Command Type %d", saceCmd->normalCmdType);
}

void SaceNormalExcutor::closeNormalCmd (sp<SaceReaderMessage> saceMsg) {
    sp<SaceCommand> saceCmd = saceMsg->msgCmd;
    sp<SaceWriter> writer = saceMsg->msgWriter;
    map<uint64_t, CommandInfo*>::iterator it = mSeqCmd.find(saceCmd->label);

    SaceResult result;
    result.sequence = saceCmd->sequence;

    if (it == mSeqCmd.end() || it->second->fd < 0) {
        result.resultStatus = SACE_RESULT_STATUS_FAIL;
        result.resultType   = SACE_RESULT_TYPE_NONE;
        SACE_LOGE("Invalid SaceCommand Sequence OR Maybe Finished %s", saceMsg->to_string().c_str());
    }
    else {
        CommandInfo *cmdInfo = it->second;

        sace_pclose(cmdInfo->fd);
        remove_vecotr_item(mRunningCmd, cmdInfo);

        result.resultStatus = SACE_RESULT_STATUS_OK;
        result.resultType   = SACE_RESULT_TYPE_NONE;
    }

    writer->sendResult(result);
}

void SaceNormalExcutor::startNormalCmd (sp<SaceReaderMessage> saceMsg) {
    sp<SaceCommand> saceCmd = saceMsg->msgCmd;
    sp<SaceWriter> writer = saceMsg->msgWriter;

    SaceResult result;
    result.sequence = saceCmd->sequence;

    CommandInfo *cmdInfo = new CommandInfo();
    cmdInfo->fd = -1;
    cmdInfo->label = (uint64_t)cmdInfo;
    cmdInfo->cmdLine = saceCmd->command;
    cmdInfo->writer  = writer;

    sp<CommandParams> param;
    if (saceCmd->command_params)
        param = saceCmd->command_params->parseCommandParams();
    else
        param = nullptr;

    int fd = sace_popen(cmdInfo->cmdLine.c_str(), saceCmd->flags == SACE_CMD_FLAG_OUT? "w" : "r", param, &cmdInfo->pid);
    if (fd < 0) {
        SACE_LOGE("popen %s fail %s", cmdInfo->cmdLine.c_str(), strerror(errno));
        result.resultStatus = SACE_RESULT_STATUS_FAIL;
        result.resultType   = SACE_RESULT_TYPE_NONE;
        writer->sendResult(result);

        delete cmdInfo;
        return;
    }
    else
        result.resultStatus = SACE_RESULT_STATUS_OK;

    cmdInfo->fd = fd;

    mRunningCmd.push_back(cmdInfo);
    mSeqCmd.insert(pair<uint64_t, CommandInfo*>(cmdInfo->label, cmdInfo));

    result.resultType = SACE_RESULT_TYPE_FD;
    result.resultStatus = SACE_RESULT_STATUS_OK;

    result.resultFd   = fd;
    result.resultExtraLen = sizeof(uint64_t);
    memcpy(result.resultExtra, &cmdInfo->label, result.resultExtraLen);

    writer->sendResult(result);
} // }

}; //namespace android

