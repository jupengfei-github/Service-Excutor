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

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/prctl.h>

#include "SaceLog.h"
#include "SaceCommandMonitor.h"
#include "SaceCommandDispatcher.h"
#include "SaceMessage.h"

using namespace android;
using namespace std;

static shared_ptr<SaceCommandDispatcher> sace_cmd_dispatcher;
static unique_ptr<SaceCommandMonitor>    sace_cmd_monitor;

static int g_event_fd = -1;
static const uint64_t EVENT_EXIT = 0x01;

static void sig_handler (int sig) {
    uint64_t val = EVENT_EXIT;

    SACE_LOGI("SACE Stopping Signal(%d)", sig);
    if (TEMP_FAILURE_RETRY(write(g_event_fd, &val, sizeof(uint64_t))) < 0)
        SACE_LOGE("write EVENT_EXIT SACE Main Thread fail errno=%d errstr=%s", errno, strerror(errno));
}

static void handle_abort_exit () {
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);
}

static void exit_sace () {
    /* stop receiving command */
    sace_cmd_monitor->stopListen();
    /* stop dispatching */
    sace_cmd_dispatcher->stop();

    delete sace_cmd_monitor.release();
}

int main(void) {
    SACE_LOGI("SACE Starting(%d)......", getpid());

    prctl(PR_SET_PDEATHSIG, SIGHUP);
    g_event_fd = eventfd(0, 0);
    if (g_event_fd < 0) {
        SACE_LOGE("SACE Initialize Exit EventFd Failed");
        return -1;
    }

    handle_abort_exit();

    /* dispatch message */
    sace_cmd_dispatcher = SaceCommandDispatcher::getInstance();
    if (!sace_cmd_dispatcher->start()) {
        SACE_LOGE("Start SaceCommandDispatcher Failed. Exiting");
        return -1;
    }

    /* monitor command */
    sace_cmd_monitor = make_unique<SaceCommandMonitor>();
    if (!sace_cmd_monitor->startListen()) {
        sace_cmd_dispatcher->stop();
        SACE_LOGE("Start SaceCommandMonitor Failed. Exiting");
        return -1;
    }

    uint64_t value;
    while (true) {
        int ret = TEMP_FAILURE_RETRY(read(g_event_fd, &value, sizeof(uint64_t)));
        if (ret <= 0) {
            SACE_LOGE("SACE Main Thread EventFd fail errno=%d errstr=%s", errno, strerror(errno));
            continue;
        }

        if (value == EVENT_EXIT)
            break;
    }

    exit_sace();
    exit(0);
}
