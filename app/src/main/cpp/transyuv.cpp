//
// Created by wangrenxing on 2019-12-10.
//
#include "newbie.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

static bool open_input_file(const char *path) {
    return true;
}

static bool open_output_file(const char *filename) {
    return true;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_godot_ffmpeg_1newbie_MainActivity_trans2yuvv1(JNIEnv *env, jclass clazz, jstring video_src,
                                                     jstring video_dst) {
    jboolean success = JNI_FALSE;
    jboolean isCopy = JNI_FALSE;
    const char* src_path = env->GetStringUTFChars(video_src, &isCopy);
    const char* dst_path = env->GetStringUTFChars(video_dst, &isCopy);

    // 封装格式上下文 输入
    AVFormatContext* ifmt_ctx = avformat_alloc_context();
    // 解码器上下文
    AVCodecContext* codec_ctx = NULL;
    // 像素数据（解码数据）
    AVFrame* frame = NULL;
    // 编码数据
    AVPacket packet = { .data = NULL, .size = 0 };
//    AVPacket* packet = NULL;
    // 像素格式转换或缩放
    struct SwsContext* sws_ctx = NULL;
    // 输出文件
    FILE * fp_yuv;
    // yuv 像素数据
    AVFrame* yuv_frame = NULL;

    uint8_t* out_buf = NULL;
    int frame_count = 0;

    AVStream *stream = NULL;
    AVCodec* dec = NULL;

    int stream_idx;
    int got_pic;

    int ret;
    // 1. 打开输入视频文件
    if( (ret = avformat_open_input(&ifmt_ctx, src_path, NULL, NULL)) < 0 ) {
        LOGE("打开文件失败，path: %s, ret: %d", src_path, ret);
        goto label_end;
    }
    // 2. 获取视频信息
    if( (ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0 ) {
        LOGE("没有找到流信息，path: %s, ret: %d", src_path, ret);
        goto label_end;
    }

    stream_idx = -1;
    // 3. 视频解码，找到视频对应的AVStream所在avfmt_ctx->streams的索引位置
    for( int i=0; i<ifmt_ctx->nb_streams; i++ ) {
        if( ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
            LOGD("找到视频流, %d", i);
            stream_idx = i;
            break;
        }
    }
    if( stream_idx < 0 ) {
        LOGE("没找到视频流%s", "");
        goto label_end;
    }
    // 4. 获取视频解码器
    stream = ifmt_ctx->streams[stream_idx];
    dec = avcodec_find_decoder(stream->codecpar->codec_id);
    if( !dec ) {
        LOGE("查找解码器失败%s", "");
        goto label_end;
    }

    codec_ctx = avcodec_alloc_context3(dec);
    if( !codec_ctx ) {
        LOGE("解码器上下文空间申请失败%s", "");
        goto label_end;
    }
    if( (ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar)) < 0 ) {
        LOGE("拷贝解码参数到解码上下文失败, ret: %d", ret);
        goto label_end;
    }
    if( (ret = avcodec_open2(codec_ctx, dec, NULL)) < 0 ) {
        LOGE("打开解码器失败, ret: %d", ret);
        goto label_end;
    }
    // 打开文件
    fp_yuv = fopen(dst_path, "wb");
    if( !fp_yuv ) {
        LOGE("打开文件失败，file: %s", dst_path);
        goto label_end;
    }
    // yuv 像素数据
    yuv_frame = av_frame_alloc();
    /*out_buf = static_cast<uint8_t *>(av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height, 1)));
    if( av_image_fill_arrays(yuv_frame->data, yuv_frame->linesize, out_buf, AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height, 1) < 0 ) {
        LOGE("av_image_fill_arrays fail. %s", "");
        goto label_end;
    }*/
    av_image_alloc(yuv_frame->data, yuv_frame->linesize, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P, 1);

    LOGD("用于ffplay播放的高宽， w: %d, h: %d", codec_ctx->width, codec_ctx->height);
    LOGD("%s", "ffplay -f rawvideo -video_size 640x480 test.yuv");
    // 像素格式转换或缩放
    sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                                codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    frame_count = 0;

    // 像素数据（解码数据）
    frame = av_frame_alloc();
    while( av_read_frame(ifmt_ctx, &packet) >= 0 ) {
        // 解码AVPacket -> AVFrame
        /*
        if( (ret = avcodec_decode_video2(codec_ctx, frame, &got_pic, packet)) < 0 ) {
            LOGE("avcodec_decode_video2 fail, ret: %d", ret);
            //goto label_end;
            continue;
        }
        if( got_pic ) {
            // frame -> yuvFrame (YUV420P) 转换格式
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
                      yuv_frame->data, yuv_frame->linesize);

            // 向YUV文件保存解码之后的帧数据
            int y_size = codec_ctx->width * codec_ctx->height;
            fwrite(yuv_frame->data[0], 1, y_size, fp_yuv);
            fwrite(yuv_frame->data[1], 1, y_size/4, fp_yuv);
            fwrite(yuv_frame->data[2], 1, y_size/4, fp_yuv);

            //av_frame_free(&frame);
            LOGD("解码%d帧", frame_count++);
        } else {
            LOGE("解码错误, ret: %d", ret);
            goto label_end;
        }
        */
        //av_packet_rescale_ts(&packet, stream->time_base, codec_ctx->time_base);
        // 解码AVPacket -> AVFrame
        if( (ret=avcodec_send_packet(codec_ctx, &packet)) < 0 ) {
            LOGE("avcodec_send_packet fail, ret: %d", ret);
            continue;
            //goto label_end;
        }
        av_packet_unref(&packet);
        while( ret >= 0 ) {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                LOGE("ret == AVERROR(EAGAIN): %d, ret == AVERROR_EOF: %d", ret == AVERROR(EAGAIN),
                     ret == AVERROR_EOF);
                break;
            } else if( ret < 0 ) {
                LOGE("解码错误, ret: %d", ret);
                goto label_end;
            }
            // frame -> yuvFrame (YUV420P) 转换格式
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
                    yuv_frame->data, yuv_frame->linesize);

            LOGD("yuv w: %d, h: %d;  frame w: %d, h: %d;   decodec w: %d, h: %d",
                    yuv_frame->width, yuv_frame->height,
                 frame->width, frame->height,
                 codec_ctx->width, codec_ctx->height);

            // 向YUV文件保存解码之后的帧数据
            int y_size = codec_ctx->width * codec_ctx->height;
            fwrite(yuv_frame->data[0], 1, y_size, fp_yuv);
            fwrite(yuv_frame->data[1], 1, y_size/4, fp_yuv);
            fwrite(yuv_frame->data[2], 1, y_size/4, fp_yuv);

            //av_frame_free(&frame);
            LOGD("解码%d帧", frame_count++);
        }
    }
    success = JNI_TRUE;
