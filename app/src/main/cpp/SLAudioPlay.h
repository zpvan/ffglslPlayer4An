//
// Created by nireus on 2018/5/7.
//

#ifndef XPLAY_SLAUDIOPLAY_H
#define XPLAY_SLAUDIOPLAY_H


#include "IAudioPlay.h"

class SLAudioPlay : public IAudioPlay {
public:
    virtual bool StartPlay(XParameter out);
    virtual void SetPause(bool isPause);
    void PlayCall(void *bufq);
    virtual void Close();

    SLAudioPlay();
    virtual ~SLAudioPlay();
protected:
    unsigned char *buf = 0;
    std::mutex sl_mux;
};


#endif //XPLAY_SLAUDIOPLAY_H
