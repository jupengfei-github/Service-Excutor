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

#include <cutils/sched_policy.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>

#include "SaceEvent.h"
#include <SaceLog.h>
#include <sace/SaceParams.h>

namespace android {

const char* SaceEvent::EVENT_THREAD_NAME  = "SEEvent.EMT";
const char* SaceEvent::NAME = "SEEvent";
const char* SaceEvent::THREAD_NAME = "SEEvent.MT";

#define CAP_MAP_ENTRY(cap)  { #cap, CAP_##cap }
static const map<string, int> cap_map = {
    CAP_MAP_ENTRY(CHOWN),
    CAP_MAP_ENTRY(DAC_OVERRIDE),
    CAP_MAP_ENTRY(DAC_READ_SEARCH),
    CAP_MAP_ENTRY(FOWNER),
    CAP_MAP_ENTRY(FSETID),
    CAP_MAP_ENTRY(KILL),
    CAP_MAP_ENTRY(SETGID),
    CAP_MAP_ENTRY(SETUID),
    CAP_MAP_ENTRY(SETPCAP),
    CAP_MAP_ENTRY(LINUX_IMMUTABLE),
    CAP_MAP_ENTRY(NET_BIND_SERVICE),
    CAP_MAP_ENTRY(NET_BROADCAST),
    CAP_MAP_ENTRY(NET_ADMIN),
    CAP_MAP_ENTRY(NET_RAW),
    CAP_MAP_ENTRY(IPC_LOCK),
    CAP_MAP_ENTRY(IPC_OWNER),
    CAP_MAP_ENTRY(SYS_MODULE),
    CAP_MAP_ENTRY(SYS_RAWIO),
    CAP_MAP_ENTRY(SYS_CHROOT),
    CAP_MAP_ENTRY(SYS_PTRACE),
    CAP_MAP_ENTRY(SYS_PACCT),
    CAP_MAP_ENTRY(SYS_ADMIN),
    CAP_MAP_ENTRY(SYS_BOOT),
    CAP_MAP_ENTRY(SYS_NICE),
    CAP_MAP_ENTRY(SYS_RESOURCE),
    CAP_MAP_ENTRY(SYS_TIME),
    CAP_MAP_ENTRY(SYS_TTY_CONFIG),
    CAP_MAP_ENTRY(MKNOD),
    CAP_MAP_ENTRY(LEASE),
    CAP_MAP_ENTRY(AUDIT_WRITE),
    CAP_MAP_ENTRY(AUDIT_CONTROL),
    CAP_MAP_ENTRY(SETFCAP),
    CAP_MAP_ENTRY(MAC_OVERRIDE),
    CAP_MAP_ENTRY(MAC_ADMIN),
    CAP_MAP_ENTRY(SYSLOG),
    CAP_MAP_ENTRY(WAKE_ALARM),
    CAP_MAP_ENTRY(BLOCK_SUSPEND),
    CAP_MAP_ENTRY(AUDIT_READ),
};

#define RLIMIT_MAP_ENTRY(rlm) { #rlm, RLIMIT_##rlm }
static const map<string, int> rlimit_map = {
    RLIMIT_MAP_ENTRY(AS),
    RLIMIT_MAP_ENTRY(CORE),
    RLIMIT_MAP_ENTRY(CPU),
    RLIMIT_MAP_ENTRY(DATA),
    RLIMIT_MAP_ENTRY(FSIZE),
    RLIMIT_MAP_ENTRY(LOCKS),
    RLIMIT_MAP_ENTRY(MEMLOCK),
    RLIMIT_MAP_ENTRY(MSGQUEUE),
    RLIMIT_MAP_ENTRY(NICE),
    RLIMIT_MAP_ENTRY(NOFILE),
    RLIMIT_MAP_ENTRY(NPROC),
    RLIMIT_MAP_ENTRY(RTPRIO),
    RLIMIT_MAP_ENTRY(SIGPENDING),
    RLIMIT_MAP_ENTRY(STACK),
};

// -------------- SaceEventWriter -------
void SaceEventWriter::sendResult (const SaceResult &result) {
    Parcel parcel;
    result.writeToParcel(&parcel);
    int expire = parcel.dataSize();

    int ret = TEMP_FAILURE_RETRY(write(fd, parcel.data(), expire));
    if (ret == 0)
        SACE_LOGE("%s closeed peer sendResult %s", getName(), result.name.c_str());
    else if (ret < 0)
        SACE_LOGE("%s sendResult %s errno=%s", getName(), result.name.c_str(), strerror(errno));
    else if (ret != expire)
        SACE_LOGE("%s sendResult expire[%d] actual[%d]", getName(), expire, ret);
}

void SaceEventWriter::sendResponse (const SaceStatusResponse &response) {
    Parcel parcel;
    response.writeToParcel(&parcel);
    int expire = parcel.dataSize();

    int ret = TEMP_FAILURE_RETRY(write(fd, parcel.data(), expire));
    if (ret <= 0)
        SACE_LOGE("%s writeResponse %s errno=%s", getName(), response.name.c_str(), strerror(errno));
    else if (ret != expire)
        SACE_LOGE("%s writeResponse expire[%d] actual[%d]", getName(), expire, ret);
}

void SaceEventWriter::close() {
    ::close(fd);
}

// -------------- SaceEvent -------------
bool SaceEvent::Service::triggered () {
    for (auto it = params->triggers.begin(); it != params->triggers.end(); it++)
        if ((*it)->triggered()) return true;

    return false;
}

SaceEvent::SaceEvent ():SaceExcutor(SACE_MESSAGE_HANDLER_EVENT, NAME, THREAD_NAME) {
    sp<SaceCommand> saceCmd = new SaceCommand();
    saceCmd->type = SACE_TYPE_SERVICE;
    saceCmd->serviceCmdType = SACE_SERVICE_CMD_STOP;

    mStopMsg = new SaceReaderMessage();
    mStopMsg->msgHandler = SACE_MESSAGE_HANDLER_SERVICE;
    mStopMsg->msgCmd    = saceCmd;
    mStopMsg->msgWriter = new SaceEventStopWriter();
}

bool SaceEvent::onInit () {
    int pfd[2];
    if (pipe(pfd) < 0) {
        SACE_LOGE("%s open writer pipe errno=%d errstr=%s", getName(), errno, strerror(errno));
        return false;
    }

    writer_fd = pfd[0];
    event_writer = new SaceEventWriter(pfd[1], getpid());

    read_ini_file();

    running.store(true);
    if (pthread_create(&event_monitor, nullptr, event_monitor_thread, (void*)this)) {
        running.store(false);
        SACE_LOGE("%s start event_monitor_thread errno=%d errstr=%s", getName(), errno, strerror(errno));
        return false;
    }

    return true;
}

void SaceEvent::onUninit () {
    running.store(false);
    pthread_join(event_monitor, nullptr);

    close(writer_fd);
    event_writer->close();

    for (auto event : running_events) {
        pair<string, uint64_t> sve(event.first, event.second);
        SACE_LOGI("%s Stop Running Events : %s", getName(), sve.first.c_str());

        stop_event(sve, 0);
        usleep(5 * 1000); // 5ms
    }
}

void SaceEvent::stop_event (pair<string, uint64_t> run_event, long sequence) {
    mStopMsg->msgCmd->label = run_event.second;

    if (sequence)
        mStopMsg->msgCmd->sequence = sequence;

    post(static_cast<sp<SaceMessageHeader>>(mStopMsg));
}

void* SaceEvent::event_monitor_thread (void *obj) {
    fd_set fds;
    char buf[SACE_RESULT_BUF_SIZE];
    struct timeval timeout;
    SaceEvent *self = static_cast<SaceEvent*>(obj);

    prctl(PR_SET_NAME, EVENT_THREAD_NAME);
    set_sched_policy(0, SP_BACKGROUND);
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_BACKGROUND);

