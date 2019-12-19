//
// Created by wangrenxing on 2019-12-18.
//
#include "newbie.h"
#include <pthread.h>
#include <unistd.h>
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

// 注意链接动态库 jnigraphics, android
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define MAX_BUFFER_SIZE (48000 * 4)

#define AUDIO_IDX 0
#define VIDEO_IDX 1
// 用c++面向对象来封装更好
struct PlayerContext {
    JavaVM* jvm;
    jobject g_ref_avplay_cls;
    jobject g_ref_audio_track_cls;
    jobject g_ref_audio_track;
    jmethodID audio_write_mid;
    bool playing;

    AVFormatContext* ifmt_ctx;
    // 0 存储音频索引 1 存储视频索引
    int stream_idx[2];
    AVCodecContext* codec_ctx[2];
    AVCodec* codec[2];

    AVFrame* v_frame;
    AVFrame* rgba_fame;
    SwsContext* sws_ctx;

    // 根据展示窗口调整大小，约定展示格式
    int sf_width;
    int sf_height;
    int adjust_width;
    int adjust_height;
    AVPixelFormat pix_fmt;
    ANativeWindow* nativeWindow;
    ANativeWindow_Buffer* buffer;

    AVFrame* a_frame;
    SwrContext* swr_ctx;
    uint64_t out_layout;
    int out_nb_channel;
    AVSampleFormat out_sample_fmt;
    int out_sample_rate;
    uint8_t *audio_out_buffer;
};

static struct PlayerContext g_player_ctx = {
        .jvm = NULL,
        .g_ref_avplay_cls = NULL,
        .playing = false,
        .ifmt_ctx = NULL,
        .stream_idx = {-1, -1},
        .codec_ctx = {NULL, NULL},
        .codec = {NULL, NULL},
        .v_frame = NULL,
        .rgba_fame = NULL,
        .sws_ctx = NULL,
        .nativeWindow = NULL,
        .buffer = NULL,
        .a_frame = NULL,
        .swr_ctx = NULL,
        .audio_out_buffer = NULL
};

void release_player_context_if_need(JNIEnv* env, PlayerContext& player_ctx);

void print_error(const char* msg, int code) {
    char err[256] = {'\0'};
    av_strerror(code, err, sizeof(err));
    LOGE("%s, err: %s", msg, err);
}

bool init_input_format(const char *file, struct PlayerContext& player_ctx) {
    bool success = false;
    int ret;
    if( (ret = avformat_open_input(&player_ctx.ifmt_ctx, file, NULL, NULL)) < 0 ) {
        print_error("avformat_open_input fail. ", ret);
        goto label_end;
    }
    if( (ret=avformat_find_stream_info(player_ctx.ifmt_ctx, NULL)) < 0 ) {
        print_error("avformat_find_stream_info fail. ", ret);
        goto label_end;
    }
    success = true;
label_end:
    return success;
}

bool init_codec(struct PlayerContext& player_ctx) {
    bool success = false;
    int ret;
    for( int i=0; i<player_ctx.ifmt_ctx->nb_streams; i++ ) {
        if( AVMEDIA_TYPE_AUDIO == player_ctx.ifmt_ctx->streams[i]->codecpar->codec_type ) {
            player_ctx.stream_idx[AUDIO_IDX] = i;
            success = true;
        } else if(AVMEDIA_TYPE_VIDEO == player_ctx.ifmt_ctx->streams[i]->codecpar->codec_type ) {
            player_ctx.stream_idx[VIDEO_IDX] = i;
            success = true;
        }
    }
    if( !success ) {
        goto label_end;
    }
    for( int i=0; i<2; i++ ) {
        int idx = player_ctx.stream_idx[i];
        if( player_ctx.stream_idx[idx] >= 0 ) {
            AVStream* stream = player_ctx.ifmt_ctx->streams[idx];
            player_ctx.codec[i] = avcodec_find_decoder(stream->codecpar->codec_id);
            if( !player_ctx.codec[i] ) {
                LOGE("avcodec_find_decoder return null, %s", "");
                goto label_end;
            }
            player_ctx.codec_ctx[i] = avcodec_alloc_context3(player_ctx.codec[i]);
            if( player_ctx.codec_ctx[i] == NULL ) {
                LOGE("avcodec_alloc_context3 fail. %s", "");
                goto label_end;
            }
            if( (ret=avcodec_parameters_to_context(player_ctx.codec_ctx[i], stream->codecpar)) < 0 ) {
                print_error("avcodec_parameters_to_context fail. %s", ret);
                goto label_end;
            }
            if( (ret=avcodec_open2(player_ctx.codec_ctx[i], player_ctx.codec[i], NULL)) < 0 ) {
                print_error("avcodec_open2 fail. %s", ret);
                goto label_end;
            }
        }
    }
    success = true;
label_end:
    return success;
}

