//
// Created by nireus on 2018/5/1.
//

#ifndef XPLAY_FFDEMUX_H
#define XPLAY_FFDEMUX_H

#include "IDemux.h"

struct AVFormatContext;

class FFDemux : public IDemux {
public:
    //打开文件或者流媒体, rtmp, http, rtsp
    virtual bool Open(const char *url);

    //获取视频参数
    virtual XParameter GetVPara();

    //获取音频参数
    virtual XParameter GetAPara();

    //读取一帧数据, 数据由调用者清理
    virtual XData Read();

    //初始化ffmpeg
    FFDemux();

private:
    AVFormatContext *av_fmt_ctx;
    int audioStream;
    int videoStream;
};


#endif //XPLAY_FFDEMUX_H
