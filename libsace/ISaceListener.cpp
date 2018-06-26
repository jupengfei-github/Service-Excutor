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

#include <binder/IBinder.h>
#include <binder/Parcel.h>

#include "ISaceListener.h"

namespace android {

class BpSaceListener : public BpInterface<ISaceListener> {
public:
	explicit BpSaceListener (const sp<IBinder>& impl) : BpInterface<ISaceListener>(impl) {}

	void onResponsed (const SaceStatusResponse &response) {
		Parcel data, reply;

		data.writeInt32(1);
		data.writeInterfaceToken(ISaceListener::getInterfaceDescriptor());
		response.writeToParcel(&data);
		remote()->transact(TRANSACTION_ONRESPONSED, data, &reply, 0);
	}
};

IMPLEMENT_META_INTERFACE(SaceListener, "android.sace.SaceListener");

// ---------------------------------------------------------------------
status_t BnSaceListener::onTransact (uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags) {
	switch(code) {
		case TRANSACTION_ONRESPONSED: {
			CHECK_INTERFACE(ISaceListener, data, reply);
			SaceStatusResponse response;
			if (data.readInt32() != 0) {
				response.readFromParcel(&data);
			}

			onResponsed(response);
			return NO_ERROR;
		}
		default:
			return BBinder::onTransact(code, data, reply, flags);
	}
}

}; //namespace android
