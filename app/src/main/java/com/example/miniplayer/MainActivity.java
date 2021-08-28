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
    private SurfaceHolder surfaceHolder;
    private SurfaceView surfaceView;
    private MiniPlayer miniPlayer;
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
            Log.d(TAG, "onStartTrackingTouch, " + miniPlayer);
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            Log.d(TAG, "onStartTrackingTouch, " + miniPlayer);
        }
    };
    // Used to load the 'native-lib' library on application startup.
    static {
        //在cmakelist中，native-lib依赖其它ffmpeg库，所以在load该库时ffmpeg库也会load
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //去掉标题栏
//        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
//        //隐藏状态栏
//        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
//                WindowManager.LayoutParams.FLAG_FULLSCREEN);
//        //横屏
//        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);
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
        surfaceView = findViewById(R.id.surfaceView);
        surfaceHolder = surfaceView.getHolder();
        miniPlayer = new MiniPlayer();
        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d(TAG, "surfaceCreated, " + miniPlayer);
                surfaceHolder = holder;
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.d(TAG, "surfaceChanged, format:" + format + " width:" + width + " height:" + height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d(TAG, "surfaceDestory");
            }
        });
        Log.d(TAG, "onCreate thread");

        //请求权限
        RequestPermissions(MainActivity.this);
    }

    @RequiresApi(api = Build.VERSION_CODES.Q)
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.stop:{
                surfaceView.setVisibility(View.VISIBLE);
                Log.d(TAG, "stop click");
                break;
            }
            case R.id.reset:{
                surfaceView.setVisibility(View.VISIBLE);
                Log.d(TAG, "reset click");
                break;
            }
            case R.id.release:{
                miniPlayer.native_release();
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
                    int isHardwareAccelerated = 0;
                    int isSoftwareOnly = 1;
                    int isVendor = 0;
                    if (!codecInfo.isEncoder()) {
                        continue;
                    }
                    for(String xtype : types) {
                        if (TextUtils.isEmpty(xtype))
                            continue;
                        type = xtype;
                    }
                    if(codecInfo.isHardwareAccelerated())
                        isHardwareAccelerated = 1;
                    else
                        isHardwareAccelerated = 0;
                    if(codecInfo.isSoftwareOnly())
                        isSoftwareOnly = 1;
                    else
                        isSoftwareOnly = 0;
                    if(codecInfo.isVendor())
                        isVendor = 1;
                    else
                        isVendor = 0;
                    if(isVendor == 0 && isHardwareAccelerated == 0)
                        continue;
                    Log.d("MiniPlayer", String.format(Locale.US, "  found codec: %30s, \t\tsupprot type:%s, \tisHardwareAccelerated:%d isSoftwareOnly:%d isVendor:%d", codecInfo.getName(), type, isHardwareAccelerated, isSoftwareOnly, isVendor));
                }
                break;
            }
            MediaCodec
            case R.id.restart:{
                Log.d(TAG, "restart click");
                break;
            }
            case R.id.pause: {
                Log.d(TAG, "pause click");
                break;
            }
            case R.id.start: {
                if(surfaceHolder == null) {
                    Log.d(TAG, "surfaceHolder is null");
                    break;
                }
//                Log.d(TAG, "onClick thread");
//                while(true);
                miniPlayer.native_setSurface(surfaceHolder.getSurface());
                miniPlayer.native_setDataSource("/sdcard/video/chip_data.ts");
                miniPlayer.native_setLoglevel(miniPlayer.AV_LOG_INFO);
//                miniPlayer.native_setDataSource("/storage/emulated/0/vrtest/VRc.ts");
                miniPlayer.native_start();
//                Log.d(TAG, "start end");
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
                Log.i(TAG,"申请权限成功，打开");
            } else {
                Log.i(TAG,"申请权限失败");
            }
        }
    }

    private boolean RequestPermissions(@NonNull Context context) {
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.i(TAG,"没有授权，申请权限");
            ActivityCompat.requestPermissions(this, new String[]{
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE}, WRITE_STORAGE_REQUEST_CODE);
            return false;
        } else {
            Log.i(TAG,"有权限，打开文件");
            return true;
        }
    }
}