    while (self->running.load()) {
        map<string, shared_ptr<Service>> cmds;

        self->event_mutex.lock();
        cmds.insert(self->events.begin(), self->events.end());
        self->event_mutex.unlock();

        /* Start Trigger Service */
        for (auto it = cmds.begin(); it != cmds.end(); it++) {
            if (it->second->triggered())
                self->start_event(it->second);
        }

        /* Restart Failed Event */
        vector<shared_ptr<Service>> restart_service;
        self->event_mutex.lock();
        for (auto eventName : self->failed_events) {
            map<string, shared_ptr<Service>>::iterator it = self->events.find(eventName);
            if (it != self->events.end())
                restart_service.push_back(it->second);
        }
        self->failed_events.clear();
        self->event_mutex.unlock();

        for (shared_ptr<Service> sve : restart_service)
            self->start_event(sve);

        /* Exception Occur */
        FD_ZERO(&fds);
        FD_SET(self->writer_fd, &fds);
        int max_fd = self->writer_fd;

        /* 300ms timeout */
        timeout.tv_sec  = 0;
        timeout.tv_usec = 300 * 1000;

        int ret = select(max_fd + 1, &fds, nullptr, nullptr, &timeout);
        if (ret <= 0) {
            if (ret < 0)
                SACE_LOGE("%s Listen Writer Fail errno=%d errstr=%s", self->getName(), errno, strerror(errno));
            continue;
        }

        ret = TEMP_FAILURE_RETRY(read(self->writer_fd, buf, sizeof(buf)));
        if (ret == 0) {
            SACE_LOGE("%s Writer Peer Close. Exiting...", self->getName());
            break;
        }
        else if (ret <= 0) {
            SACE_LOGE("%s result fail errno=%d errstr=%s", self->getName(), errno, strerror(errno));
            continue;
        }

        Parcel parcel;
        SaceResultHeader headerRslt;
        parcel.setData((uint8_t*)buf, ret);
        headerRslt.readFromParcel(&parcel);

        parcel.setDataPosition(0);
        if (headerRslt.type == SACE_BASE_RESULT_TYPE_NORMAL) {
            SaceResult rslt;
            rslt.readFromParcel(&parcel);
            self->handle_result(rslt);
        }
        else if (headerRslt.type == SACE_BASE_RESULT_TYPE_RESPONSE) {
            SaceStatusResponse response;
            response.readFromParcel(&parcel);
            self->handle_result(response);
        }
        else
            SACE_LOGD("%s unkown Response Type", self->getName());

        usleep(100 * 1000);  //100ms
    }

