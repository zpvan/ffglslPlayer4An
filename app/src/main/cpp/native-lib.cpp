#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "IPlayerProxy.h"
#include "FFDecode.h"
#include "XLog.h"

static ANativeWindow *g_window = 0;

extern "C"
JNIEXPORT

jint JNI_OnLoad(JavaVM *vm, void *res) {
    IPlayerProxy::Get()->Init(vm);
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_knox_xplay_XPlay_native_1open(JNIEnv *env, jclass clazz, jstring path) {
    const char *cpath = env->GetStringUTFChars(path, 0);
    bool ret = IPlayerProxy::Get()->Open(cpath);
    env->ReleaseStringUTFChars(path, cpath);
    return ret ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_knox_xplay_XPlay_native_1start(JNIEnv *env, jclass clazz) {
    return IPlayerProxy::Get()->Start() ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1setPause(JNIEnv *env, jclass clazz, jboolean pause) {
    IPlayerProxy::Get()->SetPause(pause == JNI_TRUE);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1seek(JNIEnv *env, jclass clazz, jdouble pos) {
    IPlayerProxy::Get()->Seek(pos);
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_knox_xplay_XPlay_native_1getProgress(JNIEnv *env, jclass clazz) {
    jlongArray result = env->NewLongArray(2);
    if (!result)
        return 0;
    jlong vals[2];
    vals[0] = IPlayerProxy::Get()->GetPlayMs();
    vals[1] = IPlayerProxy::Get()->GetTotalMs();
    env->SetLongArrayRegion(result, 0, 2, vals);
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1initView(JNIEnv *env, jobject instance, jobject surface) {
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
    g_window = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(g_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1closeView(JNIEnv *env, jobject instance) {
    IPlayerProxy::Get()->Close();
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
}
