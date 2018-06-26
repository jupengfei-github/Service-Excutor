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

#ifndef _ISACEMANAGER_H
#define _ISACEMANAGER_H

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include "SaceTypes.h"
#include "ISaceListener.h"

namespace android {

// must be kept in sync with interface in ISaceManager.aidl
enum {
    TRANSACT_SENDCOMMAND        = IBinder::FIRST_CALL_TRANSACTION,
    TRANSACT_REGISTERLISTENER   = TRANSACT_SENDCOMMAND + 1,
	TRANSACT_UNREGISTERLISTENER = TRANSACT_REGISTERLISTENER + 1,
};

// --------------------------------------------------------
class ISaceManager : public IInterface {
public:
    DECLARE_META_INTERFACE(SaceManager);

    virtual SaceResult sendCommand (SaceCommand) = 0;
    virtual void registerListener (sp<ISaceListener>) = 0;
	virtual void unregisterListener () = 0;
};

// --------------------------------------------------------
class BnSaceManager : public BnInterface<ISaceManager> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags = 0);
};

// --------------------------------------------------------
class BpSaceManager : public BpInterface<ISaceManager> {
public:
	explicit BpSaceManager (const sp<IBinder> impl):BpInterface<ISaceManager>(impl) {}

	virtual SaceResult sendCommand (SaceCommand);
	virtual void registerListener (sp<ISaceListener>);
	virtual void unregisterListener ();
};

}; //namespace android

#endif