    return nullptr;
}

bool SaceEvent::restart_event (string eventName) {
    bool restart = true;

    event_mutex.lock();
    map<string, shared_ptr<Service>>::iterator it = events.find(eventName);
    if (it != events.end()) {
        if (it->second->cmd->eventFlags == SACE_EVENT_FLAG_RESTART)
            failed_events.insert(eventName);
        else
            restart = false;
    }
    else
        restart = false;
    event_mutex.unlock();

    return restart;
}

void SaceEvent::handle_result (SaceStatusResponse &response) {
    string eventName = response.name;
    uint64_t label = response.label;

    auto e = running_events.find(eventName);
    if (e == running_events.end() || e->second != label) {
        SACE_LOGW("%s Event[%s] May Stoped", getName(), eventName.c_str());
        goto writer;
    }

    if (response.status == SACE_RESPONSE_STATUS_SIGNAL) {
        if (restart_event(eventName)) {
            SACE_LOGI("Restarting Exit Event[%s]", eventName.c_str());
            return;
        }
        else
            SACE_LOGE("%s Event[%s] Exit Illegally", getName(), eventName.c_str());
    }
    else
        SACE_LOGI("%s Event[%s] Finished", getName(), eventName.c_str());

writer:
    event_mutex.lock();
    running_events.erase(eventName);
    auto wr_it = writers.find(eventName);
    writers.erase(eventName);
    event_mutex.unlock();

    if (wr_it == writers.end())
        return;

    for (auto e : wr_it->second)
        e->sendResponse(response);
}

void SaceEvent::handle_result (SaceResult &result) {
    string eventName = result.name;

    if (starting_events.find(eventName) == starting_events.end())
        goto writer;

    SACE_LOGE("%s handle result %s", getName(), result.to_string().c_str());
    if (result.resultStatus != SACE_RESULT_STATUS_OK) {
        auto e = events.find(eventName);
        if (e != events.end()) {
            start_event(e->second);
            SACE_LOGE("%s Event[%s] start fail. try starting...", getName(), eventName.c_str());
            return;
        }
        else
            SACE_LOGE("%s Event[%s] delete. Ignore start fail", getName(), eventName.c_str());
    }
    else {
        if (result.resultType == SACE_RESULT_TYPE_LABEL) {
            uint64_t label;
            memcpy(&label, result.resultExtra, result.resultExtraLen);

            event_mutex.lock();
            starting_events.erase(eventName);
            running_events.insert(pair<string, uint64_t>(eventName, label));
            event_mutex.unlock();
        }
        else
            SACE_LOGE("%s Event[%s] fail Invalidate ResultType : %s", getName(), eventName.c_str(),
                SaceResult::mapResultTypeStr(result.resultType).c_str());
    }

writer:
    event_mutex.lock();
    auto wr_it = writers.find(eventName);
    event_mutex.unlock();

    if (wr_it == writers.end())
        return;

    for (auto e : wr_it->second)
        e->sendResult(result);
}

