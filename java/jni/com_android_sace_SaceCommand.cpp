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

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_android_sace_SaceCommand_nDestroy (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    delete cmd;
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceCommand_nRead (JNIEnv *env, jobject obj __unused, jlong ptr,
		jbyteArray buf, jint off, jint len) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);

    jbyte *bytes = env->GetByteArrayElements(buf, JNI_FALSE);
    jint ret = cmd->read((char*)bytes + off, len);
    env->ReleaseByteArrayElements(buf, bytes, JNI_FALSE);

    return ret;
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceCommand_nWrite (JNIEnv *env, jobject obj __unused, jlong ptr,
		jbyteArray buf, jint off, jint len) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);

    jbyte *bytes = env->GetByteArrayElements(buf, JNI_FALSE);
    jint ret = cmd->write((char*)bytes + off, len);
    env->ReleaseByteArrayElements(buf, bytes, JNI_FALSE);

    return ret;
}

JNIEXPORT void JNICALL Java_com_android_sace_SaceCommand_nClose (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    cmd->close();
}

JNIEXPORT void JNICALL Java_com_android_sace_SaceCommand_nFlush (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    cmd->flush();
}

JNIEXPORT jboolean JNICALL Java_com_android_sace_SaceCommand_nIsOk (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceCommandObj *cmd = reinterpret_cast<SaceCommandObj*>(ptr);
    return cmd->isOk();
}

#ifdef __cplusplus
}
#endif
