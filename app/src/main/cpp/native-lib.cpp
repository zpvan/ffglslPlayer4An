#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "IPlayerProxy.h"

extern "C"
JNIEXPORT

jint JNI_OnLoad(JavaVM *vm, void *res) {
    IPlayerProxy::Get()->Init(vm);
    IPlayerProxy::Get()->Open("/mnt/sdcard/Download/mm/Jingju_108.mov");
    IPlayerProxy::Get()->Start();
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_knox_xplay_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}



extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1initView(JNIEnv *env, jobject instance, jobject surface) {
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(nwin);
}