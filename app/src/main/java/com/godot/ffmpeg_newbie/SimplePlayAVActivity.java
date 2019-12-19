package com.godot.ffmpeg_newbie;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;

import androidx.appcompat.app.AppCompatActivity;

import com.godot.ffmpeg_newbie.player.SimpleAVPlayer;
import com.godot.ffmpeg_newbie.player.VVideoView;

public class SimplePlayAVActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private VVideoView videoView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_play_av);

        videoView = findViewById(R.id.video_view);
        videoView.getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        boolean ret = SimpleAVPlayer.play(getVideoFile(), holder.getSurface());
        Log.d("bad-boy", "SimpleAVPlayer.play, ret: " + ret);
    }

    private String getVideoFile() {
        return "/sdcard/avtest/" + "test.mp4";
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        //SimpleAVPlayer.release();
    }
}
