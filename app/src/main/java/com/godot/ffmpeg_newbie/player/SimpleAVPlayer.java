package com.godot.ffmpeg_newbie.player;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
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
        System.loadLibrary("swresample-3");
        System.loadLibrary("avutil-56");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("newbie");
    }

    public static AudioTrack createAudioTrack(int sampleRate, int nbChannel) {
        int audioFmt = AudioFormat.ENCODING_PCM_16BIT;
        int chanelConfig = nbChannel > 1 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, chanelConfig, audioFmt);
        return new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, chanelConfig,
                audioFmt, bufferSize, AudioTrack.MODE_STREAM);
    }
    public native static boolean play(String fileName, Surface surface);
    public native static boolean release();
    public native static boolean playV2(String fileName, Surface surface);
    public native static boolean releaseV2();
}
