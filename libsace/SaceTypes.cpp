/*
 * Copyright (C) 2018-2024 The Service-And-Command Excutor Project
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

#include "SaceTypes.h"

namespace android {
uint16_t SaceCommandHeader::mSequence = 0;

void SaceCommandHeader::reserveSpace (Parcel *data) const {
    data->writeUint32(0);
    data->writeByte(static_cast<int8_t>(type));
    data->writeUint32(sequence);
}

status_t SaceCommandHeader::writeToParcel (Parcel *data) const {
    size_t data_position = data->dataPosition();
    data->setDataPosition(0);

    data->writeUint32(data->dataSize());
    data->setDataPosition(data_position);

    return OK;
}

status_t SaceCommandHeader::readFromParcel (const Parcel *data) {
    len  = data->readUint32();
    type = static_cast<enum SaceCommandType>(data->readByte());
    sequence = data->readUint32();

    return OK;
}

uint32_t SaceCommandHeader::parcelSize () {
    return sizeof(uint32_t)*2 + sizeof(int8_t);
}

string SaceCommandHeader::mapCmdTypeStr (enum SaceCommandType type) {
    switch (type) {
        case SACE_TYPE_NORMAL:
            return "SACE_TYPE_NORMAL";
        case SACE_TYPE_SERVICE:
            return "SACE_TYPE_SERVICE";
        case SACE_TYPE_EVENT:
            return "SACE_TYPE_EVENT";
        default:
            return "UNKNOWN";
    }
}

// SaceCommand ----------------------------------------------------
status_t SaceCommand::writeToParcel (Parcel *data) const {
    reserveSpace(data);

    data->writeUint64(label);
    data->writeUtf8AsUtf16(name);
    data->writeUtf8AsUtf16(command);
    data->writeUint32(extraLen);
    data->write(extra, extraLen);

    if (type == SACE_TYPE_NORMAL) {
        data->writeByte(static_cast<int8_t>(normalCmdType));
        data->writeByte(static_cast<int8_t>(flags));
    }
    else if (type == SACE_TYPE_SERVICE) {
        data->writeByte(static_cast<int8_t>(serviceCmdType));
        data->writeByte(static_cast<int8_t>(serviceFlags));
    }
    else if (type == SACE_TYPE_EVENT) {
        data->writeByte(static_cast<int8_t>(eventType));
        data->writeByte(static_cast<int8_t>(eventFlags));
    }

    /* Note : Must be last */
    if (command_params) {
        data->writeBool(true);
        command_params->writeToParcel(data);
    }
    else
        data->writeBool(false);

    return SaceCommandHeader::writeToParcel(data);
}

status_t SaceCommand::readFromParcel (const Parcel *data) {
    SaceCommandHeader::readFromParcel(data);

    label = data->readUint64();
    data->readUtf8FromUtf16(&name);
    data->readUtf8FromUtf16(&command);
    extraLen = data->readUint32();
    data->read(extra, extraLen);

    if (type == SACE_TYPE_NORMAL) {
        normalCmdType = static_cast<enum SaceNormalCommandType>(data->readByte());
        flags = static_cast<enum SaceCommandFlags>(data->readByte());
    }
    else if (type == SACE_TYPE_SERVICE) {
        serviceCmdType = static_cast<enum SaceServiceCommandType>(data->readByte());
        serviceFlags   = static_cast<enum SaceServiceFlags>(data->readByte());
    }
    else if (type == SACE_TYPE_EVENT) {
        eventType  = static_cast<enum SaceEventType>(data->readByte());
        eventFlags = static_cast<enum SaceEventFlags>(data->readByte());
    }

    /* Note : Must be last */
    if (data->readBool()) {
        command_params = (type == SACE_TYPE_EVENT)? make_shared<SaceEventParams>() : make_shared<SaceCommandParams>();
        command_params->readFromParcel(data);
    }

    return OK;
}

string SaceCommand::mapServiceCmdTypeStr (enum SaceServiceCommandType type) {
    switch (type) {
        case SACE_SERVICE_CMD_INFO:
            return "SACE_SERVICE_CMD_INFO";
        case SACE_SERVICE_CMD_STOP:
            return "SACE_SERVICE_CMD_STOP";
        case SACE_SERVICE_CMD_START:
            return "SACE_SERVICE_CMD_START";
        case SACE_SERVICE_CMD_PAUSE:
            return "SACE_SERVICE_CMD_PAUSE";
        case SACE_SERVICE_CMD_RESTART:
            return "SACE_SERVICE_CMD_RESTART";
        default:
            return "UNKNOWN";
    }
}

