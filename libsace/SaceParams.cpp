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

#include <stdlib.h>
#include <pwd.h>
#include <cutils/properties.h>

#include "sace/SaceParams.h"
#include "SaceLog.h"

namespace android {

// -------------- Trigger -----------------
bool PropertyTrigger::triggered () {
    char value[PROPERTY_VALUE_MAX];

    if (property_get(property_name.c_str(), value, "") <= 0)
        return false;

    if (!strcmp(property_value.c_str(), value))
        return last_value == value? false : (last_value = value, true);
    else
        return false;
}

string PropertyTrigger::to_string () const {
    string property("property:");
    property.append(property_name).append("=").append(property_value);

    return property;
}

bool BootTrigger::triggered () {
    return has_triggered? false : (has_triggered = true);
}

string BootTrigger::to_string () const  {
    return string("boot");
}

// ------------ SaceCommandParams ------------------
status_t SaceCommandParams::writeToParcel (Parcel* parcel) const {
    status_t status = OK;

    status |= parcel->writeUtf8AsUtf16(uid);
    status |= parcel->writeUtf8AsUtf16(gid);
    status |= parcel->writeUtf8VectorAsUtf16Vector(supp_gids);
    status |= parcel->writeUtf8AsUtf16(seclabel);
    status |= parcel->writeUint64(capabilities.to_ulong());
    parcel->writeUtf8VectorAsUtf16Vector(rlimits);

    return status;
}

status_t SaceCommandParams::readFromParcel (const Parcel* parcel) {
    status_t status = OK;

    status |= parcel->readUtf8FromUtf16(&uid);
    status |= parcel->readUtf8FromUtf16(&gid);
    status |= parcel->readUtf8VectorFromUtf16Vector(&supp_gids);
    status |= parcel->readUtf8FromUtf16(&seclabel);
    capabilities = CapSet(parcel->readUint64());
    status |= parcel->readUtf8VectorFromUtf16Vector(&rlimits);

    return status;
}

sp<CommandParams> SaceCommandParams::parseCommandParams () const {
    sp<CommandParams> cmdParams = new CommandParams();

    cmdParams->seclabel = seclabel;
    cmdParams->capabilities = capabilities;

    if (!decode_uid(uid, &cmdParams->uid))
        cmdParams->uid = DEF_UID;

    if (!decode_uid(gid, &cmdParams->gid))
        cmdParams->gid = DEF_GID;

    gid_t supp_gid;
    for (auto g : supp_gids) {
        if (!decode_uid(g, &supp_gid)) cmdParams->supp_gids.push_back(supp_gid);
    }

    int resource;
    struct rlimit limit;
    for (auto rls : rlimits) {
        stringstream out_stream(rls);
        out_stream>>resource>>limit.rlim_cur>>limit.rlim_max;
        cmdParams->rlimits.push_back(pair<int, rlimit>(resource, limit));
    }

    return cmdParams;
}

bool SaceCommandParams::decode_uid (string uid_str, uid_t* uid) const {
    if (uid_str.empty())
        return false;

    if (isalpha(uid_str[0])) {
        passwd* pwd = getpwnam(uid_str.c_str());
        if (pwd == nullptr) {
            SACE_LOGE("getpwnam %s failed %s", uid_str.c_str(), strerror(errno));
            return false;
        }

        *uid = pwd->pw_uid;
        return true;
    }

    errno = 0;
    *uid = static_cast<uid_t>(strtoul(uid_str.c_str(), nullptr, 10));
    if (errno) {
        SACE_LOGE("%s strtoul failed %s", uid_str.c_str(), strerror(errno));
        return false;
    }

    return true;
}

// ------------ SaceEventParams -------------
status_t SaceEventParams::writeToParcel (Parcel* parcel) const {
    status_t status = OK;

    status |= SaceCommandParams::writeToParcel(parcel);
    status |= parcel->writeBool(boot);
    status |= parcel->writeUtf8VectorAsUtf16Vector(property_key);
    status |= parcel->writeUtf8VectorAsUtf16Vector(property_value);

    return status;
}

status_t SaceEventParams::readFromParcel (const Parcel* parcel) {
    status_t status = OK;

    status |= SaceCommandParams::readFromParcel(parcel);
    boot = parcel->readBool();
    status |= parcel->readUtf8VectorFromUtf16Vector(&property_key);
    status |= parcel->readUtf8VectorFromUtf16Vector(&property_value);

    return status;
}

sp<EventParams> SaceEventParams::parseEventParams () const {
    sp<EventParams> cmdParams = new EventParams();

    int property_size = property_key.size();
    for (int i = 0; i < property_size; i++)
        cmdParams->triggers.push_back(make_shared<PropertyTrigger>(property_key[i], property_value[i]));

    if (boot)
        cmdParams->triggers.push_back(make_shared<BootTrigger>());

    return cmdParams;
}

}; // namespace
