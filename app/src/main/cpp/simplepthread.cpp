//
// Created by wangrenxing on 2019-12-17.
//
#include <jni.h>
#include <pthread.h>
#include "newbie.h"

static JavaVM* g_jvm;

static jobject g_ref_class_loader;
static jmethodID g_load_class_mid;

JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved) {
    LOGD("JNI_OnLoad %s", "");
    g_jvm = jvm;

    JNIEnv* env = NULL;
    if( JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) ) {
        LOGE("jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) fail. %s", "");
        return JNI_VERSION_1_2;
    }

    // 主线程加载，能找到自定义类，
    // 找到对应类classloader，生成全局引用
    // 找出系统类ClassLoader的loadClass方法ID
    jclass class_loader_cls = env->FindClass("java/lang/ClassLoader");
    jclass adapt_class_loader_cls = env->FindClass("com/godot/util/UUIDUtils");
    if( adapt_class_loader_cls ) {
        jmethodID get_cls_loader_mid = env->GetStaticMethodID(adapt_class_loader_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject cls_loader_obj = env->CallStaticObjectMethod(adapt_class_loader_cls, get_cls_loader_mid);
        g_ref_class_loader = env->NewGlobalRef(cls_loader_obj);
        g_load_class_mid = env->GetMethodID(class_loader_cls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    }
    return JNI_VERSION_1_4;
}

static jobject g_ref_uuid_cls = NULL;

// JNIEnv线程独立 java在jni层的封装
// JavaVM全局共享 虚拟机在jni层封装
// 通过AttachCurrentThread附加到虚拟机的线程在查找类时只会通过系统类加载器进行查找，不会通过应用类加载器进行查找，
// 因此可以加载系统类，但是不能加载非系统类，如自己在java层定义的类会返回NULL
/*
 * 方法一：获取classLoader，通过调用classLoader的loadClass来加载自定义类。适合自定义类比较多的情况。利用可以调系统类来实现。
 * 方法二：在主线程创建一个全局的自定义类引用。适合自定义类比较少的情况
 */

static void* p_routine(void* args) {
    LOGD("p_routine, args: %d", (int)args);
    JNIEnv* env = NULL;
    if( JNI_OK == g_jvm->AttachCurrentThread(&env, NULL) ) {
        LOGE("AttachCurrentThread success %s", "");
    } else {
        LOGE("AttachCurrentThread fail %s", "");
        pthread_exit((void*)1);
    }

    /*
    // 方法一
    jstring cls_name = env->NewStringUTF("com/godot/util/UUIDUtils");
    jclass uuid_cls = static_cast<jclass>(env->CallObjectMethod(g_ref_class_loader,
                                                                g_load_class_mid, cls_name));
    // 构造方法名<init>，签名javap -s 全类名 导出
    // env->NewObject()
    LOGD("uuid_cls: %d", !!uuid_cls);
    jmethodID getuuid_mid = env->GetStaticMethodID(uuid_cls, "getUUID", "()Ljava/lang/String;");
    LOGD("getuuid_mid: %d", !!getuuid_mid);
     // =============== 结束分割线 ===============
    */
    // 方法二
    jclass uuid_cls = static_cast<jclass>(g_ref_uuid_cls);
    jmethodID getuuid_mid = env->GetStaticMethodID(uuid_cls, "getUUID", "()Ljava/lang/String;");
    // =============== 结束分割线 ===============

    jboolean isCopy = JNI_FALSE;
    for( int i=0; i<10; i++ ) {
        jstring uuid = static_cast<jstring>(env->CallStaticObjectMethod(uuid_cls, getuuid_mid));
        const char* c_uuid = env->GetStringUTFChars(uuid, &isCopy);
        LOGD("i: %d, uuid: %s", i, c_uuid);
        env->ReleaseStringUTFChars(uuid, c_uuid);
    }
    if( g_ref_uuid_cls ) {
        env->DeleteGlobalRef(g_ref_uuid_cls);
        g_ref_uuid_cls = NULL;
    }
    g_jvm->DetachCurrentThread();
    pthread_exit((void*)0);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_godot_ffmpeg_1newbie_MainActivity_testPthread(JNIEnv *env, jclass clazz) {
    jclass cls = env->FindClass("com/godot/util/UUIDUtils");
    g_ref_uuid_cls = env->NewGlobalRef(cls);

    pthread_t p_id;
    pthread_create(&p_id, NULL, p_routine, (void*)1);

}
