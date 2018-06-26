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

package com.android.sace;

import java.util.List;

import android.util.Log;

public class SaceManager {
    private static final String TAG = "SaceManager";
    private static final int SACE_PARAMS_VERSION_1_0 = 0x01;

    private static SaceManager mInstance = null;
    static {
        System.loadLibrary("sace_jni");
    }

    private SaceManager () {
        nNativeCreate();
    }

    public static SaceManager getInstance () {
        if (mInstance == null) {
            synchronized (SaceManager.class) {
                if (mInstance == null)
                    mInstance = new SaceManager();
            }
        }

        return mInstance;
    }

    public SaceCommand runCommand (String cmd, boolean inout) throws IllegalArgumentException {
        return nRunCommandExt(cmd, inout, new SaceParameter());
    }

    public SaceCommand runCommand (String cmd, boolean inout, SaceParameter param) throws IllegalArgumentException {
        if (param == null)
            throw new NullPointerException("runCommandm[" + cmd + "] need SaceParameter");
        return nRunCommandExt(cmd, inout, param);
    }

    public boolean runCommand (String cmd) throws IllegalArgumentException {
        return nRunCommand(cmd, new SaceParameter());
    }

    public boolean runCommand (String cmd, SaceParameter param) throws IllegalArgumentException {
        if (param == null)
            throw new NullPointerException("runCommandm[" + cmd + "] need SaceParameter");
        return nRunCommand(cmd, param);
    }

    public SaceService checkService (String name, String cmd) throws IllegalArgumentException {
        return nCheckService(name, cmd, new SaceParameter());
    }

    public SaceService checkService (String name, String cmd, SaceParameter param) throws IllegalArgumentException {
        if (param == null)
            throw new NullPointerException("checkService[" + cmd + "] need SaceParameter");
        return nCheckService(name, cmd, param);
    }

    @Override
    protected void finalize () throws Throwable {
        try {
            Log.d(TAG, "nNativeDestroy");
            nNativeDestroy();
        }
        finally {
            super.finalize();
        }
    }

    /* Native Methods */
    private native void nNativeCreate();
    private native void nNativeDestroy();
    private native SaceCommand nRunCommandExt(String cmd, boolean inout, SaceParameter param);
    private native boolean nRunCommand(String cmd, SaceParameter param);
    private native SaceService nCheckService(String name, String cmd, SaceParameter param);

    public static class SaceParameter {
        public final int version;
        public int uid;
        public List<Integer> gids;

        SaceParameter () {
            version = SACE_PARAMS_VERSION_1_0;
            uid = -1;
        }
    }
}