static sp<SaceCommand> clone_service_command (sp<SaceCommand> old) {
    sp<SaceCommand> saceCmd = new SaceCommand(*old.get());
    memset(saceCmd->extra, 0, sizeof(saceCmd->extra));
    saceCmd->extraLen = 0;

    /* Start Service */
    saceCmd->type = SACE_TYPE_SERVICE;
    saceCmd->serviceCmdType = SACE_SERVICE_CMD_START;
    saceCmd->serviceFlags   = SACE_SERVICE_FLAG_EVENT;

    return saceCmd;
}

void SaceEvent::start_event (shared_ptr<Service> service) {
    sp<SaceCommand> saceCmd = service->cmd;
    sp<SaceReaderMessage> cmdMsg = new SaceReaderMessage();

    cmdMsg->msgHandler = SACE_MESSAGE_HANDLER_SERVICE;
    cmdMsg->msgCmd  = clone_service_command(service->cmd);
    cmdMsg->msgWriter = event_writer;

    event_mutex.lock();
    starting_events.insert(saceCmd->name);
    event_mutex.unlock();

    SACE_LOGI("%s Starting Event : %s", getName(), cmdMsg->msgCmd->to_string().c_str());
    post(static_cast<sp<SaceMessageHeader>>(cmdMsg));
}

void SaceEvent::excuteNormal (sp<SaceMessageHeader> msg) {
    sp<SaceReaderMessage> saceMsg = (SaceReaderMessage*)msg.get();
    sp<SaceWriter>  writer  = saceMsg->msgWriter;
    sp<SaceCommand> saceCmd = saceMsg->msgCmd;

    SaceResult result;
    result.sequence = saceCmd->sequence;
    result.name = saceCmd->name;
    result.resultStatus = SACE_RESULT_STATUS_FAIL;
    result.resultType   = SACE_RESULT_TYPE_NONE;

    string eventName = saceCmd->name;
    if (saceCmd->eventType == SACE_EVENT_TYPE_DEL) {
        bool ok = false, stop = false;
        memcpy(&stop, saceCmd->extra, saceCmd->extraLen);

        auto e = events.find(eventName);
        if (e != events.end()) {
            event_mutex.lock();
            events.erase(e);
            event_mutex.unlock();
            result.resultStatus = SACE_RESULT_STATUS_OK;
        }
        else {
            SACE_LOGE("%s delete Event %s not found", getName(), eventName.c_str());
            result.resultStatus = SACE_RESULT_STATUS_FAIL;
            goto err;
        }

        if (stop) {
            event_mutex.lock();
            auto e = running_events.find(eventName);
            if (e != running_events.end()) {
                pair<string, uint64_t> sve(e->first, e->second);
                stop_event(sve, saceCmd->sequence);
                add_writers(eventName, writer);
            }
            event_mutex.unlock();
        }
    }
    else if (saceCmd->eventType == SACE_EVENT_TYPE_ADD) {
        if (events.find(eventName) != events.end()) {
            SACE_LOGE("%s repeated Event %s", getName(), eventName.c_str());
            result.resultStatus = SACE_RESULT_STATUS_EXISTS;
            goto err;
        }

        sp<EventParams> param;
        if (saceCmd->command_params)
            param = static_pointer_cast<SaceEventParams>(saceCmd->command_params)->parseEventParams();

        if (!param.get() || param->triggers.size() <= 0) {
            SACE_LOGW("%s %s don't have more triggers, ignore...", getName(), saceMsg->msgCmd->name.c_str());
            goto err;
        }

        shared_ptr<Service> service = make_shared<Service>(param, saceMsg->msgCmd);
        if (service->triggered())
             start_event(service);

        event_mutex.lock();
        events.insert(pair<string, shared_ptr<Service>>(saceCmd->name, service));
        event_mutex.unlock();

        result.resultStatus = SACE_RESULT_STATUS_OK;
    }
    else if (saceCmd->eventType == SACE_EVENT_TYPE_INFO) {
        bool ok = true;

        if (saceCmd->command != SaceServiceInfo::SERVICE_GET_BY_NAME) {
            SACE_LOGE("%s Only Support SERVICE_GET_BY_NAME %s", getName(), saceCmd->to_string().c_str());
            goto err;
        }

        event_mutex.lock();
        if (running_events.find(saceCmd->name) == running_events.end())
            ok = false;
        event_mutex.unlock();
        if (!ok)
            goto err;

        sp<SaceReaderMessage> cmdMsg = new SaceReaderMessage();
        cmdMsg->msgHandler = SACE_MESSAGE_HANDLER_SERVICE;
        cmdMsg->msgCmd  = clone_service_command(saceCmd);
        cmdMsg->msgCmd->serviceCmdType = SACE_SERVICE_CMD_INFO;
        cmdMsg->msgCmd->serviceFlags = SACE_SERVICE_FLAG_EVENT;
        cmdMsg->msgWriter = event_writer;

        event_mutex.lock();
        add_writers(saceCmd->name, writer);
        event_mutex.unlock();

        post(static_cast<sp<SaceMessageHeader>>(cmdMsg));
        return;
    }

    write_ini_file();
err:
    writer->sendResult(result);
}

