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

#ifndef _SACE_PARAMS_H_
#define _SACE_PARAMS_H_

#include <sys/types.h>
#include <string>
#include <bitset>
#include <vector>
#include <sys/capability.h>
#include <sys/resource.h>
#include <utils/RefBase.h>
#include <private/android_filesystem_config.h>
#include <sstream>

#include <binder/Parcelable.h>
#include <binder/Parcel.h>

#define DEF_UID AID_SYSTEM
#define DEF_GID AID_SYSTEM

#define DEF_SERVICE_UID  AID_SYSTEM
#define DEF_SERVICE_GID  AID_SYSTEM
#define DEF_SERVICE_SEC  "u:r:system_server:s0"

#define DEF_CMD_UID AID_ROOT
#define DEF_CMD_GID AID_ROOT
#define DEF_CMD_SEC "u:r:su:s0"

using namespace std;
using CapSet = bitset<CAP_LAST_CAP + 1>;

namespace android {

/* Event Triggers */
class Trigger {
public:
    virtual bool triggered () = 0;
    virtual string to_string () const;
    virtual ~Trigger () {}
};

class PropertyTrigger : public Trigger {
    string property_name;
    string property_value;
    string last_value;

public:
    PropertyTrigger (string name, string value) {
        property_name  = name;
        property_value = value;
    }
    virtual ~PropertyTrigger () {}

    virtual bool triggered () override;
    virtual string to_string () const override;
};

class BootTrigger : public Trigger {
    bool has_triggered;

public:
    BootTrigger ():has_triggered(false) {}
    virtual ~BootTrigger () {}

    virtual bool triggered () override;
    virtual string to_string () const override;
};

/* Command Excute Params */
struct CommandParams : public RefBase {
    uid_t uid;
    gid_t gid;
    vector<gid_t> supp_gids;
    vector<pair<int, rlimit>> rlimits;
    string seclabel;
    CapSet capabilities;
};

/* Event Excute Params */
struct EventParams : public RefBase {
    vector<shared_ptr<Trigger>> triggers;
};

/* User friendly Comamnd Params */
class SaceEvent;
class SaceCommandParams : public Parcelable {
    string uid;
    string gid;
    vector<string> supp_gids;
    vector<string> rlimits;
    string  seclabel;
    CapSet capabilities;

    friend class SaceEvent;
private:
    bool decode_uid (string uid_str, uid_t* uid) const;

public:
    virtual ~SaceCommandParams () {}

    void set_uid (uid_t uid)  { set_uid(::to_string(uid)); }
    void set_uid (string uid) { this->uid = uid; }

    void set_gid (gid_t gid)  { set_gid(::to_string(gid)); }
    void set_gid (string gid) { this->gid = gid; }

    void add_gids (gid_t gid)  { add_gids(::to_string(gid)); }
    void add_gids (string gid) { supp_gids.push_back(gid);   }

    void set_seclabel (string seclabel) { this->seclabel = seclabel; }

    bool set_capabilities (unsigned int cap) {
        if (cap >= capabilities.size())
            return false;

        capabilities[cap] = true;
        return true;
    }

    void add_rlimits (int rlimit, int soft, int hard) {
        stringstream in_stream;
        in_stream<<rlimit<<soft<<hard;
        rlimits.push_back(in_stream.str());
    }

    sp<CommandParams> parseCommandParams () const;

    virtual status_t writeToParcel (Parcel* parcel) const override;
    virtual status_t readFromParcel (const Parcel* parcel) override;
};

/* User friendly Event Params */
class SaceManager;
class SaceEventParams : public SaceCommandParams {
    vector<string> property_key;
    vector<string> property_value;
    bool boot;

    friend class SaceManager;
    friend class SaceEvent;
public:
    SaceEventParams () {
        boot = true;
    }

    virtual ~SaceEventParams () {}

    void set_boot (bool boot)  { this->boot = boot; }
    void add_property (string name, string value) {
        property_key.push_back(name);
        property_value.push_back(value);
    }

    sp<EventParams> parseEventParams () const;

    virtual status_t writeToParcel (Parcel* parcel) const override;
    virtual status_t readFromParcel (const Parcel* parcel) override;
};

}; // namespace

#endif
