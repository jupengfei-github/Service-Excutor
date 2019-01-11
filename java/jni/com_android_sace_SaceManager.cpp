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
#include "sace/SaceManager.h"
#include "sace_jni.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

static sp<SaceManager> gSaceManager = nullptr;

/* must be consist with SaceManager.java */
static int SACE_PARAM_VERSION_1_0 = 0x01;

JNIEXPORT void JNICALL Java_com_android_sace_SaceManager_nNativeCreate (JNIEnv *env __unused, jobject obj __unused) {
    gSaceManager = SaceManager::getInstance();
}

JNIEXPORT void JNICALL Java_com_android_sace_SaceManager_nNativeDestroy (JNIEnv *env __unused ,jobject obj __unused) {
    gSaceManager = nullptr;
}

static void throwIllegalException(JNIEnv* env, string err) {
    jclass exception = env->FindClass("java/lang/IllegalArgumentException");
    env->ThrowNew(exception, err.c_str());
}

static shared_ptr<SaceCommandParams> nativeSaceParams (JNIEnv *env, jobject sace_params) {
    jclass cls = env->GetObjectClass(sace_params);
    shared_ptr<SaceCommandParams> param = make_shared<SaceCommandParams>();

    jint version = env->GetIntField(sace_params, env->GetFieldID(cls, "version", "I"));
    if (version > SACE_PARAM_VERSION_1_0)
        throwIllegalException(env, string("unknown versionCode=").append(::to_string(version)).append(" In SaceParams"));

    jint uid = env->GetIntField(sace_params, env->GetFieldID(cls, "uid", "I"));
    if (uid > 0)
        param->set_uid(static_cast<uid_t>(uid));

    jobject obj = env->GetObjectField(sace_params, env->GetFieldID(cls, "gids", "Ljava/util/List;"));
    if (obj == nullptr)
        return param;

    jclass list_cls = env->GetObjectClass(obj);
    jmethodID list_get  = env->GetMethodID(list_cls, "get", "(I)Ljava/lang/Integer;");
    jmethodID list_size = env->GetMethodID(list_cls, "size", "()I");

    jsize length = env->CallIntMethod(sace_params, list_size);
    for (jint i = 0; i < length; i++) {
        jobject obj_value = env->CallObjectMethod(sace_params, list_get, i);

        jclass obj_cls = env->GetObjectClass(obj_value);
        gid_t  gid = static_cast<gid_t>(env->CallIntMethod(obj_value, env->GetMethodID(obj_cls, "intValue", "(I)V")));

        if (i <= 0)
            param->set_gid(gid);
        else
            param->add_gids(gid);
    }

    return param;
}

JNIEXPORT jobject JNICALL Java_com_android_sace_SaceManager_nRunCommandExt (JNIEnv *env, jobject __unused, jstring cmd, jboolean inout, jobject param) {
    const char* chars = env->GetStringUTFChars(cmd, JNI_FALSE);
    sp<SaceCommandObj> command = gSaceManager->runCommand(chars, nativeSaceParams(env, param), inout);
    env->ReleaseStringUTFChars(cmd, chars);

    jclass jCommand = env->FindClass("com/android/sace/SaceCommand");
    jmethodID jCmdInit = env->GetMethodID(jCommand, "<init>", "(JZ)V");

    SaceCommandWrapper* wrapper = new SaceCommandWrapper(command);
    jobject jCmdObj = env->NewObject(jCommand, jCmdInit, (jlong)wrapper, inout);

    return jCmdObj;
}

JNIEXPORT jboolean JNICALL Java_com_android_sace_SaceManager_nRunCommand (JNIEnv *env, jobject obj __unused, jstring cmd, jobject param) {
    const char* chars = env->GetStringUTFChars(cmd, JNI_FALSE);
    sp<SaceCommandObj> command = gSaceManager->runCommand(chars, nativeSaceParams(env, param));
    env->ReleaseStringUTFChars(cmd, chars);

    if (!command.get())
        return false;
    else
        command->close();

    return true;
}

JNIEXPORT jobject JNICALL Java_com_android_sace_SaceManager_nCheckService (JNIEnv *env, jobject obj __unused, jstring name, jstring cmd, jobject param) {
    const char* chars = env->GetStringUTFChars(name, JNI_FALSE);
    const char* char_cmd = env->GetStringUTFChars(cmd, JNI_FALSE);
    sp<SaceServiceObj> service = gSaceManager->checkService(chars, char_cmd, nativeSaceParams(env, param));
    env->ReleaseStringUTFChars(name, chars);
    env->ReleaseStringUTFChars(cmd, char_cmd);

    jclass jService = env->FindClass("com/android/sace/SaceService");
    jmethodID jServiceInit = env->GetMethodID(jService, "<init>", "(J)V");

    SaceServiceWrapper* wrapper = new SaceServiceWrapper(service);
    jobject jServiceObj = env->NewObject(jService, jServiceInit, (jlong)wrapper);

    return jServiceObj;
}

#ifdef __cplusplus
}
#endif
