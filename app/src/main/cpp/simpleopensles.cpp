//
// Created by wangrenxing on 2019-12-20.
//

#include "newbie.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// 谷歌官网入门：https://developer.android.google.cn/ndk/guides/audio/opensl/getting-started
// 查阅 OpenSL_ES_Specification_1.0.1.pdf
// 谷歌的ndk-samples-master工程中native-audio工程

extern "C"
JNIEXPORT void JNICALL
Java_com_godot_ffmpeg_1newbie_player_OpenSLESPlay_play(JNIEnv *env, jclass clazz, jstring path) {
    // TODO: implement play()
}