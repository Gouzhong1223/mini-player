package com.example.miniplayer;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.util.Locale;

import static android.media.MediaCodec.BUFFER_FLAG_CODEC_CONFIG;


public class MainActivity extends AppCompatActivity implements View.OnClickListener{
    String TAG = "MiniPlayer";
    private static final int WRITE_STORAGE_REQUEST_CODE = 100;
    private SurfaceHolder surfaceHolder1;
    private SurfaceHolder surfaceHolder2;
    private SurfaceHolder surfaceHolder3;
    private SurfaceView surfaceView1;
    private SurfaceView surfaceView2;
    private SurfaceView surfaceView3;
    //private MiniPlayer miniPlayer;
    Button stopButton;
    Button resetButton;
    Button releaseButton;
    Button setDataSourceButton;
    Button prepareButton;
    Button restarButton;
    Button pauseButton;
    Button startButton;
    SeekBar seekBar;

    private SeekBar.OnSeekBarChangeListener onSeekBarChangeListener = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            Log.d(TAG, "onProgressChanged, progress:" + progress + " fromUser:" + fromUser);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            Log.d(TAG, "onStartTrackingTouch, ");
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            Log.d(TAG, "onStartTrackingTouch, ");
        }
    };
    // Used to load the 'native-lib' library on application startup.
    static {
        //???cmakelist??????native-lib????????????ffmpeg???????????????load?????????ffmpeg?????????load
        System.loadLibrary("mini-player");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //???????????????
//        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
//        //???????????????
//        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
//                WindowManager.LayoutParams.FLAG_FULLSCREEN);
//        //??????
//        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setContentView(R.layout.activity_main);
        stopButton = findViewById(R.id.stop);
        stopButton.setOnClickListener(this);
        resetButton = findViewById(R.id.reset);
        resetButton.setOnClickListener(this);
        releaseButton = findViewById(R.id.release);
        releaseButton.setOnClickListener(this);
        setDataSourceButton = findViewById(R.id.setDataSource);
        setDataSourceButton.setOnClickListener(this);
        prepareButton = findViewById(R.id.prepare);
        prepareButton.setOnClickListener(this);
        restarButton = findViewById(R.id.restart);
        restarButton.setOnClickListener(this);
        pauseButton = findViewById(R.id.pause);
        pauseButton.setOnClickListener(this);
        startButton = findViewById(R.id.start);
        startButton.setOnClickListener(this);
        seekBar = findViewById(R.id.seekbar);
        seekBar.setOnSeekBarChangeListener(onSeekBarChangeListener);

        surfaceView1 = findViewById(R.id.surfaceView1);
        surfaceHolder1 = surfaceView1.getHolder();
        surfaceView2 = findViewById(R.id.surfaceView2);
        surfaceHolder2 = surfaceView2.getHolder();
        surfaceView3 = findViewById(R.id.surfaceView3);
        surfaceHolder3 = surfaceView3.getHolder();

        //miniPlayer = new MiniPlayer();
        surfaceHolder1.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d(TAG, "1surfaceCreated, ");
                surfaceHolder1 = holder;
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d(TAG, "1surfaceChanged, format:" + format + " width:" + width + " height:" + height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d(TAG, "surfaceDestory");
            }
        });

        surfaceHolder2.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d(TAG, "2surfaceCreated, ");
                surfaceHolder2 = holder;
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d(TAG, "2surfaceChanged, format:" + format + " width:" + width + " height:" + height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d(TAG, "surfaceDestory");
            }
        });

        Log.d(TAG, "onCreate thread");

        //????????????
        RequestPermissions(MainActivity.this);
    }

    @RequiresApi(api = Build.VERSION_CODES.Q)
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.stop:{
                surfaceView1.setVisibility(View.VISIBLE);
                surfaceView2.setVisibility(View.VISIBLE);
                Log.d(TAG, "stop click");
                break;
            }
            case R.id.reset:{
                surfaceView1.setVisibility(View.VISIBLE);
                Log.d(TAG, "reset click");
                break;
            }
            case R.id.release:{
                //miniPlayer.native_release();
                Log.d(TAG, "release click");
                break;
            }
            case R.id.setDataSource:{
                Log.d(TAG, "setDataSource click");
                break;
            }
            case R.id.prepare: {
                int numCodecs = MediaCodecList.getCodecCount();
                for (int i = 0; i < numCodecs; i++) {
                    MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
                    String[] types = codecInfo.getSupportedTypes();
                    String type = null;
                    if (codecInfo.isEncoder()) {
                        continue;
                    }
                    for(String xtype : types) {
                        if (TextUtils.isEmpty(xtype))
                            continue;
                        type = xtype;
                    }
//                    if(codecInfo.isHardwareAccelerated())
//                        isHardwareAccelerated = 1;
//                    else
//                        isHardwareAccelerated = 0;
//                    if(codecInfo.isSoftwareOnly())
//                        isSoftwareOnly = 1;
//                    else
//                        isSoftwareOnly = 0;
//                    if(codecInfo.isVendor())
//                        isVendor = 1;
//                    else
//                        isVendor = 0;
//                    if(isVendor == 0 && isHardwareAccelerated == 0)
//                        continue;
                    Log.d("MiniPlayer", String.format(Locale.US, "  found codec: %-30s, \t\tsupprot type:%s", codecInfo.getName(), type));
                }
                break;
            }

            case R.id.restart:{
                Log.d(TAG, "restart click");
                MiniPlayer miniPlayer = new MiniPlayer();
                miniPlayer.init();
                Log.d(TAG, "restart click 1");
                miniPlayer.setSurface(surfaceHolder1.getSurface());
                Log.d(TAG, "restart click 2");
////                miniPlayer.setDataSource("/sdcard/video/VR.ts");
//                //miniPlayer.native_setDataSource("/sdcard/cctv8.ts");
//                //miniPlayer.setLoglevel(miniPlayer.AV_LOG_INFO);
                miniPlayer.setDataSource("/sdcard/video/test.mp4");
                Log.d(TAG, "restart click 3");
                miniPlayer.start();
                Log.d(TAG, "a start");
                break;
            }
            case R.id.pause: {
                MiniPlayer miniPlayer = new MiniPlayer();
                miniPlayer.init();
//                miniPlayer.native_setup();
                miniPlayer.setSurface(surfaceHolder2.getSurface());
                //miniPlayer.setDataSource("/sdcard/video/VR.ts");
                //miniPlayer.native_setDataSource("/sdcard/cctv8.ts");
                //miniPlayer.setLoglevel(miniPlayer.AV_LOG_INFO);
                miniPlayer.setDataSource("/sdcard/video/test.flv");
                miniPlayer.start();
                break;
            }
            case R.id.start: {
                if(surfaceHolder1 == null || surfaceHolder2 == null) {
                    Log.d(TAG, "surfaceHolder is null");
                    break;
                }
                MiniPlayer miniPlayer = new MiniPlayer();
                miniPlayer.init();

                miniPlayer.setSurface(surfaceHolder3.getSurface());
               // miniPlayer.setDataSource("/sdcard/video/VR.ts");
                //miniPlayer.native_setDataSource("/sdcard/cctv8.ts");
                miniPlayer.setLoglevel(miniPlayer.AV_LOG_INFO);
                miniPlayer.setDataSource("/sdcard/video/test1.flv");
                miniPlayer.start();
//                break;

            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == WRITE_STORAGE_REQUEST_CODE) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED &&
                    ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
                Log.i(TAG,"???????????????????????????");
            } else {
                Log.i(TAG,"??????????????????");
            }
        }
    }

    private boolean RequestPermissions(@NonNull Context context) {
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.i(TAG,"???????????????????????????");
            ActivityCompat.requestPermissions(this, new String[]{
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE}, WRITE_STORAGE_REQUEST_CODE);
            return false;
        } else {
            Log.i(TAG,"????????????????????????");
            return true;
        }
    }
}