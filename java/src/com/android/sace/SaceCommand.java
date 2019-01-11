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

import java.io.InputStream;
import java.io.OutputStream;

public class SaceCommand {
    private static final String TAG = "SaceCommand";
    private boolean mIn    = false;
    private boolean mClose = false;
    private long mNativePtr;

    private InputStream  mInput;
    private OutputStream mOutput;

    private SaceCommand (long ptr, boolean in) {
        mIn = in;
        mNativePtr = ptr;
    }

    public InputStream getInput () {
        if (mIn == false)
            throw new RuntimeException("Unsupported InputStream");

        if (mInput == null)
            mInput = new SaceInputStream();

        return mInput;
    }

    public OutputStream getOutput () {
        if (mIn == true)
            throw new RuntimeException("Unsupported OutputStream");

        if (mOutput == null)
            mOutput = new SaceOutputStream();

        return mOutput;
    }

    public void close () {
        mClose = true;
        nClose(mNativePtr);
    }

    @Override
    protected void finalize () throws Throwable {
        try {
            nDestroy(mNativePtr);
        }
        finally {
            super.finalize();
        }
    }

    private class SaceInputStream extends InputStream {
		@Override
		public boolean markSupported () {
			return false;
		}

        @Override
        public int read() {
			byte[] bytes = new byte[1];
			nRead(mNativePtr, bytes, 0, 1);
			return bytes[0];
        }

		@Override
		public int read (byte[] b, int off, int len) {
			return nRead(mNativePtr, b, off, len);
		}

		@Override
		public int read (byte[] b) {
			return nRead(mNativePtr, b, 0, b.length);
		}

		@Override
		public void close() {
			nClose(mNativePtr);
		}
    }

    private class SaceOutputStream extends OutputStream {
		@Override
		public void flush () {
            throw new UnsupportedOperationException();
		}

        @Override
        public void write(int b) {
			byte[] bytes = new byte[] {(byte)b};
			nWrite(mNativePtr, bytes, 0, 1);
        }

		@Override
		public void write (byte[] b, int off, int len) {
			nWrite(mNativePtr, b, off, len);
		}

		@Override
		public void close () {
			nClose(mNativePtr);
		}
    }

    private native int nRead (long ptr,  byte[] buf, int off, int len);
    private native int nWrite (long ptr, byte[] buf, int off, int len);
    private native void nClose (long ptr);
    private native void nDestroy (long ptr);
}