string SaceCommand::mapServiceFlagStr (enum SaceServiceFlags flag) {
    switch (flag) {
        case SACE_SERVICE_FLAG_NORMAL:
            return "SACE_SERVICE_FLAG_NORMAL";
        case SACE_SERVICE_FLAG_EVENT:
            return "SACE_SERVICE_FLAG_EVENT";
        default:
            return "UNKNOWN";
    }
}

string SaceCommand::mapNormalCmdTypeStr (enum SaceNormalCommandType type) {
    switch (type) {
        case SACE_NORMAL_CMD_START:
            return "SACE_NORMAL_CMD_START";
        case SACE_NORMAL_CMD_CLOSE:
            return "SACE_NORMAL_CMD_CLOSE";
        default:
            return "UNKNOWN";
    }
}

string SaceCommand::mapNormalCmdFlagStr (enum SaceCommandFlags flag) {
    switch (flag) {
        case SACE_CMD_FLAG_IN:
            return "SACE_CMD_FLAG_IN";
        case SACE_CMD_FLAG_OUT:
            return "SACE_CMD_FLAG_OUT";
        default:
            return "UNKNOWN";
    }
}

string SaceCommand::mapEventFlagStr (enum SaceEventFlags flag) {
    switch (flag) {
        case SACE_EVENT_FLAG_RESTART:
            return "SACE_EVENT_FLAG_RESTART";
        case SACE_EVENT_FLAG_NONE:
            return "SACE_EVENT_FLAG_NONE";
        default:
            return "UNKNOWN";
    }
}

string SaceCommand::mapEventTypeStr (enum SaceEventType type) {
    switch (type) {
        case SACE_EVENT_TYPE_ADD:
            return "SACE_EVENT_TYPE_ADD";
        case SACE_EVENT_TYPE_DEL:
            return "SACE_EVENT_TYPE_DEL";
        case SACE_EVENT_TYPE_INFO:
            return "SACE_EVENT_TYPE_INFO";
        default:
            return "UNKNOWN";
    }
}

const string SaceCommand::to_string() const {
    string cmdDescriptor = string("SaceCommand={");

    cmdDescriptor.append(string("type=") + SaceCommandHeader::mapCmdTypeStr(type))
        .append(" sequecne=" + ::to_string(sequence))
        .append(" name=" + name)
        .append(" command="  + command);

    if (type == SACE_TYPE_NORMAL)
        cmdDescriptor.append(" normalCmdType=" + mapNormalCmdTypeStr(normalCmdType))
            .append(" flags=" + mapNormalCmdFlagStr(flags));
    else if (type == SACE_TYPE_SERVICE) {
        cmdDescriptor.append(" serviceCmdType=" + mapServiceCmdTypeStr(serviceCmdType))
            .append(" flags=" + mapServiceFlagStr(serviceFlags));
    }
    else if (type == SACE_TYPE_EVENT) {
        cmdDescriptor.append(" eventType="  + mapEventTypeStr(eventType));
        cmdDescriptor.append(" eventFlags=" + mapEventFlagStr(eventFlags));
    }

    cmdDescriptor.append("}");
    return cmdDescriptor;
}

// SaceResultHeader --------------------------------------------
uint32_t SaceResultHeader::parcelSize () {
    return sizeof(uint32_t) + sizeof(int8_t);
}

void SaceResultHeader::reserveSpace (Parcel *data) const {
    data->writeUint32(0);
    data->writeByte(static_cast<int8_t>(type));
}

status_t SaceResultHeader::writeToParcel (Parcel *data) const {
    size_t data_position = data->dataPosition();
    data->setDataPosition(0);

    data->writeUint32(data->dataSize());
    data->setDataPosition(data_position);

    return OK;
}

status_t SaceResultHeader::readFromParcel (const Parcel *data) {
    len  = data->readUint32();
    type = static_cast<enum SaceResultHeaderType>(data->readByte());

    return OK;
}

// SaceResult ------------------------------------------------
status_t SaceResult::writeToParcel (Parcel *data) const {
    reserveSpace(data);

    data->writeUint32(sequence);
    data->writeUtf8AsUtf16(name);
    data->writeByte(static_cast<int8_t>(resultType));
    data->writeByte(static_cast<int8_t>(resultStatus));
    data->writeUint32(resultExtraLen);
    data->write(resultExtra, resultExtraLen);

    if (resultType == SACE_RESULT_TYPE_FD)
        data->writeFileDescriptor(resultFd);

    return SaceResultHeader::writeToParcel(data);
}

