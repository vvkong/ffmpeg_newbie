package com.godot.ffmpeg_newbie.player;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import androidx.annotation.Keep;

/**
 * @Desc 说明
 * @Author Godot
 * @Date 2019-12-13
 * @Version 1.0
 * @Mail 809373383@qq.com
 */
public class SimpleAudioPlay {

    static {
        System.loadLibrary("yuv");
        System.loadLibrary("swscale-5");
        System.loadLibrary("avutil-56");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("swresample-3");
        System.loadLibrary("newbie");
    }

    public SimpleAudioPlay() {
    }

    @Keep
    public AudioTrack createAudioTrack(int sampleRate, int nb_channels) {
        int channelConfig = nb_channels == 1 ? AudioFormat.CHANNEL_IN_DEFAULT : AudioFormat.CHANNEL_IN_STEREO;
        int buffSize = AudioTrack.getMinBufferSize(sampleRate, nb_channels, AudioFormat.ENCODING_PCM_16BIT);
        AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig,
                AudioFormat.ENCODING_PCM_16BIT,
                buffSize,
                AudioTrack.MODE_STREAM);
//        audioTrack.play();
//        audioTrack.write();
//        audioTrack.stop();
//        audioTrack.release();
        return audioTrack;
    }

    public native void playAudio(String fileName);
}
