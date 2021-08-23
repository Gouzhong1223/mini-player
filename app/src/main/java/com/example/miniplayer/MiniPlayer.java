package com.example.miniplayer;

import android.view.Surface;

public class MiniPlayer {
    public native void native_setDataSource(String path);
    public native void native_setSurface(Surface surface);
    public native boolean native_start();
}
