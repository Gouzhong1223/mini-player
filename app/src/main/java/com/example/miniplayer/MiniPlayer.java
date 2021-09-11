package com.example.miniplayer;

import android.os.Message;
import android.util.Log;
import android.view.Surface;

import java.lang.ref.WeakReference;
import java.util.Locale;

public class MiniPlayer {
    //log level
    public int AV_LOG_QUIET = -8;
    public int AV_LOG_PANIC = 0;
    public int AV_LOG_FATAL = 8;
    public int AV_LOG_ERROR = 16;
    public int AV_LOG_WARNING = 24;
    public int AV_LOG_INFO = 32;
    public int AV_LOG_VERBOSE = 40;
    public int AV_LOG_DEBUG = 48;
    public int AV_LOG_TRACE = 56;

    public long mNativeMiniPlayer;
    private static int player_num = 0;

    private static void postEventFromNative(Object weakThiz, int what,
                                            int arg1, int arg2, Object obj) {
        Log.d("IJKMEDIA", " postEventFromNative what:" + what + " arg1:" + arg1 + " arg2:" + arg2);
        if (weakThiz == null)
            return;

        @SuppressWarnings("rawtypes")
        MiniPlayer mp = (MiniPlayer) ((WeakReference) weakThiz).get();
        if (mp == null) {
            return;
        }
    }
    public void start() {
        _start();
    }

    public void setDataSource(String path)
    {
        _setDataSource(path);
        //setLogTag(String.format(Locale.US, "@%02d", player_num++));
    }

    public void setSurface(Surface surface) {
        _setSurface(surface);
    }

    public void release() {
        _release();
    }

    public void init()
    {
        native_setup(new WeakReference<MiniPlayer>(this));
    }

    public void setLogTag(String tag) {
        _setLogTag(tag);
    }

    public static void setLoglevel(int level) {
        _setLogLevel(level);
    }

    public native void _setDataSource(String path);
    public native void _setSurface(Surface surface);
    public native void _start();
    public native void native_setup(Object MiniPlayer_this);
    public native void _setLogTag(String tag);
    //public native void native_setup();
    public native void _release();

    public static native void _setLogLevel(int level);
}
