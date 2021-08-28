package com.example.miniplayer;

import android.view.Surface;

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

    public native void native_setDataSource(String path);
    public native void native_setSurface(Surface surface);
    public native void native_setLoglevel(int level);
    public native boolean native_start();
    public native void native_release();
}
