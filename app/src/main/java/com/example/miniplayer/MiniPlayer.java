package com.example.miniplayer;

import android.view.Surface;

public class MiniPlayer {
    public native boolean native_Open(String path, Object surface);
}