void SaceEvent::add_writers (string name, sp<SaceWriter> wr) {
    auto wrs = writers.find(name);

    if (wrs == writers.end()) {
        vector<sp<SaceWriter>> wr_vector;
        wr_vector.push_back(wr);
        writers.insert(pair<string, vector<sp<SaceWriter>>>(name, wr_vector));
        return;
    }
    else {
        for (auto e : wrs->second)
            if (e->operator==(wr)) return;
        wrs->second.push_back(wr);
    }
}

SaceEvent::TokenState SaceEvent::next_token (string& line, string& token) {
    /* trim right space */
    int j = 0;
    for (j = static_cast<int>(line.size()) - 1; j >= 0 && isspace(line[j]); j--);
    line.erase(j, line.size());

    if (line.empty())
        return TOKEN_EMPTY;

    string str;
    for (size_t i = 0; i < line.length(); i++) {
        switch (line[i]) {
        case '\"':
            str.push_back('"');
            while (i < line.length() && line[i] != '\"') str.push_back(line[i++]);
            if (i >= line.length())
                return TOKEN_ERR;

            str.push_back('"');
            break;
        case '\\':
            str.push_back('\\');
            break;
        case '\t':
        case ' ' :
        case '\n':
            goto success;
        default:
            str.push_back(line[i]);
            break;
        }
    }

success:
    line.erase(0, str.size());
    token.assign(str);
    return TOKEN_TEXT;
}

bool SaceEvent::read_ini_file () {
    static string empty_str = "";
    ifstream conf_in(DATA_INI_FILE);

    if (!conf_in.is_open()) {
        SACE_LOGE("%s open %s errno=%d errstr=%s", getName(), DATA_INI_FILE, errno, strerror(errno));
        conf_in.open(DEFAULT_INI_FILE);
        if (!conf_in.is_open())
            SACE_LOGE("%s open %s errno=%d errstr=%s", getName(), DEFAULT_INI_FILE, errno, strerror(errno));
    }

    if (!conf_in.is_open()) {
        SACE_LOGE("Can't Acess Any Config File");
        return false;
    }

    char buf[128] = {0};
    string line;
    while (conf_in.good()) {
        conf_in.getline(buf, sizeof(buf));
        line.assign(buf, conf_in.gcount());

        /* file over */
        if (conf_in.eof()) {
            if (line.size() > 0)
                parse_event_from_ini(line);
            break;
        }

        parse_event_from_ini(line);
    }

    /* finish finally event */
    parse_event_from_ini(empty_str);

    return true;
}

