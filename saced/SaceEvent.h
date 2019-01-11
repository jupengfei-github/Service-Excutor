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

#ifndef _SACE_EVENT_H_
#define _SACE_EVENT_H_

#include <vector>
#include <mutex>
#include <string>

#include <set>
#include "SaceExcutor.h"
#include "SaceWriter.h"
#include "SaceCommandDispatcher.h"

#define DEFAULT_INI_FILE "/system/etc/sace_event.ini"
#define DATA_INI_FILE    "/data/sace/sace_event.ini"

#define SACE_STOP_WRITER  "SaceEventStopWriter"
#define SACE_EVENT_WRITER "SaceEventWriter"

using namespace std;

namespace android {

class SaceEventWriter : public SaceWriter {
    int fd;

public:
    SaceEventWriter (int fd, pid_t pid):SaceWriter(SACE_EVENT_WRITER, pid),fd(fd) {}
    ~SaceEventWriter () {}

    virtual void sendResult (const SaceResult &) override;
    virtual void sendResponse (const SaceStatusResponse &) override;

    void close();
};

class SaceEventStopWriter : public SaceWriter {
public:
    SaceEventStopWriter ():SaceWriter(SACE_EVENT_WRITER, -1) {}
    ~SaceEventStopWriter () {}

    virtual void sendResult (const SaceResult &) {}
    virtual void sendResponse (const SaceStatusResponse &) {}
};

class SaceEvent : public SaceExcutor, public MessageDistributable {
    static const char* EVENT_THREAD_NAME;
    static const char* NAME;
    static const char* THREAD_NAME;

    struct Service {
        sp<EventParams> params;
        sp<SaceCommand> cmd;

        Service (sp<EventParams> params, sp<SaceCommand> cmd):params(params),cmd(cmd) {}
        bool triggered ();
    };

    enum ParseState {
        PARSE_SERVICE,
        PARSE_NONE,
    };

    enum TokenState {
        TOKEN_EMPTY,
        TOKEN_TEXT,
        TOKEN_ERR,
    };

    pthread_t event_monitor;
    atomic_bool running;

    mutex event_mutex;
    /* need mutex protect */
    map<string, shared_ptr<Service>> events;
    map<string, vector<sp<SaceWriter>>> writers;
    map<string, uint64_t> running_events;
    set<string> failed_events;
    set<string> starting_events;

    sp<SaceEventWriter> event_writer;
    int writer_fd;
    sp<SaceReaderMessage> mStopMsg;

    bool read_ini_file ();
    bool write_ini_file () const;

    void parse_event_from_ini (string line);
    TokenState next_token (string& line, string& token);
    void parse_service_attr (string line, sp<SaceCommand> cmd);

    void start_event (shared_ptr<Service>);
    void stop_event (pair<string, uint64_t>, long);
    bool restart_event (string);

    void handle_result (SaceResult &);
    void handle_result (SaceStatusResponse &);

    void add_writers (string name, sp<SaceWriter> wr);

    static void* event_monitor_thread (void *);

public:
    SaceEvent ();
    virtual ~SaceEvent () {}

protected:
    virtual void excuteNormal (sp<SaceMessageHeader>) override;
    virtual bool onInit () override;
    virtual void onUninit () override;
};

}; // namespace android

#endif