bool decode_audio_frame(JNIEnv* env, const PlayerContext * const player_ctx, AVPacket& packet) {
    AVCodecContext* avctx = player_ctx->codec_ctx[AUDIO_IDX];
    AVCodec* codec = player_ctx->codec[AUDIO_IDX];
    int ret = avcodec_send_packet(avctx, &packet);
    if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
        return true;
    } else if( ret < 0 ) {
        print_error("avcodec_send_packet fail.", ret);
        return false;
    }
    AVFrame* frame = player_ctx->a_frame;
    while( ret >= 0 ) {
        ret = avcodec_receive_frame(avctx, frame);
        if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
            break;
        } else if( ret < 0 ) {
            print_error("avcodec_receive_frame fail.", ret);
            return false;
        }
        //int out_samples = swr_get_out_samples(player_ctx->swr_ctx, frame->nb_samples);
        uint8_t* ob_arr[2] = {NULL, NULL};
        ob_arr[0] = player_ctx->audio_out_buffer;
        if( (ret = swr_convert(
                player_ctx->swr_ctx,
                ob_arr,
                frame->nb_samples,
                (const uint8_t **)(frame->data),
                frame->nb_samples)) < 0 ) {
            print_error("swr_convert fail.", ret);
            return false;
        }
        int out_samples = ret;

        int size = av_samples_get_buffer_size(NULL, player_ctx->out_nb_channel, frame->nb_samples, player_ctx->out_sample_fmt, 1);
        LOGD("av_samples_get_buffer_size: %d, ret*player_ctx->out_nb_channel: %d", size, out_samples*player_ctx->out_nb_channel);
        if( size < 0 ) {
            LOGE("av_samples_get_buffer_size fail.%s", "");
            return false;
        }
        jbyteArray jArr = env->NewByteArray(size);
        if( !jArr ) {
            LOGE("env->NewByteArray(out_samples*player_ctx->out_nb_channel) fail. %s", "");
            return false;
        }
        env->SetByteArrayRegion(jArr, 0, size,
                                reinterpret_cast<const jbyte *>(player_ctx->audio_out_buffer));
        env->CallIntMethod(player_ctx->g_ref_audio_track, player_ctx->audio_write_mid, jArr, 0, size);
        env->DeleteLocalRef(jArr);
        //usleep(16*1000);
    }
    return true;
}

bool decode_video_frame(const PlayerContext * const player_ctx, AVPacket& packet) {
    AVCodecContext* avctx = player_ctx->codec_ctx[VIDEO_IDX];
    AVCodec* codec = player_ctx->codec[VIDEO_IDX];
    int ret = avcodec_send_packet(avctx, &packet);
    if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
        return true;
    } else if( ret < 0 ) {
        print_error("avcodec_send_packet fail.", ret);
        return false;
    }
    AVFrame* frame = player_ctx->v_frame;
    while( ret >= 0 ) {
        ret = avcodec_receive_frame(avctx, frame);
        if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
            break;
        } else if( ret < 0 ) {
            print_error("avcodec_receive_frame fail.", ret);
            return false;
        }

        int slice = sws_scale(player_ctx->sws_ctx, frame->data, frame->linesize, 0, frame->height,
                player_ctx->rgba_fame->data, player_ctx->rgba_fame->linesize);
        LOGD("slice: %d", slice);
        ANativeWindow_lock(player_ctx->nativeWindow, (player_ctx->buffer) , NULL);

        uint8_t* src = player_ctx->rgba_fame->data[0];
        uint8_t* dst = static_cast<uint8_t *>((player_ctx->buffer->bits));

        int src_linesize = (player_ctx->rgba_fame->linesize[0]);
        int dst_linesize = player_ctx->buffer->stride * 4;

        LOGD("buffer line size: %d, rgba frame line size: %d", dst_linesize, src_linesize);
        LOGD("rgba frame w: %d, h: %d", player_ctx->rgba_fame->width, player_ctx->rgba_fame->height);

        // 居中展示
        int xoffset = (dst_linesize-src_linesize) >> 1;
        if( xoffset < 0 ) {
            xoffset = 0;
        }
        int yoffset = (player_ctx->sf_height-avctx->height) >> 1;
        if( yoffset < 0 ) {
            yoffset = 0;
        }
        dst += yoffset * (dst_linesize);

        for( int i=0; i<avctx->height; i++ ) {
            memcpy(dst + xoffset, src, src_linesize);
            dst += dst_linesize;
            src += src_linesize;
        }
        ANativeWindow_unlockAndPost(player_ctx->nativeWindow);
        usleep(16*1000);
    }
    return true;
}