static string trim_space_char (string line) {
	size_t i = 0, j = 0;

	/* skip left space */
    for (i = 0; i < line.size() && isspace(line[i]); i++);
    line.erase(0, i);

	// skip right space
    size_t len = line.size();
	for (i = len > 0? len -1 : 0, j = len; i > 0 && isspace(line[i]); i--, j--);
	line.erase(j);

    return line;
}

void SaceEvent::parse_event_from_ini (string line_token) {
    static sp<SaceCommand> parse_command = nullptr;
    static enum ParseState parse_state = PARSE_NONE;

    string line = trim_space_char(line_token);

    /* over last event. getline add '\0' */
    if (line.empty() || (line.size() == 1 && line[0] == '\0')) {
        if (parse_state == PARSE_SERVICE) {
            sp<EventParams> parse_event_param = static_pointer_cast<SaceEventParams>(parse_command->command_params)->parseEventParams();
            shared_ptr<Service> parse_service = make_shared<Service>(parse_event_param, parse_command);
            events.insert(pair<string, shared_ptr<Service>>(parse_command->name, parse_service));
            parse_command = nullptr;
        }

        parse_state = PARSE_NONE;
        return;
    }

    /* skip commit line */
    if (line[0] == '#')
        return;

    string service_name, service_cmd;
    switch (parse_state) {
    case PARSE_NONE:
        if (next_token(line, service_name) == TOKEN_TEXT && line.size() > 0) {
            parse_state = PARSE_SERVICE;
            parse_command = new SaceCommand();
            parse_command->command_params = make_shared<SaceEventParams>();
            parse_command->name = service_name;
            parse_command->command = line;
        }
        else
            SACE_LOGE("%s parse invalid service : %s", getName(), line.c_str());
        break;
    case PARSE_SERVICE:
        parse_service_attr(line, parse_command);
        break;
    default:
        parse_state = PARSE_NONE;
        break;
    }
}

static int parse_capability_name (string capability_name) {
    auto e = cap_map.find(capability_name);
    if (e != cap_map.end())
        return e->second;
    else
        return -1;
}

static int parse_rlimits_name (string rlimit_name) {
    auto e = rlimit_map.find(rlimit_name);
    if (e != rlimit_map.end())
        return e->second;
    else
        return -1;
}

/* Service Ini Format
 * service_name service_cmd
 * user   <uid | user_name>
 * group  <gid | group_name>
 * groups <gid | group_name> ...
 * seclabel secontext
 * capability capability_name ...
 * trigger property:proper_name=property_value
 * trigger boot <true | false>
 * rlimits limit_name hard_limit soft_limit
 */
void SaceEvent::parse_service_attr (string line, sp<SaceCommand> cmd) {
    shared_ptr<SaceEventParams> cmd_params = static_pointer_cast<SaceEventParams>(cmd->command_params);
    stringstream out_stream(line);
    string tag, str_value;
    int int_value;

    out_stream>>tag;
    if (!out_stream.good()) {
        SACE_LOGE("Ignore Invalid Token : %s", line.c_str());
        return;
    }

    if (tag == "user") {
        out_stream>>str_value;
        if (!out_stream.fail())
            cmd_params->set_uid(str_value);
        else
            SACE_LOGE("%s parse service_attr_user fail : %s", getName(), line.c_str());
    }
    else if (tag == "group") {
        out_stream>>str_value;
        if (!out_stream.fail())
            cmd_params->set_gid(str_value);
        else
            SACE_LOGE("%s parse service_attr_group fail : %s", getName(), line.c_str());
    }
    else if (tag == "seclabel") {
        out_stream>>str_value;
        if (!out_stream.fail())
            cmd_params->set_seclabel(str_value);
        else
            SACE_LOGE("%s parse service_attr_group fail : %s", getName(), line.c_str());
    }
    else if (tag == "capability") {
        do {
            out_stream>>str_value;
            if (out_stream.fail())
                break;
            cmd_params->set_capabilities(parse_capability_name(str_value));
        } while(true);
    }
    else if (tag == "groups") {
        do {
            out_stream>>str_value;
            if (out_stream.fail())
                break;
            cmd_params->add_gids(str_value);
        } while(true);
    }
    else if (tag == "trigger") {
        out_stream>>str_value;
        if (out_stream.fail()) {
            SACE_LOGE("Invalide Trigger : %s", line.c_str());
            return;
        }

        if (!strncmp(str_value.data(), "property", strlen("property"))) {
            size_t colon_pos = str_value.find(':');
            if (colon_pos != string::npos) {
                size_t equal_pos = str_value.find('=', colon_pos);
                if (equal_pos != string::npos) {
                    cmd_params->add_property(str_value.substr(colon_pos, equal_pos - colon_pos),
                        str_value.substr(equal_pos));
                }
                else
                    SACE_LOGE("%s Invalide [equal] in %s", getName(), str_value.c_str());
            }
            else
                SACE_LOGE("%s Invalide [colon] in %s", getName(), str_value.c_str());
        }
        else if (!strncmp(str_value.data(), "boot", strlen("boot"))) {
            cmd_params->set_boot(true);
        }
        else
            SACE_LOGE("Invalide Trigger : %s", str_value.c_str());
    }
    else if (tag == "rlimits") {
        string rlm_name;
        int rlm_hard, rlm_soft;

        out_stream>>rlm_name>>rlm_hard>>rlm_soft;
        if (!out_stream.fail())
            cmd_params->add_rlimits(parse_rlimits_name(rlm_name), rlm_hard, rlm_soft);
        else
            SACE_LOGE("Invalide Rlimits Format : %s", line.c_str());
    }
    else {
        SACE_LOGE("Invalide Service Attr : %s", tag.c_str());
        return;
    }
}

