//
// Created by nireus on 2018/5/7.
//

#ifndef XPLAY_FFRESAMPLE_H
#define XPLAY_FFRESAMPLE_H


#include "IResample.h"

struct SwrContext;

class FFResample : public IResample {
public:
    virtual bool Open(XParameter in, XParameter out = XParameter());
    virtual void Close();
    virtual XData Resample(XData inData);

private:
    SwrContext *swr_ctx;
    std::mutex mux;
};


#endif //XPLAY_FFRESAMPLE_H
