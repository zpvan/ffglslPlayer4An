#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "XEGL.h"
#include "XShader.h"
#include "IVideoView.h"
#include "GLVideoView.h"
#include "FFResample.h"
#include "IAudioPlay.h"
#include "SLAudioPlay.h"
#include "XLog.h"

extern "C" {
#include "libavcodec/jni.h"
}

static const char *MEDIA_FILE = "/mnt/sdcard/Download/valor-01-01.mp4";

class TextObs : public IObserver {
public:
    void Update(XData data) {
        XLOGD("TestObs Update data size: %d", data.size);
    }
};
#include "IPlayerProxy.h"
#include "FFDecode.h"
#include "FFDemux.h"

extern "C"
JNIEXPORT

jint JNI_OnLoad(JavaVM *vm, void *res) {
    IPlayerProxy::Get()->Init(vm);
    IPlayerProxy::Get()->Open(MEDIA_FILE);
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