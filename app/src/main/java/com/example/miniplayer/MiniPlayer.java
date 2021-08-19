package com.example.miniplayer;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.nfc.Tag;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MiniPlayer extends SurfaceView implements Runnable, SurfaceHolder.Callback {

    public MiniPlayer(Context context, AttributeSet attr) {
        super(context, attr);
        getHolder().addCallback(this);
        Log.i("MiniPlayer", "MiniPlayer()");
    }

    @Override
    public void run() {
        Log.i("MiniPlayer", "run");
        Open("/storage/emulated/0/vrtest/VRc.ts", getHolder().getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i("MiniPlayer", "surfaceCreated" + getHolder().getSurface());
        new Thread(this).start();
        //Open("/storage/emulated/0/vrtest/VRc.ts", getHolder().getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i("MiniPlayer", "surfaceChanged");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i("MiniPlayer", "surfaceDestroyed");
    }

    public native boolean Open(String path, Object surface);
}
