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

#include <jni.h>
#include "sace/SaceObj.h"
#include "sace/SaceError.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

class RecycleByteArray {
    jbyte* byte_ptr;
    jbyteArray& array_ptr;
    JNIEnv* env_ptr;

public:
    RecycleByteArray (JNIEnv* env, jbyteArray& array):array_ptr(array),env_ptr(env) {}
    ~RecycleByteArray () {
        env_ptr->ReleaseByteArrayElements(array_ptr, byte_ptr, JNI_FALSE);
    }

    jbyte* obtain () {
        byte_ptr = env_ptr->GetByteArrayElements(array_ptr, JNI_FALSE);
        return byte_ptr;
    }
};

JNIEXPORT void JNICALL Java_com_android_sace_SaceCommand_nDestroy (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    delete cmd;
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceCommand_nRead (JNIEnv *env, jobject obj __unused, jlong ptr,
		jbyteArray buf, jint off, jint len) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    jint ret = -1;

    try {
        RecycleByteArray recycle_array(env, buf);
        jbyte *bytes = recycle_array.obtain();

        ret = cmd->read((char*)bytes + off, len);
    }
    catch (UnsupportedOperation& e) {
        env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"), e.what());
    }
    catch (RemoteException& e) {
        env->ThrowNew(env->FindClass("android/os/RemoteException"), e.what());
    }
    catch (InvalidOperation& e) {
        env->ThrowNew(env->FindClass("android/util/AndroidRuntimeException"), e.what());
    }

    return ret;
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceCommand_nWrite (JNIEnv *env, jobject obj __unused, jlong ptr,
		jbyteArray buf, jint off, jint len) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    jint ret = -1;

    try {
        RecycleByteArray recycle_array(env, buf);
        jbyte *bytes = recycle_array.obtain();

        ret = cmd->write((char*)bytes + off, len);
    }
    catch (UnsupportedOperation& e) {
        env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"), e.what());
    }
    catch (RemoteException& e) {
        env->ThrowNew(env->FindClass("android/os/RemoteException"), e.what());
    }
    catch (InvalidOperation& e) {
        env->ThrowNew(env->FindClass("android/util/AndroidRuntimeException"), e.what());
    }

    return ret;
}

JNIEXPORT void JNICALL Java_com_android_sace_SaceCommand_nClose (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    cmd->close();
}

#ifdef __cplusplus
}
#endif
