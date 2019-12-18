package com.godot.ffmpeg_newbie.player;

import android.view.Surface;

/**
 * @Desc 说明
 * @Author Godot
 * @Date 2019-12-18
 * @Version 1.0
 * @Mail 809373383@qq.com
 */
public class SimpleAVPlayer {
    static {
        System.loadLibrary("yuv");
        System.loadLibrary("swscale-5");
        System.loadLibrary("avutil-56");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("newbie");
    }

    public native static boolean play(String fileName, Surface surface);
}
