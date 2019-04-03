//
// Created by nireus on 2018/5/14.
//

#include "IPlayer.h"

#include "IDemux.h"
#include "IDecode.h"
#include "IVideoView.h"
#include "IResample.h"
#include "IAudioPlay.h"
#include "XLog.h"

IPlayer *IPlayer::Get(unsigned char index) {
    static IPlayer p[256];
    return &p[index];
}

void IPlayer::Main() {
    while (!isExit) {
        muxtex.lock();
        if (!audioPlay || !vdecode) {
            muxtex.unlock();
            XSleep(2);
            continue;
        }
        // 获取音频的pts, 告诉视频
        int apts = audioPlay->pts;
        //XLOGE("audio pts: %d", audioPlay->pts);
        vdecode->syncPts = apts;
        muxtex.unlock();
        XSleep(2);
    }
}

void IPlayer::Close() {
    muxtex.lock();
    // 先关闭主题线程, 再清理观察者
    // 同步线程
    XThread::Stop();
    // 解封装线程
    if (demux)
        demux->Stop();
    // 解码线程
    if (vdecode)
        vdecode->Stop();
    if (adecode)
        adecode->Stop();

    // 清理缓冲队列
    if (vdecode)
        vdecode->Clear();
    if (adecode)
        adecode->Clear();
    if (audioPlay)
        audioPlay->Clear();

    // 清理资源
    if (audioPlay)
        audioPlay->Close();
    if (videoView)
        videoView->Close();
    if (vdecode)
        vdecode->Close();
    if (adecode)
        adecode->Close();
    if (demux)
        demux->Close();
    muxtex.unlock();
}

bool IPlayer::Open(const char *path) {

    Close();

    muxtex.lock();
    if (!demux || !demux->Open(path)) {
        XLOGE("demux->Open %s failed!", path);
        muxtex.unlock();
        return false;
    }
    if (!vdecode || !vdecode->Open(demux->GetVPara(), isHardDecode)) {
        XLOGE("vdecode->Open %s failed!", path);
    }
    if (!adecode || !adecode->Open(demux->GetAPara())) {
        XLOGE("adecode->Open %s failed!", path);
    }

    if (outPara.sample_rate <= 0)
        outPara = demux->GetAPara();
    if (!resample || !resample->Open(demux->GetAPara(), outPara)) {
        XLOGE("resample->Open %s failed!", path);
    }
    muxtex.unlock();
    return true;
}

bool IPlayer::Start() {
    muxtex.lock();

    if (audioPlay) {
        audioPlay->StartPlay(outPara);
    }

    if (adecode) {
        adecode->Start();
    }

    if (vdecode) {
        vdecode->Start();
    }

    if (!demux || !demux->Start()) {
        XLOGE("demux->Start failed!");
        muxtex.unlock();
        return false;
    }
    
    XThread::Start();

    muxtex.unlock();
    return true;
}

bool IPlayer::InitView(void *win) {
    if (videoView)
        videoView->SetRender(win);

    return true;
}