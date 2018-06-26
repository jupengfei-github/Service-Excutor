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

JNIEXPORT void JNICALL Java_com_android_sace_SaceService_nDestroy (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
    delete sve;
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceService_nStop (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
    return sve->stop();
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceService_nPause (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
    return sve->pause();
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceService_nRestart (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
    return sve->restart();
}

JNIEXPORT jstring JNICALL Java_com_android_sace_SaceService_nGetName (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
	string name = sve->getName();
	return env->NewStringUTF(name.c_str());
}

JNIEXPORT jstring  JNICALL Java_com_android_sace_SaceService_nGetCmd (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
	string cmd  = sve->getCmd();
    return env->NewStringUTF(cmd.c_str());
}

JNIEXPORT jint JNICALL Java_com_android_sace_SaceService_nGetState (JNIEnv *env __unused, jobject obj __unused, jlong ptr) {
    SaceServiceObj *sve = reinterpret_cast<SaceServiceObj*>(ptr);
    return sve->getState();
}

#ifdef __cplusplus
}
#endif
