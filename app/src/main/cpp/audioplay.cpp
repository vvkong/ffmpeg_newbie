#include "newbie.h"
#include "stdio.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

static void print_error(const char* err, int code) {
    char err_buf[128] = {'\0'};
    av_strerror(code, err_buf, sizeof(err_buf));
    LOGE("%s fail, ret: %d, error: %s", err, code, err_buf);
}
#define MAX_BUF_SIZE (48000 * 4)

/**
 * 重采样有一个原则，就是“不管你怎么采样，音频总时长不能变”
 * 影响音频播放时长的因素是每帧的采样数和采样率。
 * 每帧有固定采样点数，结合采样率可以决定每帧播放时长
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_godot_ffmpeg_1newbie_player_SimpleAudioPlay_playAudio(JNIEnv *env, jobject thiz,
                                                               jstring file_name) {
    int ret;
    jboolean isCopy = JNI_FALSE;

    AVFormatContext* ifmt_ctx = NULL;
    int audio_stream_index = -1;
    AVCodec* decodec = NULL;
    AVCodecContext* decodec_ctx = NULL;
    AVFrame* frame = NULL;
    AVFrame* out_frame = NULL;
    SwrContext* swr_ctx = NULL;
    uint8_t* out_buf = NULL;
    // 输出声道布局
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    int out_nb_channels = av_get_channel_layout_nb_channels(out_ch_layout);
    // 输出采样格式 PCM 16bit
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    // 输出采样率
    int out_sample_rate = 44100;
    FILE* fp_out = NULL;
    AVPacket packet = {.data = NULL, .size = 0};
    int frame_count = 0;

    jclass audio_play_cls = NULL;
    jmethodID create_audio_track_id;
    jobject audio_track = NULL;
    jclass audio_track_cls = NULL;
    jmethodID  play_id;
    jmethodID stop_id;
    jmethodID release_id;
    jmethodID write_id;


    const char* filename = env->GetStringUTFChars(file_name, &isCopy);
    LOGD("file: %s", filename);
    if( (ret=avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0 ) {
        print_error("avformat_open_input", ret);
        goto label_end;
    }

    if( (ret=avformat_find_stream_info(ifmt_ctx, NULL)) < 0 ) {
        print_error("avformat_find_stream_info", ret);
        goto label_end;
    }

    for( int i=0; i<ifmt_ctx->nb_streams; i++ ) {
        if( AVMEDIA_TYPE_AUDIO == ifmt_ctx->streams[i]->codecpar->codec_type ) {
            audio_stream_index = i;
            break;
        }
    }
    if( audio_stream_index < 0 ) {
        LOGE("couldn't find audio from file. %s", "");
        goto label_end;
    }

    decodec = avcodec_find_decoder(ifmt_ctx->streams[audio_stream_index]->codecpar->codec_id);
    if( !decodec ) {
        LOGE("couldn't find audio decoder. %s", "");
        goto label_end;
    }

    decodec_ctx = avcodec_alloc_context3(decodec);
    if( !decodec_ctx ) {
        LOGE("avcodec_alloc_context3 fail. %s", "");
        goto label_end;
    }
    if( (ret = avcodec_parameters_to_context(decodec_ctx, ifmt_ctx->streams[audio_stream_index]->codecpar)) < 0 ) {
        print_error("avcodec_parameters_to_context", ret);
        goto label_end;
    }

    if( (ret=avcodec_open2(decodec_ctx, decodec, NULL)) < 0 ) {
        print_error("avcodec_open2", ret);
        goto label_end;
    }
    frame = av_frame_alloc();
    if( !frame ) {
        LOGE("av_frame_alloc fail. %s", "");
        goto label_end;
    }

    out_buf = static_cast<uint8_t *>(av_malloc(MAX_BUF_SIZE));
    if( !out_buf ) {
        LOGE("av_malloc fail. %s", "");
        goto label_end;
    }

    // 可以直接用转到buf即可，不用frame
    out_frame = av_frame_alloc();
    if( !out_frame ) {
        LOGE("av_frame_alloc fail. %s", "");
        goto label_end;
    }
    if((ret=av_samples_fill_arrays(out_frame->data, out_frame->linesize,
                                   reinterpret_cast<const uint8_t *>(&out_buf), out_nb_channels, out_sample_rate, out_sample_fmt, 1)) < 0 ) {
        print_error("av_samples_fill_arrays", ret);
        goto label_end;
    }

    swr_ctx = swr_alloc();
    if( !swr_ctx ) {
        LOGE("swr_alloc fail. %s", "");
        goto label_end;
    }
    swr_ctx = swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate,
            decodec_ctx->channel_layout, decodec_ctx->sample_fmt, decodec_ctx->sample_rate, 0, NULL);

    swr_init(swr_ctx);

    fp_out = fopen("/sdcard/avtest/test.pcm", "wb");
    if( !fp_out ) {
        LOGE("fopen fail. %s", "");
        goto label_end;
    }


//    audio_play_cls = env->GetObjectClass(thiz);
    audio_play_cls = env->FindClass("com/godot/ffmpeg_newbie/player/SimpleAudioPlay");
    create_audio_track_id = env->GetMethodID(audio_play_cls, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    audio_track = env->CallObjectMethod(thiz, create_audio_track_id, out_sample_rate, out_nb_channels);
    LOGD("audio_play_cls: %#x, create_audio_track_id: %#x, audio_track: %#x", audio_play_cls, create_audio_track_id, audio_track);

    audio_track_cls = env->GetObjectClass(audio_track);
    LOGD("audio_track_cls: %#x", audio_track_cls);
    play_id = env->GetMethodID(audio_track_cls, "play", "()V");
    stop_id = env->GetMethodID(audio_track_cls, "stop", "()V");
    release_id = env->GetMethodID(audio_track_cls, "release", "()V");
    write_id = env->GetMethodID(audio_track_cls, "write", "([BII)I");

    env->CallVoidMethod(audio_track, play_id);

    while( av_read_frame(ifmt_ctx, &packet) >= 0 ) {
        LOGD("decode frame: %d", ++frame_count);
        if( packet.stream_index == audio_stream_index ) {
            ret = avcodec_send_packet(decodec_ctx, &packet);
            if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
                continue;
            } else if( ret < 0 ) {
                print_error("avcodec_send_packet", ret);
                goto label_end;
            }
            while( ret >= 0 ) {
                ret = avcodec_receive_frame(decodec_ctx, frame);
                if( AVERROR(EAGAIN) == ret || AVERROR_EOF == ret ) {
                    break;
                } else if( ret < 0 ) {
                    print_error("avcodec_receive_frame", ret);
                    break;
                }
#define USE_BUF_RECEIVE 1
#if USE_BUF_RECEIVE
                // 方法一 格式转换
//                if( (ret=swr_convert(swr_ctx, &out_buf, MAX_BUF_SIZE, (const uint8_t **)(frame->data), frame->nb_samples)) < 0 ) {
                uint8_t* ob_arr[] = {NULL, NULL};
                ob_arr[0] = out_buf;
                if( (ret=swr_convert(swr_ctx, ob_arr, frame->nb_samples, (const uint8_t **)(frame->data), frame->nb_samples)) < 0 ) {
                    print_error("swr_convert", ret);
                    goto label_end;
                }
                int size = av_samples_get_buffer_size(NULL, out_nb_channels, frame->nb_samples, out_sample_fmt, 1);
                if( size < 0 ) {
                    print_error("av_samples_get_buffer_size", ret);
                    goto label_end;
                }
#else
                // 方法二 格式转换 没理解透彻，以后再干！！！
                if( (ret=swr_convert_frame(swr_ctx, out_frame, frame)) < 0 ) {
                    print_error("swr_convert_frame", ret);
                    goto label_end;
                }
#endif
#if USE_BUF_RECEIVE
                // 调用java方法，切结方法入参转为java类型，以及释放局部变量
                jbyteArray jbArr = env->NewByteArray(size);
                env->SetByteArrayRegion(jbArr, 0, size, reinterpret_cast<const jbyte *>(out_buf));
                int s = env->CallIntMethod(audio_track, write_id, jbArr, 0, size);
                LOGD("buf size: %d, write size: %d", size, s);
                env->DeleteLocalRef(jbArr);
                fwrite(out_buf, 1, size, fp_out);
#else
                int data_size = av_get_bytes_per_sample(decodec_ctx->sample_fmt);
                if( data_size < 0 ) {
                    LOGE("number of bytes per sample is unknown. %s", "");
                    goto label_end;
                }
                LOGD("data_size: %d, out_frame->nb_samples: %d, out_frame->channels: %d", data_size, out_frame->nb_samples, out_frame->channels);
                for( int i=0; i<out_frame->nb_samples; i++ ) {
                    for( int j=0; j<out_frame->channels; i++ ) {
                        fwrite(out_frame->data[j] + data_size*i, 1, data_size, fp_out);
                    }
                }
#endif
            }
        }
        av_packet_unref(&packet);
    }
label_end:
    if( audio_track ) {
        env->CallVoidMethod(audio_track, stop_id);
        env->CallVoidMethod(audio_track, release_id);
    }
    if( fp_out ) {
        fclose(fp_out);
    }
    if( out_buf ) {
        av_free(out_buf);
    }
    if( frame ) {
        av_frame_free(&frame);
    }
    if( out_frame ) {
        av_frame_free(&out_frame);
    }
    if( swr_ctx ) {
        swr_free(&swr_ctx);
    }
    if( decodec_ctx ) {
        avcodec_close(decodec_ctx);
        avcodec_free_context(&decodec_ctx);
    }
    if( ifmt_ctx ) {
        avformat_close_input(&ifmt_ctx);
    }
    env->ReleaseStringUTFChars(file_name, filename);
}