//
// Created by wangrenxing on 2019-12-11.
//
#include "newbie.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libyuv.h"
}
extern "C"
JNIEXPORT void JNICALL
Java_com_godot_ffmpeg_1newbie_player_SimplePlayer_render(JNIEnv *env, jobject thiz,
        jstring file_name, jobject surface) {
    int ret;
    AVFormatContext* ifmt_ctx;
    AVCodec* decodec = NULL;
    AVCodecContext* decodec_ctx;
    AVPacket packet = {.data = NULL, .size = 0};
    AVFrame* frame = NULL;
    AVFrame* yuv_frame = NULL;
    AVFrame* rgb_frame = NULL;
    SwsContext* sws_rgb_ctx = NULL;
    SwsContext* sws_yuv_ctx = NULL;
    FILE* fp_yuv = NULL;

    int window_width;
    int window_height;
    int adj_video_width;
    int adj_video_height;
    ANativeWindow* native_window;
    ANativeWindow_Buffer out_buffer;
    int video_stream_idx = -1;
    int frame_count = 0;
    jboolean isCopy = JNI_FALSE;
    const char* filename = env->GetStringUTFChars(file_name, &isCopy);

    LOGD("path: %s", filename);
    ifmt_ctx = avformat_alloc_context();
    if( (ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0 ) {
        LOGE("avformat_open_input fail, ret: %d", ret);
        goto label_end;
    }
    if( avformat_find_stream_info(ifmt_ctx, NULL) < 0 ) {
        LOGE("no stream info %s", "");
        goto label_end;
    }
    for( int i=0; i<ifmt_ctx->nb_streams; i++ ) {
        if( ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
            video_stream_idx = i;
            break;
        }
    }
    if( video_stream_idx == -1 ) {
        LOGE("not find video stream %s", "");
        goto label_end;
    }
    decodec = avcodec_find_decoder(ifmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if( !decodec ) {
        LOGE("not find decoder %s", "");
        goto label_end;
    }
    decodec_ctx = avcodec_alloc_context3(decodec);
    if( !decodec_ctx ) {
        LOGE("avcodec_alloc_context3 fail %s", "");
        goto label_end;
    }
    if( (ret=avcodec_parameters_to_context(decodec_ctx, ifmt_ctx->streams[video_stream_idx]->codecpar)) < 0 ) {
        LOGE("avcodec_parameters_to_context fail. ret: %d", ret);
        goto label_end;
    }

    if( (ret = avcodec_open2(decodec_ctx, decodec, NULL)) < 0 ) {
        LOGE("avcodec_open2 fail. ret: %d", ret);
        goto label_end;
    }

    // 注意surfaceview创建并确保surface已经创建成功
    native_window = ANativeWindow_fromSurface(env, surface);
    if( !native_window ) {
        LOGD("ANativeWindow_fromSurface return null. surface:  %d", (!!surface));
        goto label_end;
    }
    // 窗口高宽
    window_width = ANativeWindow_getWidth(native_window);
    window_height = ANativeWindow_getHeight(native_window);
    // 根据画布大小调整视频高宽
    if( window_height > 0 && window_width > 0 ) {
        double w_ap = (double)window_width / (double)window_height;
        double v_ap = (double)decodec_ctx->width / (double)decodec_ctx->height;
        LOGD("w_ap: %f, v_ap: %f", w_ap, v_ap);
        if(  w_ap > v_ap ) {
            adj_video_height = window_height;
            adj_video_width = (decodec_ctx->width * adj_video_height / decodec_ctx->height);
        } else {
            adj_video_width = window_width;
            adj_video_height = (decodec_ctx->height * adj_video_width / decodec_ctx->width);
        }
    }
    LOGD("window_width: %d, window_height: %d, adj_w: %d, adj_h: %d", window_width, window_height, adj_video_width, adj_video_height);

    // 设置窗体缓冲区，宽、高、像素格式，注意与surface像素格式一致
    // 变形就是与surface的高、宽不一致导致的
    ANativeWindow_setBuffersGeometry(native_window, window_width, window_height, WINDOW_FORMAT_RGBA_8888);

    // 可以直接采用AV_PIX_FMT_ARGB 么？待测试
    // 解码出来一定是YUV420格式的帧么？能指定不？待测
    frame = av_frame_alloc();
    yuv_frame = av_frame_alloc();
    if( av_image_alloc(yuv_frame->data, yuv_frame->linesize, adj_video_width, adj_video_height, AV_PIX_FMT_YUV420P, 1) < 0 ) {
        LOGE("av_image_alloc fail for yuv frame. %s", "");
        goto label_end;
    }
    // 可以看出解码上下文中的格式是否为YUV420P
    LOGD("decodec_ctx->pix_fmt: %d, AV_PIX_FMT_YUV420P: %d", decodec_ctx->pix_fmt, AV_PIX_FMT_YUV420P);

    rgb_frame = av_frame_alloc();
    // rgb 缓冲区自己申请，共享没搞定？有办法么？
    av_image_alloc(rgb_frame->data, rgb_frame->linesize, adj_video_width, adj_video_height, AV_PIX_FMT_RGBA, 1);

    // 解码后的Frame->yuv frame
    sws_yuv_ctx = sws_getContext(decodec_ctx->width, decodec_ctx->height, decodec_ctx->pix_fmt,
                             adj_video_width, adj_video_height, AV_PIX_FMT_YUV420P,
                             SWS_BICUBIC, NULL, NULL, NULL);
    if( !sws_yuv_ctx ) {
        LOGE("sws_getContext fail. %s", "");
    }

    sws_rgb_ctx = sws_getContext(decodec_ctx->width, decodec_ctx->height, decodec_ctx->pix_fmt,
                                 adj_video_width, adj_video_height, AV_PIX_FMT_RGBA,
                                 SWS_BICUBIC, NULL, NULL, NULL);

    if( !sws_rgb_ctx ) {
        LOGE("sws_getContext fail. %s", "");
    }

    fp_yuv = fopen("/sdcard/avtest/ttt.yuv", "wb");
    if( !fp_yuv ) {
        LOGE("fopen(\"/sdcard/avtest/ttt.yuv\" fail. %s", "");
    }

    while( av_read_frame(ifmt_ctx, &packet) >= 0 ) {
        //LOGD("packet.stream_index: %d, video_stream_idx: %d", packet.stream_index, video_stream_idx);
        if( video_stream_idx == packet.stream_index) {
            if( (ret = avcodec_send_packet(decodec_ctx, &packet)) < 0 ) {
                LOGE("avcodec_send_packet fail, ret: %d", ret);
                continue;
            }
            while( ret >= 0 ) {
                ret = avcodec_receive_frame(decodec_ctx, frame);
                LOGD("decode frame: %d", ++frame_count);
                if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) {
                    LOGE("ret == AVERROR(EAGAIN): %d, ret == AVERROR_EOF: %d", ret == AVERROR(EAGAIN), ret == AVERROR_EOF);
                    break;
                } else if( ret < 0 ) {
                    LOGE("avcodec_send_packet fail. ret: %d", ret);
                    goto label_end;
                }

                ANativeWindow_acquire(native_window);
                // 缓冲区 -> window -> surface
                ANativeWindow_lock(native_window, &out_buffer, NULL);
                //LOGD("out buffer, w: %d, h: %d, fmt: %d", out_buffer.width, out_buffer.height, out_buffer.format);

                // rgb_frame像素区与窗体缓冲区一致，无法复用一个缓冲区？要什么操作呢？
                //av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, (uint8_t*)out_buffer.bits, AV_PIX_FMT_RGBA, adj_video_width, adj_video_height, 1);

                if( sws_scale(sws_yuv_ctx, frame->data, frame->linesize, 0, frame->height,
                              yuv_frame->data, yuv_frame->linesize) < 0 ) {
                    LOGE("sws_scale fail, ret: %s", "");
                    break;
                }
                // 方法一 ffmpeg直接转 或 方法二 libyuv 变换

#define USE_LIB_YUV 1
#if USE_LIB_YUV
                // 方法一 ffmpeg直接转
                if( sws_scale(sws_rgb_ctx, frame->data, frame->linesize, 0, frame->height,
                              rgb_frame->data, rgb_frame->linesize) < 0 ) {
                    LOGE("sws_scale fail, ret: %s", "");
                    break;
                }
#else
                // 方法二 libyuv 变换
                // 参数梗
                // YUV420P，Y，U，V三个分量都是平面格式，分为I420和YV12
                // I420格式和YV12格式的不同处在U平面和V平面的位置不同
                // 在I420格式中，U平面紧跟在Y平面之后，然后才是V平面（即：YUV）；但YV12则是相反（即：YVU）。
                libyuv::I420ToARGB(
                        yuv_frame->data[0], yuv_frame->linesize[0],
                        yuv_frame->data[2], yuv_frame->linesize[2],
                        yuv_frame->data[1], yuv_frame->linesize[1],
                        rgb_frame->data[0], rgb_frame->linesize[0],
                        adj_video_width, adj_video_height);
#endif
                // 拷贝rgba frame的像素值到窗口缓冲区上，刷新即可展示
                uint8_t *dst = (uint8_t *) out_buffer.bits;
                //解码后的像素数据首地址
                //由于使用的是RGBA格式，所以解码图像数据只保存在data[0]中。但如果是YUV就会有data[0],data[1],data[2]
                uint8_t *src = rgb_frame->data[0];
                //获取一行字节数
                int oneLineByte = out_buffer.stride * 4;
                LOGD("out_buffer.stride: %d, adj_video_height: %d", out_buffer.stride, adj_video_height);
                //复制一行内存的实际数量
                int srcStride = rgb_frame->linesize[0];
//                for (int i = 0; i < decodec_ctx->height; i++) {
                for (int i = 0; i < adj_video_height; i++) {
                    memcpy(dst + i * oneLineByte, src + i * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(native_window);

                ANativeWindow_release(native_window);

                if( fp_yuv ) {
                    // 向YUV文件保存解码之后的帧数据
                    int y_size = decodec_ctx->width * decodec_ctx->height;
                    fwrite(yuv_frame->data[0], 1, y_size, fp_yuv);
                    fwrite(yuv_frame->data[1], 1, y_size/4, fp_yuv);
                    fwrite(yuv_frame->data[2], 1, y_size/4, fp_yuv);
                }
                usleep(16*1000);
            }
        }
        av_packet_unref(&packet);
    }

label_end:
    if( fp_yuv ) {
        fclose(fp_yuv);
    }
    if( sws_yuv_ctx ) {
        sws_freeContext(sws_yuv_ctx);
    }
    if( sws_rgb_ctx ) {
        sws_freeContext(sws_rgb_ctx);
    }
    if( native_window ) {
        ANativeWindow_release(native_window);
    }
    if( yuv_frame ) {
        av_frame_free(&yuv_frame);
    }
    if( frame ) {
        av_frame_free(&frame);
    }
    if( rgb_frame ) {
        av_frame_free(&rgb_frame);
    }
    if( ifmt_ctx ) {
        avformat_close_input(&ifmt_ctx);
    }
    if( decodec_ctx ) {
        avcodec_close(decodec_ctx);
        avcodec_free_context(&decodec_ctx);
    }
    env->ReleaseStringUTFChars(file_name, filename);
}