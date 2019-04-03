//
// Created by nireus on 2018/5/7.
//

#include "SLAudioPlay.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "XLog.h"

static SLObjectItf engineObject = NULL;
static SLEngineItf engineItf = NULL;
static SLAndroidSimpleBufferQueueItf pcmQueue = NULL;
static SLObjectItf audioPlayer = NULL;
static SLPlayItf audioPlayerItf = NULL;
static SLObjectItf outputMixObject = NULL;

static SLresult createSLEngine(SLObjectItf *p_engineObject, SLEngineItf *p_engineEngine) {
    SLObjectItf engineObject = *p_engineObject;
    SLEngineItf engineEngine = *p_engineEngine;

    SLresult slresult = 0;
    slresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("slCreateEngine failed");
        return slresult;
    }
    slresult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("slCreateEngine Realize failed");
        return slresult;
    }
    slresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("slCreateEngine GetInterface failed");
        return slresult;
    }
    *p_engineObject = engineObject;
    *p_engineEngine = engineEngine;
    return slresult;
}

static SLresult createSLMix(SLObjectItf *p_outputMixObject, SLEngineItf engine) {
    SLObjectItf outputMixObject = *p_outputMixObject;
    SLEngineItf engineEngine = engine;

    SLresult slresult = 0;
    slresult = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("CreateOutputMix failed");
        return slresult;
    }
    slresult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("CreateOutputMix Realize failed");
        return slresult;
    }
    *p_outputMixObject = outputMixObject;
    return slresult;
}

static SLresult
createSLPlayer(SLObjectItf *p_audioPlayer, SLEngineItf engine, SLDataSource slDataSource,
               SLDataSink audioSink, SLPlayItf *p_audioPlayerItf) {
    SLObjectItf audioPlayer = *p_audioPlayer;
    SLEngineItf engineEngine = engine;
    SLPlayItf audioPlayerItf = *p_audioPlayerItf;

    SLresult slresult = 0;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    slresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &audioPlayer, &slDataSource,
                                                  &audioSink,
                                                  sizeof(ids) / sizeof(SLInterfaceID), ids, req);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("CreateAudioPlayer failed");
        return slresult;
    }
    slresult = (*audioPlayer)->Realize(audioPlayer, SL_BOOLEAN_FALSE);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("CreateAudioPlayer Realize failed");
        return slresult;
    }
    slresult = (*audioPlayer)->GetInterface(audioPlayer, SL_IID_PLAY, &audioPlayerItf);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("CreateAudioPlayer GetInterface failed");
        return slresult;
    }
    *p_audioPlayer = audioPlayer;
    *p_audioPlayerItf = audioPlayerItf;
    return slresult;
}

static void pcmCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    SLAudioPlay *ap = (SLAudioPlay *) context;
    if (!ap) {
        XLOGE("pcmCallBack context is null");
        return;
    }
    ap->PlayCall((void *) bf);
}

SLAudioPlay::SLAudioPlay() {
    buf = new unsigned char[1024 * 1024];
}

SLAudioPlay::~SLAudioPlay() {
    delete (buf);
    buf = NULL;
}

void SLAudioPlay::PlayCall(void *bufq) {
    if (!bufq)
        return;

    SLAndroidSimpleBufferQueueItf bf = (SLAndroidSimpleBufferQueueItf) bufq;
    //XLOGD("SLAudioPlay::PlayCall");
    //阻塞函数
    XData d = GetData();
    if (d.size <= 0) {
        XLOGE("GetData size %d <= 0", d.size);
        return;
    }
    if (!buf) {
        return;
    }
    memcpy(buf, d.data, (size_t) d.size);
    sl_mux.lock();
    (*bf)->Enqueue(bf, buf, (size_t) d.size);
    d.Drop();
    sl_mux.unlock();
}

void SLAudioPlay::Close() {

    IAudioPlay::Clear();

    sl_mux.lock();
    // 停止播放
    if (audioPlayerItf && (*audioPlayerItf)) {
        (*audioPlayerItf)->SetPlayState(audioPlayerItf, SL_PLAYSTATE_STOPPED);
    }
    // 清理播放队列
    if (pcmQueue && (*pcmQueue)) {
        (*pcmQueue)->Clear(pcmQueue);
    }
    // 销毁队列
    if (audioPlayer && (*audioPlayer)) {
        (*audioPlayer)->Destroy(audioPlayer);
    }
    // 销毁混音器
    if (outputMixObject && (*outputMixObject)) {
        (*outputMixObject)->Destroy(outputMixObject);
    }
    // 销毁播放引擎
    if (engineObject && (*engineObject)) {
        (*engineObject)->Destroy(engineObject);
    }
    sl_mux.unlock();
}

bool SLAudioPlay::StartPlay(XParameter out) {

    Close();

    sl_mux.lock();
    // 1
    SLresult slresult = 0;
    slresult = createSLEngine(&engineObject, &engineItf);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("createSL failed");
        sl_mux.unlock();
        return false;
    }
    XLOGD("createSL success");

    // 2
    //
    slresult = createSLMix(&outputMixObject, engineItf);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("createMix failed");
        sl_mux.unlock();
        return false;
    }
    XLOGD("createMix success");

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSink = {&outputMix, NULL};

    // 3
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM, //播放pcm格式的数据
            (SLuint32) out.channels, //2个声道(立体声)
            (SLuint32) out.sample_rate * 1000, // 44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16, // 位数16位
            SL_PCMSAMPLEFORMAT_FIXED_16, // 和位数一样
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, // 立体声(前左前右)
            SL_BYTEORDER_LITTLEENDIAN //小端
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    // 4
    slresult = createSLPlayer(&audioPlayer, engineItf, slDataSource, audioSink,
                              &audioPlayerItf);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("createAudioPlayer failed");
        sl_mux.unlock();
        return false;
    }
    XLOGD("createAudioPlayer success %p %p", audioPlayer, audioPlayerItf);

    // 5
    slresult = (*audioPlayer)->GetInterface(audioPlayer, SL_IID_BUFFERQUEUE, &pcmQueue);
    if (slresult != SL_RESULT_SUCCESS) {
        XLOGE("create pcm queue failed");
        sl_mux.unlock();
        return false;
    }
    XLOGD("create pcm queue success %p", pcmQueue);

    // 6 设置回调函数， 播放队列空时调用
    (*pcmQueue)->RegisterCallback(pcmQueue, pcmCallBack, this);

    // 7 设置为播放状态
    (*audioPlayerItf)->SetPlayState(audioPlayerItf, SL_PLAYSTATE_PLAYING);

    // 8 启动队列回调
    (*pcmQueue)->Enqueue(pcmQueue, "", 1);

    XLOGD("SLAudioPlay::StartPlay success");
    sl_mux.unlock();
    return true;
}