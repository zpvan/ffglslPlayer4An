//
// Created by nireus on 2018/5/5.
//

#ifndef XPLAY_FFDECODE_H
#define XPLAY_FFDECODE_H


#include "IDecode.h"

struct AVCodecContext;
struct AVFrame;

class FFDecode : public IDecode {
public:
    static void InitHard(void *vm);

    virtual bool Open(XParameter para, bool isHard = false);
    virtual void Close();

    FFDecode();

    //Future模型, 发送数据到线程解码
    virtual bool SendPacket(XData packet);

    //从线程中获取解码结果
    virtual XData RecvFrame();

private:
    AVCodecContext *av_cdc_ctx;
    AVFrame *frame;
    std::mutex mux;
};


#endif //XPLAY_FFDECODE_H
