#include <jni.h>
#include <string>

#include "FFDemux.h"
#include "XLog.h"
#include "FFDecode.h"

#include "android/native_window.h"
#include "android/native_window_jni.h"
#include "XEGL.h"
#include "XShader.h"
#include "IVideoView.h"
#include "GLVideoView.h"
#include "FFResample.h"
#include "IAudioPlay.h"
#include "SLAudioPlay.h"

extern "C" {
#include "libavcodec/jni.h"
}

class TextObs : public IObserver {
public:
    void Update(XData data) {
        XLOGD("TestObs Update data size: %d", data.size);
    }
};

extern "C"
JNIEXPORT

jint JNI_OnLoad(JavaVM *vm, void *res) {
    av_jni_set_java_vm(vm, NULL);
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_knox_xplay_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    IDemux *dmx = new FFDemux();
    dmx->Open("/mnt/sdcard/Download/mm/Jingju_108.mov");
    //for (;;) {
    //    XData xData = dmx->Read();
    //    XLOGD("read packet, size: %d", xData.size);
    //    if (xData.size == 0) {
    //        XLOGD("read the end");
    //        break;
    //    }
    //    xData.Drop();
    //}

    //TextObs *tobs = new TextObs();
    //dmx->AddObs(tobs);

    IDecode *vdec = new FFDecode();
    vdec->Open(dmx->GetVPara());
    dmx->AddObs(vdec);

    IDecode *adec = new FFDecode();
    adec->Open(dmx->GetAPara());
    dmx->AddObs(adec);

    dmx->Start();
    vdec->Start();
    adec->Start();

    //XSleep(300);
    //dmx->Stop();

    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1initView(JNIEnv *env, jobject instance, jobject surface) {

    //ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);
    //XEGL::Get()->Init(nwin);
    //XShader shader;
    //shader.Init();


    IDemux *dmx = new FFDemux();
    dmx->Open("/mnt/sdcard/Download/mm/Jingju_108.mov");

    IDecode *vdec = new FFDecode();
    vdec->Open(dmx->GetVPara(), true);
    dmx->AddObs(vdec);

    IDecode *adec = new FFDecode();
    adec->Open(dmx->GetAPara());
    dmx->AddObs(adec);

    IVideoView *view = new GLVideoView();
    view->SetRender(ANativeWindow_fromSurface(env, surface));
    vdec->AddObs(view);

    IResample *resample = new FFResample();
    XParameter outPara = dmx->GetAPara();
    resample->Open(dmx->GetAPara(), outPara);
    adec->AddObs(resample);

    IAudioPlay *audioPlay = new SLAudioPlay();
    audioPlay->StartPlay(outPara);
    resample->AddObs(audioPlay);

    dmx->Start();
    vdec->Start();
    adec->Start();
}