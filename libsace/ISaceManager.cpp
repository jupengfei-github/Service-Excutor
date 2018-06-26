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

#include <binder/Parcel.h>
#include <sys/types.h>
#include <stdint.h>

#include "ISaceManager.h"

namespace android {

IMPLEMENT_META_INTERFACE(SaceManager, "android.sace.ISaceManager");

// ---------------------------------------------------------------------------------------------------
status_t BnSaceManager::onTransact (uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags) {
    switch (code) {
        case TRANSACT_SENDCOMMAND: {
            CHECK_INTERFACE(ISaceManager, data, reply);
            SaceCommand command;
            if (data.readInt32() != 0)
                command.readFromParcel((Parcel*)&data);

            SaceResult result = sendCommand(command);
            result.writeToParcel(reply);

            return NO_ERROR;
        }
        case TRANSACT_REGISTERLISTENER: {
            CHECK_INTERFACE(ISaceManager, data, reply);
            sp<ISaceListener> listener =interface_cast<ISaceListener>(data.readStrongBinder());
            registerListener(listener);
            return NO_ERROR;
        }
        case TRANSACT_UNREGISTERLISTENER:
			CHECK_INTERFACE(ISaceManager, data, reply);
			unregisterListener();
			return NO_ERROR;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// --------------------------------------------------------------------------------------------------
SaceResult BpSaceManager::sendCommand (SaceCommand command) {
	Parcel data, reply;

    data.writeInterfaceToken(ISaceManager::getInterfaceDescriptor());
    data.writeInt32(1);
    command.writeToParcel(&data);
    remote()->transact(TRANSACT_SENDCOMMAND, data, &reply, 0);

    SaceResult result;
    result.readFromParcel(&reply);

    return result;
}

void BpSaceManager::registerListener (sp<ISaceListener> listener) {
    Parcel data;

    data.writeInterfaceToken(ISaceManager::getInterfaceDescriptor());
    data.writeStrongBinder(IInterface::asBinder(listener));
    remote()->transact(TRANSACT_REGISTERLISTENER, data, nullptr, 0);
}

void BpSaceManager::unregisterListener () {
	Parcel data;

	data.writeInterfaceToken(ISaceManager::getInterfaceDescriptor());
	remote()->transact(TRANSACT_UNREGISTERLISTENER, data, nullptr, 0);
}

}; //namespace android