void* decode_data_fun(void* args) {
    LOGD("in decode_data_fun, %s", "");
    PlayerContext* player_ctx = (PlayerContext*) args;
    JNIEnv* env = NULL;
    AVPacket packet;
    int frame_count = 0;
    int ret;
    if( JNI_OK != player_ctx->jvm->AttachCurrentThread(&env, NULL) ) {
        LOGE("AttachCurrentThread fail. %s", "");
        goto label_end;
    }
    if( player_ctx->codec_ctx[AUDIO_IDX] ) {
        jmethodID play_mid = env->GetMethodID(static_cast<jclass>(player_ctx->g_ref_audio_track_cls), "play", "()V");
        env->CallVoidMethod(player_ctx->g_ref_audio_track, play_mid);

        player_ctx->audio_write_mid = env->GetMethodID(
                static_cast<jclass>(player_ctx->g_ref_audio_track_cls), "write", "([BII)I");
    }
    while ( player_ctx->playing && (ret=av_read_frame(player_ctx->ifmt_ctx, &packet)) >= 0 ) {
        LOGD("decode frame: %d", ++frame_count);
        if( packet.stream_index == player_ctx->stream_idx[AUDIO_IDX] ) {
            decode_audio_frame(env, player_ctx, packet);
        } else if( packet.stream_index == player_ctx->stream_idx[VIDEO_IDX]){
            decode_video_frame(player_ctx, packet);
        }
        av_packet_unref(&packet);
    }

    if( ret < 0 ) {
        print_error("av_read_frame fail. ", ret);
    }
    LOGD("end decode_data_fun %s", "");
label_end:
    release_player_context_if_need(env, *player_ctx);
    if( env ) {
        player_ctx->jvm->DetachCurrentThread();
    }
    pthread_exit(NULL);
}

