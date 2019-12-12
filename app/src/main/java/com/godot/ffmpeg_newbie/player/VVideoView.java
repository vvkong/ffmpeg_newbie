package com.godot.ffmpeg_newbie.player;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Build;
import android.util.AttributeSet;
import android.view.SurfaceView;

import androidx.annotation.RequiresApi;

/**
 * @Desc 说明
 * @Author Godot
 * @Date 2019-12-11
 * @Version 1.0
 * @Mail 809373383@qq.com
 */
public class VVideoView extends SurfaceView {
    public VVideoView(Context context) {
        this(context, null);
    }

    public VVideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public VVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public VVideoView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        getHolder().setFormat(PixelFormat.RGBA_8888);
    }
}
