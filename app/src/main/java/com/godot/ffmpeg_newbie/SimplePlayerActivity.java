package com.godot.ffmpeg_newbie;

import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.godot.ffmpeg_newbie.player.SimplePlayer;
import com.godot.ffmpeg_newbie.player.VVideoView;

public class SimplePlayerActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private VVideoView videoView;
    private SimplePlayer simplePlayer;
    private Spinner spinner;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_player);

        videoView = findViewById(R.id.video_view);
        simplePlayer = new SimplePlayer();

        videoView.getHolder().addCallback(this);

        spinner = findViewById(R.id.sp_video);
        String[] videoArray = getResources().getStringArray(R.array.video_names);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                android.R.id.text1, videoArray);
        spinner.setAdapter(adapter);
    }

    boolean validSurface = false;
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        validSurface = true;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        validSurface = false;
    }

    boolean isRunning = false;
    public void onClick(View view) {
        if( !validSurface ) {
            Toast.makeText(this, "surface 还没准备好", Toast.LENGTH_LONG).show();
            return;
        }
        if( isRunning ) {
            Toast.makeText(this, "请等待这个视频播放结束", Toast.LENGTH_LONG).show();
            return;
        }
        isRunning = true;
        new Thread(new Runnable() {
            @Override
            public void run() {
                simplePlayer.render("/sdcard/avtest/"+spinner.getSelectedItem().toString(), videoView.getHolder().getSurface());
                isRunning = false;
            }
        }).start();
    }
}