bool prepare_for_decode(PlayerContext *const player_ctx, JNIEnv *env, jobject surface) {
    /**
     * 音频准备
     */
    AVCodecContext* audio_codec_ctx = player_ctx->codec_ctx[AUDIO_IDX];
    AVCodec* audio_codec = player_ctx->codec[AUDIO_IDX];
    if( audio_codec_ctx ) {
        player_ctx->swr_ctx = swr_alloc();
        if( !player_ctx->swr_ctx ) {
            LOGE("swr_alloc fail. %s", "");
            return false;
        }
        player_ctx->out_layout = AV_CH_LAYOUT_STEREO;
        player_ctx->out_nb_channel = av_get_channel_layout_nb_channels(player_ctx->out_layout);
        player_ctx->out_sample_fmt = AV_SAMPLE_FMT_S16;
        player_ctx->out_sample_rate = 44100;
        player_ctx->swr_ctx = swr_alloc_set_opts(player_ctx->swr_ctx,
                    player_ctx->out_layout, player_ctx->out_sample_fmt, player_ctx->out_sample_rate,
                    audio_codec_ctx->channel_layout, audio_codec_ctx->sample_fmt, audio_codec_ctx->sample_rate,
                    0, NULL);
        if( !player_ctx->swr_ctx ) {
            LOGE("swr_alloc_set_opts fail. %s", "");
            return false;
        }
        // api 没告诉返回啥成功，略过不表
        swr_init(player_ctx->swr_ctx);

        player_ctx->audio_out_buffer = static_cast<uint8_t *>(av_malloc(MAX_BUFFER_SIZE));
        if( !player_ctx->audio_out_buffer ) {
            LOGE("av_malloc(MAX_BUFFER_SIZE) fail. %s", "");
            return false;
        }

        jclass av_player_cls = env->FindClass("com/godot/ffmpeg_newbie/player/SimpleAVPlayer");
        if( !av_player_cls ) {
            LOGD("env->FindClass(\"com/godot/ffmpeg_newbie/player/SimpleAVPlayer\") fail. %s", "");
            return  false;
        }
        player_ctx->g_ref_avplay_cls = env->NewGlobalRef(av_player_cls);
        jmethodID create_au_mid = env->GetStaticMethodID(av_player_cls, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
        jobject au = env->CallStaticObjectMethod(av_player_cls, create_au_mid, player_ctx->out_sample_rate, player_ctx->out_nb_channel);
        player_ctx->g_ref_audio_track = env->NewGlobalRef(au);
        player_ctx->g_ref_audio_track_cls = env->NewGlobalRef(env->GetObjectClass(au));
    }

    /**
     * 视频准备
     */
    AVCodecContext* av_codec_ctx = player_ctx->codec_ctx[VIDEO_IDX];
    // 有展示surface、视频时才进行必要初始化
    if( surface && av_codec_ctx ) {
        player_ctx->v_frame = av_frame_alloc();
        if( !(player_ctx->v_frame) ) {
            LOGE("av_frame_alloc fail. %s", "");
            return false;
        }
        player_ctx->a_frame = av_frame_alloc();
        if( !(player_ctx->a_frame) ) {
            LOGE("av_frame_alloc fail. %s", "");
            return false;
        }

        ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
        if( !nativeWindow ) {
            LOGE("ANativeWindow_fromSurface return null. %s", "");
            return false;
        }
        player_ctx->nativeWindow = nativeWindow;
        player_ctx->buffer = new ANativeWindow_Buffer();
        const int sf_width = ANativeWindow_getWidth(nativeWindow);
        const int sf_height = ANativeWindow_getHeight(nativeWindow);
        int ret;
        if( (ret = ANativeWindow_setBuffersGeometry(nativeWindow, sf_width, sf_height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM)) < 0 ) {
            LOGE("ANativeWindow_setBuffersGeometry fail, ret: %d", ret);
            return false;
        }
        player_ctx->sf_width = sf_width;
        player_ctx->sf_height = sf_height;

        // 与surface设置像素格式一致
        player_ctx->pix_fmt = AV_PIX_FMT_RGBA;
        // 计算视频合适的展示大小(按比例缩放是否能填充窗口大小，以不超出准则来)
        if( sf_width * av_codec_ctx->height > sf_height * av_codec_ctx->width ) {
            player_ctx->adjust_height = sf_height;
            player_ctx->adjust_width = player_ctx->adjust_height * av_codec_ctx->width / av_codec_ctx->height;
        } else {
            player_ctx->adjust_width = sf_width;
            player_ctx->adjust_height = player_ctx->adjust_width * av_codec_ctx->height / av_codec_ctx->width;
        }

        LOGD("surface w: %d, h: %d; video w: %d, h: %d; adjust w: %d, h: %d",
                sf_width, sf_height,
                av_codec_ctx->width, av_codec_ctx->height,
                player_ctx->adjust_width, player_ctx->adjust_height);

        player_ctx->sws_ctx = sws_getContext(av_codec_ctx->width, av_codec_ctx->height, av_codec_ctx->pix_fmt,
                                             player_ctx->adjust_width, player_ctx->adjust_height, player_ctx->pix_fmt,
                                             SWS_BICUBIC, NULL, NULL, NULL);
        if( !player_ctx->sws_ctx ) {
            LOGE("sws_getContext fail. %s", "");
            return false;
        }

        player_ctx->rgba_fame = av_frame_alloc();
        if( !(player_ctx->rgba_fame) ) {
            LOGE("av_frame_alloc fail. %s", "");
            return false;
        }
        if( (ret=av_image_alloc(player_ctx->rgba_fame->data, player_ctx->rgba_fame->linesize,
                       player_ctx->adjust_width, player_ctx->adjust_height, player_ctx->pix_fmt, 1)) < 0 ) {
            print_error("av_image_alloc fail.", ret);
            return false;
        }
    }
    return true;
}



extern "C"
JNIEXPORT jboolean JNICALL
Java_com_godot_ffmpeg_1newbie_player_SimpleAVPlayer_play(JNIEnv *env, jclass clazz,
                                                          jstring file_name, jobject surface) {
    jboolean isCopy = JNI_FALSE;
    PlayerContext* const player_ctx = &g_player_ctx;

    if( JNI_OK != env->GetJavaVM(&player_ctx->jvm) ) {
        LOGD("GetJavaVM fail %s", "");
    }

    LOGD("start %#x", &g_player_ctx);

    const char* file = env->GetStringUTFChars(file_name, &isCopy);

    LOGD("file: %s", file);

    if( !init_input_format(file, *player_ctx) ) {
        goto label_end;
    }
    if( !init_codec(*player_ctx) ) {
        goto label_end;
    }
    if( !prepare_for_decode(player_ctx, env, surface) ) {
        goto label_end;
    }

    player_ctx->playing = true;
    pthread_t pt;
    pthread_create(&pt, NULL, decode_data_fun, (void*)player_ctx);
    LOGD("after pthread_create %s", "");

    env->ReleaseStringUTFChars(file_name, file);
    return JNI_TRUE;
label_end:
    release_player_context_if_need(env, *player_ctx);
    env->ReleaseStringUTFChars(file_name, file);
    return JNI_FALSE;
}

void release_player_context_if_need(JNIEnv* env, PlayerContext& player_ctx) {
    if( (!env) && player_ctx.codec_ctx[AUDIO_IDX] ) {
       if( player_ctx.g_ref_audio_track ) {
           jmethodID stop_mid = env->GetMethodID(static_cast<jclass>(player_ctx.g_ref_audio_track_cls), "stop", "()V");
           env->CallVoidMethod(player_ctx.g_ref_audio_track, stop_mid);
           jmethodID release_mid = env->GetMethodID(static_cast<jclass>(player_ctx.g_ref_audio_track_cls), "release", "()V");
           env->CallVoidMethod(player_ctx.g_ref_audio_track, release_mid);

           env->DeleteGlobalRef(player_ctx.g_ref_audio_track_cls);
           player_ctx.g_ref_audio_track_cls = NULL;
           env->DeleteGlobalRef(player_ctx.g_ref_audio_track);
           player_ctx.g_ref_audio_track = NULL;
       }
        if( player_ctx.g_ref_avplay_cls ) {
            env->DeleteGlobalRef(player_ctx.g_ref_avplay_cls);
            player_ctx.g_ref_avplay_cls = NULL;
        }
    }

    if( player_ctx.a_frame ) {
        av_frame_free(&player_ctx.a_frame);
        player_ctx.a_frame = NULL;
    }
    if( player_ctx.swr_ctx ) {
        if( swr_is_initialized(player_ctx.swr_ctx) > 0 ) {
            swr_close(player_ctx.swr_ctx);
        }
        swr_free(&(player_ctx.swr_ctx));
    }
    /*
     // 是否出错，为啥？
     if( player_ctx.audio_out_buffer ) {
        av_freep(player_ctx.audio_out_buffer);
        player_ctx.audio_out_buffer = NULL;
    }*/
    if( player_ctx.nativeWindow ) {
        ANativeWindow_release(player_ctx.nativeWindow);
    }
    if( player_ctx.buffer ) {
        delete player_ctx.buffer;
    }

    if( player_ctx.v_frame ) {
        av_frame_free(&player_ctx.v_frame);
    }
    if( player_ctx.rgba_fame ) {
        av_freep(&(player_ctx.rgba_fame->data[0]));
        av_frame_free(&player_ctx.rgba_fame);
    }

    for( int i=0; i<(sizeof(player_ctx.stream_idx)/sizeof(int)); i++ ) {
        if( player_ctx.codec_ctx[i] ) {
            if( avcodec_is_open(player_ctx.codec_ctx[i]) > 0 ) {
                avcodec_close(player_ctx.codec_ctx[i]);
            }
            avcodec_free_context(&player_ctx.codec_ctx[i]);
        }
    }
    if( player_ctx.ifmt_ctx ) {
        avformat_close_input(&player_ctx.ifmt_ctx);
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_godot_ffmpeg_1newbie_player_SimpleAVPlayer_release(JNIEnv *env, jclass clazz) {
//    LOGD("end %#x", &g_player_ctx);
//    PlayerContext* player_ctx = &g_player_ctx;
    // 为啥不能修改？？？？为啥奔溃呢？
//    player_ctx->playing = false;
    return JNI_TRUE;
}