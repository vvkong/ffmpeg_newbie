#include <string>
#include "newbie.h"
// 大写，^_^
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_godot_ffmpeg_1newbie_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_godot_ffmpeg_1newbie_MainActivity_printMetaData(JNIEnv *env, jclass clazz,
                                                         jstring video_path) {
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    int ret;

    jboolean isCopy = false;
    const char* path = env->GetStringUTFChars(video_path, &isCopy);

    LOGD("video path: %s\n", path);

    if ((ret = avformat_open_input(&fmt_ctx, path, NULL, NULL))) {
        LOGD("avformat_open_input fail, ret: %d", ret);
        return env->NewStringUTF("avformat_open_input fail");
    }


    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        LOGD("avformat_find_stream_info fail, ret: %d", ret);
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return env->NewStringUTF("avformat_find_stream_info fail");
    }

    std::string buf(path);
    buf += "\n";

    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        LOGD("%s=%s\n", tag->key, tag->value);
        buf += tag->key;
        buf += "=";
        buf += tag->value;
        buf += "\n";
    }

    avformat_close_input(&fmt_ctx);

    env->ReleaseStringUTFChars(video_path, path);

    LOGD("%s\n", "printMetaData end");
    return env->NewStringUTF(buf.c_str());
}