bool SaceEvent::write_ini_file () const {
    if (!access(DATA_INI_FILE, F_OK)) {
        /* create new config file */
        int fd = open(DATA_INI_FILE, O_WRONLY, O_CREAT | S_IRUSR | S_IWUSR);
        if (fd < 0) {
            SACE_LOGE("%s create %s fail errno=%d errstr=%s", getName(), DATA_INI_FILE, errno, strerror(errno));
            return false;
        }
        close(fd);
    }

    ofstream conf_out(DATA_INI_FILE);
    if (!conf_out.is_open()) {
        SACE_LOGE("%s open %s fail errno=%d errstr=%s", getName(), DATA_INI_FILE, errno, strerror(errno));
        return false;
    }

    char buf[128] = {0};
    for (auto event : events) {
        /* Service Ini Format
         * service_name service_cmd
         * user   <uid | user_name>
         * group  <gid | group_name>
         * groups <gid | group_name> ...
         * seclabel secontext
         * capability capability_name ...
         * trigger property:proper_name=property_value
         * trigger boot <true | false>
         * rlimits limit_name hard_limit soft_limit
         */

        sp<SaceCommand> cmd = event.second->cmd;
        sp<EventParams> event_param = event.second->params;
        shared_ptr<SaceCommandParams> cmd_param = cmd->command_params;

        int last_index = conf_out.tellp();
        string service_str;

        // Service Name
        sprintf(buf, "%s %s\n", cmd->name.c_str(), cmd->command.c_str());
        service_str.append(buf, strlen(buf));

        // User/Group
        sprintf(buf, "  user %s\n  group %s\n  seclabel %s\n",
            cmd_param->uid.c_str(), cmd_param->gid.c_str(), cmd_param->seclabel.c_str());
        service_str.append(buf, strlen(buf));

        //capability
        if (cmd_param->capabilities.to_ulong() > 0)
            service_str.append("  capability ").append(::to_string(cmd_param->capabilities.to_ulong()));

        // Groups
        if (cmd_param->supp_gids.size() > 0) {
            service_str.append("  groups ");
            for (auto gp : cmd_param->supp_gids)
                service_str.append(gp).append(" ");
            service_str.append("\n");
        }

        // Rlimits
        for (auto rlms : cmd_param->rlimits) {
            service_str.append("  rlimits ").append(::to_string(rlms[0])).append(" ")
                .append(::to_string(rlms[1])).append(" ")
                .append(::to_string(rlms[2])).append(" ");
        }

        // Triggers
        for (auto tg : event_param->triggers)
            service_str.append("  trigger ").append(tg->to_string()).append("\n");

        conf_out<<service_str;
        if (!conf_out.good()) {
            SACE_LOGE("%s write Event[%s %s] fail", getName(), cmd->name.c_str(), cmd->command.c_str());
            conf_out.seekp(last_index, ios_base::beg);
        }
        else
			conf_out<<endl;
    }

    return true;
}
}; // namespace
