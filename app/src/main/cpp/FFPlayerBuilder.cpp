//
// Created by nireus on 2018/5/16.
//

#include "FFPlayerBuilder.h"

#include "FFDemux.h"
#include "FFDecode.h"
#include "FFResample.h"
#include "GLVideoView.h"
#include "SLAudioPlay.h"

IDemux *FFPlayerBuilder::CreateDemux() {
    return new FFDemux();
}

IDecode *FFPlayerBuilder::CreateDecode() {
    return new FFDecode();
}

IResample *FFPlayerBuilder::CreateResample() {
    return new FFResample();
}

IVideoView *FFPlayerBuilder::CreateVideoView() {
    return new GLVideoView();
}

IAudioPlay *FFPlayerBuilder::CreateAudioPlay() {
    return new SLAudioPlay();
}

IPlayer *FFPlayerBuilder::CreatePlayer(unsigned char index) {
    return IPlayer::Get(index);
}

void FFPlayerBuilder::InitHard(void *vm) {
    FFDecode::InitHard(vm);
}