label_end:
    if( fp_yuv ) {
        fclose(fp_yuv);
    }
    if( frame ) {
        av_frame_free(&frame);
    }
    if( yuv_frame ) {
        av_frame_free(&yuv_frame);
    }
    if( codec_ctx ) {
        avcodec_close(codec_ctx);
    }
    if( ifmt_ctx ) {
        avformat_close_input(&ifmt_ctx);
        avformat_free_context(ifmt_ctx);
    }
    if( out_buf ) {
        av_free(out_buf);
    }
    env->ReleaseStringUTFChars(video_src, src_path);
    env->ReleaseStringUTFChars(video_dst, dst_path);
    LOGD("success: %d", success);
    return success;
}

static int frame_count = 0;
static bool decode(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket* packet, SwsContext* sws_ctx, AVFrame* yuv_frame, FILE *fp_out) {
    int ret;
    if( (ret=avcodec_send_packet(codec_ctx, packet)) < 0 ) {
        LOGE("avcodec_send_packet fail, ret: %d", ret);
        return false;
    }
//    av_packet_unref(packet);
    while( ret >= 0 ) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if( ret < 0 ) {
            LOGE("解码错误, ret: %d", ret);
            return false;
        }
        // frame -> yuvFrame (YUV420P) 转换格式
        sws_scale(sws_ctx,
                frame->data, frame->linesize, 0, frame->height,
                yuv_frame->data, yuv_frame->linesize);

        // 向YUV文件保存解码之后的帧数据
        int y_size = codec_ctx->width * codec_ctx->height;
        fwrite(yuv_frame->data[0], 1, y_size, fp_out);
        fwrite(yuv_frame->data[1], 1, y_size/4, fp_out);
        fwrite(yuv_frame->data[2], 1, y_size/4, fp_out);

        //av_frame_free(&frame);
        LOGD("解码%d帧", frame_count++);
    }
    return true;
}

#define INBUF_SIZE 4096

