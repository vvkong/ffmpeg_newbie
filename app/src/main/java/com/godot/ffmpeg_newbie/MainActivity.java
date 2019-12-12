package com.godot.ffmpeg_newbie;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("avutil-56");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("swscale-5");
        System.loadLibrary("newbie");
    }

    private TextView tvInfo;
    public static final String VIDEO_PATH = "/sdcard/avtest/test.mp4";
    public static final String VIDEO_DST_PATH = "/sdcard/avtest/";
    private int clickId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
        tvInfo = findViewById(R.id.tv_info);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for( int p : grantResults ) {
            if( p == PackageManager.PERMISSION_DENIED ) {
                Toast.makeText(this, "没有权限", Toast.LENGTH_LONG).show();
                return;
            }
        }
        doRealClick(clickId);
    }

    private void getInfo() {
        //printMetaData("file:///android_asset/test.mp4");
        String ret = printMetaData("/sdcard/DCIM/Camera/VID_20191126220240.mp4");
        tvInfo.setText(ret);
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public void onClick(View view) {
        clickId = view.getId();
        if(PackageManager.PERMISSION_GRANTED == ContextCompat.checkSelfPermission(this, Manifest.permission_group.STORAGE)) {
            doRealClick(clickId);
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 11);
        }
    }

    boolean isRunning = false;
    private void doRealClick(final int id) {
        switch (id){
            case R.id.btn_trans2yuv_v1:
            case R.id.btn_trans2yuv_v2:
                if( !isRunning ) {
                    isRunning = true;
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            String dst = VIDEO_DST_PATH + "test.yuv";
                            File f = new File(dst);
                            if( f.exists() ) {
                                f.delete();
                            }
                            if( id == R.id.btn_trans2yuv_v2 ) {
                                trans2yuvv2(VIDEO_PATH, dst);
                            } else {
                                trans2yuvv1(VIDEO_PATH, dst);
                            }
                            isRunning = false;
                        }
                    }).start();
                }
                break;
            case R.id.btn_get_info:
                getInfo();
                break;
            case R.id.btn_simple_player:
                startActivity(new Intent(this, SimplePlayerActivity.class));
                break;
        }
    }

    // 获取并打印媒体信息
    public static native String printMetaData(String videoPath);
    // 转为yuv格式视频文件
    public static native boolean trans2yuvv1(String videoSrc, String videoDst);
    public static native boolean trans2yuvv2(String videoSrc, String videoDst);
}
