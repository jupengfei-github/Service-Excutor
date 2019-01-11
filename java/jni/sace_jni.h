#ifndef _SACE_JNI_H_
#define _SACE_JNI_H_

#include "sace/SaceObj.h"

namespace android {

struct SaceCommandWrapper {
    sp<SaceCommandObj> cmd;

    SaceCommandWrapper (sp<SaceCommandObj>& cmd) {
        this->cmd = cmd;
    }
};

struct SaceServiceWrapper {
    sp<SaceServiceObj> sve;

    SaceServiceWrapper (sp<SaceServiceObj>& sve) {
        this->sve = sve;
    }
};

}; // namespace

#endif
