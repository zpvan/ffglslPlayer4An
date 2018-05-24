//
// Created by nireus on 2018/5/16.
//

#include "IPlayerBuilder.h"
#include "IDemux.h"
#include "IDecode.h"
#include "IResample.h"
#include "IAudioPlay.h"
#include "IVideoView.h"

IPlayer *IPlayerBuilder::BuilderPlayer(unsigned char index) {
    IPlayer *player = CreatePlayer(index);

    IDemux *dmx = CreateDemux();

    IDecode *vdec = CreateDecode();
    dmx->AddObs(vdec);

    IDecode *adec = CreateDecode();
    dmx->AddObs(adec);

    IVideoView *view = CreateVideoView();
    vdec->AddObs(view);

    IResample *resample = CreateResample();
    adec->AddObs(resample);

    IAudioPlay *audioPlay = CreateAudioPlay();
    resample->AddObs(audioPlay);

    player->demux = dmx;
    player->adecode = adec;
    player->vdecode = vdec;
    player->videoView = view;
    player->resample = resample;
    player->audioPlay = audioPlay;

    return player;
}