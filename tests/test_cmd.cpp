#define LOG_TAG "TEST_CMD"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>

#include <log/log.h>
#include <sace/SaceManager.h>

using namespace android;

void test_command (const char* cm) {
    char buf[1024];
    int len;

    sp<SaceManager> manager = SaceManager::getInstance();

    ALOGI("runCommand %s", cm);
    sp<SaceCommandObj> cmd = manager->runCommand(cm);
    if (!cmd.get()) {
        ALOGE("run Command Failed");
        return;
    }

    do {
        len = cmd->read(buf, 1023);
        if (len <= 0)
            break;

        std::cout<<buf;
    }while(true);

    cmd->close();
}

void test_service (const char *name, const char* cm) {
    sp<SaceManager> manager = SaceManager::getInstance();

    ALOGI("runCommand %s", cm);
    sp<SaceServiceObj> sve = manager->checkService(name, cm);
    if (!sve.get()) {
        ALOGE("run Service Failed");
        return;
    }

    sleep(5);
    sve->pause();
    sleep(5);
    sve->restart();
    sleep(5);
    sve->stop();
}

void test_event (const char* name, const char* cm) {
    sp<SaceManager> manager = SaceManager::getInstance();

    ALOGI("runEvent %s", cm);
    if (!manager->addEvent(name, cm)) {
        ALOGE("run Event Failed");
        return;
    }

    sp<SaceServiceObj> sve;
    do {
        sve = manager->checkService(name);
    } while (!sve.get());

    sleep(5);
    sve->pause();
    sleep(5);
    sve->restart();
    sleep(5);
    sve->stop();
}

int main (void) {
    test_command("ls /sdcard");
    test_service("service", "ping www.baidu.com");
    test_event("event", "ping www.baidu.com");
    return 0;
}