status_t SaceResult::readFromParcel (const Parcel *data) {
    SaceResultHeader::readFromParcel(data);

    sequence = data->readUint32();
    data->readUtf8FromUtf16(&name);
    resultType   = static_cast<enum SaceResultType>(data->readByte());
    resultStatus = static_cast<enum SaceResultStatus>(data->readByte());
    resultExtraLen = data->readUint32();
    data->read(resultExtra, resultExtraLen);

   if (resultType == SACE_RESULT_TYPE_FD)
        resultFd = data->readFileDescriptor();

    return OK;
}

string SaceResult::mapResultTypeStr (enum SaceResultType type) {
    switch (type) {
        case SACE_RESULT_TYPE_FD:
            return "SACE_RESULT_TYPE_FD";
        case SACE_RESULT_TYPE_NONE:
            return "SACE_RESULT_TYPE_NONE";
        case SACE_RESULT_TYPE_LABEL:
            return "SACE_RESULT_TYPE_LABEL";
        case SACE_RESULT_TYPE_EXTRA:
            return "SACE_RESULT_TYPE_EXTRA";
        default:
            return "UNKNOWN";
    }
}

string SaceResult::mapResultStatusStr (enum SaceResultStatus status) {
    switch (status) {
        case SACE_RESULT_STATUS_OK:
            return "SACE_RESULT_STATUS_OK";
        case SACE_RESULT_STATUS_FAIL:
            return "SACE_RESULT_STATUS_FAIL";
        case SACE_RESULT_STATUS_SECURE:
            return "SACE_RESULT_STATUS_SECURE";
        case SACE_RESULT_STATUS_TIMEOUT:
            return "SACE_RESULT_STATUS_TIMEOUT";
        default:
            return "UNKNOWN";
    }
}

const string SaceResult::to_string() const {
    string resultDescriptor = string("SaceResult={");

    resultDescriptor.append("sequence=" + ::to_string(sequence))
        .append(" name=" + name)
        .append(" resultType="   + mapResultTypeStr(resultType))
        .append(" resultStatus=" + mapResultStatusStr(resultStatus))
        .append(" resultFd=" + ::to_string(resultFd));

    resultDescriptor.append("}");
    return resultDescriptor;
}

// SaceResponse ---------------------------------------------------------
status_t SaceStatusResponse::writeToParcel (Parcel *data) const {
    reserveSpace(data);

    data->writeUint64(label);
    data->writeUtf8AsUtf16(name);
    data->writeByte(static_cast<int8_t>(type));
    data->writeByte(static_cast<int8_t>(status));
    data->writeUint32(extraLen);
    data->write(extra, extraLen);

    return SaceResultHeader::writeToParcel(data);
}

status_t SaceStatusResponse::readFromParcel (const Parcel *data) {
    SaceResultHeader::readFromParcel(data);

    label  = data->readUint64();
    data->readUtf8FromUtf16(&name);
    type   = static_cast<enum SaceResponseType>(data->readByte());
    status = static_cast<enum SaceResponseStatus>(data->readByte());
    extraLen = data->readUint32();
    data->read(extra, extraLen);

    return OK;
}

string SaceStatusResponse::mapStatusStr (enum SaceResponseStatus status) {
    switch (status) {
        case SACE_RESPONSE_STATUS_EXIT:
            return "SACE_RESPONSE_STATUS_EXIT";
        case SACE_RESPONSE_STATUS_SIGNAL:
            return "SACE_RESPONSE_STATUS_SIGNAL";
        case SACE_RESPONSE_STATUS_USER:
            return "SACE_RESPONSE_STATUS_USER";
        case SACE_RESPONSE_STATUS_UNKNOWN:
            return "SACE_RESPONSE_STATUS_UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

string SaceStatusResponse::mapTypeStr (enum SaceResponseType type) {
    switch (type) {
        case SACE_RESPONSE_TYPE_NORMAL:
            return "SACE_RESPONSE_TYPE_NORMAL";
        case SACE_RESPONSE_TYPE_SERVICE:
            return "SACE_RESPONSE_TYPE_SERVICE";
        default:
            return "UNKNOWN";
    }
}

const string SaceStatusResponse::to_string() const {
    string responseDescriptor = string("SaceStatusResponse={");

    responseDescriptor.append("sequence=" + ::to_string(label))
        .append(" name="   + name)
        .append(" type="   + mapTypeStr(type))
        .append(" status=" + mapStatusStr(status))
        .append("}");

    return responseDescriptor;
}

}; //namespace android
