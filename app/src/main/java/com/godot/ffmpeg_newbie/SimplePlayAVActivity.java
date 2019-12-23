package com.godot.ffmpeg_newbie;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.appcompat.app.AppCompatActivity;

import com.godot.ffmpeg_newbie.player.SimpleAVPlayer;
import com.godot.ffmpeg_newbie.player.VVideoView;

public class SimplePlayAVActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    public static Intent buildIntent(Context sourceAct, boolean version2) {
        Intent intent = new Intent(sourceAct, SimplePlayAVActivity.class);
        intent.putExtra("key", version2);
        return intent;
    }

    private VVideoView videoView;
    boolean v2 = false;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_play_av);
        v2 = getIntent().getBooleanExtra("key", false);
        videoView = findViewById(R.id.video_view);
        videoView.getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        String f = getVideoFile();
        Surface sf = holder.getSurface();
        boolean ret = v2 ? SimpleAVPlayer.playV2(f, sf) : SimpleAVPlayer.play(f, sf);
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
