//
// Created by nireus on 2018/5/14.
//

#ifndef XPLAY_IPLAYER_H
#define XPLAY_IPLAYER_H


#include <mutex>
#include "XThread.h"
#include "XParameter.h"

class IDemux;
class IDecode;
class IResample;
class IVideoView;
class IAudioPlay;

class IPlayer : public XThread {
public:
    static IPlayer *Get(unsigned char index = 0);
    virtual bool Open(const char *path);
    virtual void Close();
    virtual bool Start();
    virtual bool InitView(void *win);

    bool isHardDecode = true;
    XParameter outPara;

    IDemux *demux = 0;
    IDecode *adecode = 0;
    IDecode *vdecode = 0;
    IResample *resample = 0;
    IVideoView *videoView = 0;
    IAudioPlay *audioPlay = 0;

protected:
    void Main();
    std::mutex muxtex;
    IPlayer(){};
};


#endif //XPLAY_IPLAYER_H