void docode(AVCodecContext *pContext, AVFrame *pFrame, FILE *pFile);

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_godot_ffmpeg_1newbie_MainActivity_trans2yuvv2(JNIEnv *env, jclass clazz,
                                                         jstring video_src, jstring video_dst) {

    jboolean success = JNI_FALSE;
    jboolean isCopy = JNI_FALSE;
    int ret;
    const char* src_path = env->GetStringUTFChars(video_src, &isCopy);
    const char* dst_path = env->GetStringUTFChars(video_dst, &isCopy);
    AVStream* vstream = NULL;
    AVCodec* decodec = NULL;
    AVCodecParserContext* parser = NULL;
    AVCodecContext* decodec_ctx = NULL;
    FILE* fp_in = NULL;
    FILE* fp_out = NULL;
    AVFrame* avframe = NULL;
    AVFrame* yuvframe = NULL;
    SwsContext* sws_ctx = NULL;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t data_size;
    AVPacket* packet;

    LOGD("path: %s", src_path);

    AVFormatContext* ifmt_ctx = avformat_alloc_context();
    if( (ret = avformat_open_input(&ifmt_ctx, src_path, NULL, NULL)) < 0 ) {
        LOGE("avformat_open_input fail, ret: %d", ret);
        goto label_end;
    }
    if( (ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0 ) {
        LOGE("没有找到视频信息，path: %s, ret: %d", src_path, ret);
        goto label_end;
    }
    for( int i=0; i<ifmt_ctx->nb_streams; i++ ) {
        if( ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
            LOGD("找到视频流, %d", i);
            vstream = ifmt_ctx->streams[i];
            break;
        }
    }
    if( !vstream ) {
        LOGE("couldn't find video stream, ret: %s", "");
        goto label_end;
    }

    decodec = avcodec_find_decoder(vstream->codecpar->codec_id);
    if( decodec == NULL ) {
        LOGE("couldn't find video decoder, ret: %s", "");
        goto label_end;
    }

    decodec_ctx = avcodec_alloc_context3(decodec);
    if( !decodec_ctx ) {
        LOGE("couldn't allocate video codec context, ret: %s", "");
        goto label_end;
    }
    LOGD("===w: %d, h: %d", vstream->codecpar->width, vstream->codecpar->height);

    if( (ret = avcodec_parameters_to_context(decodec_ctx, vstream->codecpar)) < 0 ) {
        LOGE("拷贝解码参数到解码上下文失败, ret: %d", ret);
        goto label_end;
    }
    if( (ret = avcodec_open2(decodec_ctx, decodec, NULL)) < 0 ) {
        LOGE("avformat_open_input fail, ret: %d", ret);
        goto label_end;
    }
    fp_in = fopen(src_path, "rb");
    if( !fp_in ) {
        LOGE("couldn't open file,  %s", src_path);
        goto label_end;
    }
    fp_out = fopen(dst_path, "wb");
    if( !fp_out) {
        LOGE("couldn't open file,  %s", dst_path);
        goto label_end;
    }

    avframe = av_frame_alloc();
    if( !avframe ) {
        LOGE("couldn't alloc frame,  %s", "");
        goto label_end;
    }
    yuvframe = av_frame_alloc();
    if( !yuvframe ) {
        LOGE("couldn't alloc frame,  %s", "");
        goto label_end;
    }
    LOGD("用于ffplay播放的高宽， w: %d, h: %d", decodec_ctx->width, decodec_ctx->height);
    LOGD("%s", "ffplay -f rawvideo -video_size widthxheigth test.yuv");

    if( (ret=av_image_alloc(yuvframe->data, yuvframe->linesize, decodec_ctx->width, decodec_ctx->height, AV_PIX_FMT_YUV420P, 1)) < 0 ) {
        LOGE("av_image_alloc fail,  %d", ret);
        goto label_end;
    }

    // 像素格式转换或缩放
    sws_ctx = sws_getContext(decodec_ctx->width, decodec_ctx->height, decodec_ctx->pix_fmt,
                             decodec_ctx->width, decodec_ctx->height, AV_PIX_FMT_YUV420P,
                             SWS_FAST_BILINEAR, NULL, NULL, NULL);

    parser = av_parser_init(decodec->id);
    if( parser == NULL ) {
        LOGE("couldn't find video decoder, ret: %s", "");
        goto label_end;
    }

    packet = av_packet_alloc();
    if( !packet ) {
        LOGE("couldn't alloc packet,  %s", "");
        goto label_end;
    }
    frame_count = 0;
    while( !feof(fp_in) ) {
        data_size = fread(inbuf, 1, INBUF_SIZE, fp_in);
        if( data_size <= 0 ) {
            break;
        }
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, decodec_ctx, &packet->data, &packet->size,
                    data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if( ret < 0 ) {
                LOGE("av_parser_parse2 fail,  ret: %d", ret);
                goto label_end;
            }
            data += ret;
            data_size -= ret;
            if( packet->size ) {
                decode(decodec_ctx, avframe, packet, sws_ctx, yuvframe, fp_out);
            }
        }
    }
    decode(decodec_ctx, avframe, NULL, sws_ctx, yuvframe, fp_out);

label_end:
    if( ifmt_ctx ) {
        avformat_close_input(&ifmt_ctx);
    }
    if( parser ) {
        av_parser_close(parser);
    }
    if( packet ) {
        av_packet_free(&packet);
    }
    if( avframe ) {
        av_frame_free(&avframe);
    }
    if( decodec_ctx ) {
        avcodec_close(decodec_ctx);
        avcodec_free_context(&decodec_ctx);
    }
    if( fp_in ) {
        fclose(fp_in);
    }
    if( fp_out) {
        fclose(fp_out);
    }

    env->ReleaseStringUTFChars(video_src, src_path);
    env->ReleaseStringUTFChars(video_dst, dst_path);
    LOGD("success: %d", success);
    return success;
}
