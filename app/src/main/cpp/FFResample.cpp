//
// Created by nireus on 2018/5/7.
//


#include "FFResample.h"
#include "XLog.h"

extern "C" {
#include "libswresample/swresample.h"
#include <libavcodec/avcodec.h>
}

void FFResample::Close() {
    mux.lock();
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    mux.unlock();
}

bool FFResample::Open(XParameter in, XParameter out) {

    Close();

    mux.lock();
    swr_ctx = swr_alloc();
    swr_ctx = swr_alloc_set_opts(swr_ctx,
            //输出参数
                                 av_get_default_channel_layout(out.channels),
                                 AV_SAMPLE_FMT_S16, //主要是改变sampleFormat
                                 out.sample_rate,
            //输入参数
                                 av_get_default_channel_layout(
                                         in.para->channels),
                                 (AVSampleFormat) in.para->format,
                                 in.para->sample_rate,
                                 0, NULL);
    int res = swr_init(swr_ctx);
    uint8_t *pcm = new uint8_t[4800 * 4 * 2];

    if (res != 0) {
        XLOGE("swr_init failed!");
        mux.unlock();
        return false;
    }

    outChannels = in.para->channels;
    outFormat = AV_SAMPLE_FMT_S16;
    XLOGD("swr_init success!");
    mux.unlock();
    return true;
}

XData FFResample::Resample(XData inData) {
    //XLOGE("indata size: %d", inData.size);

    if (inData.size <= 0 || !inData.data)
        return XData();

    mux.lock();
    if (!swr_ctx) {
        mux.unlock();
        return XData();
    }

    XData out;
    AVFrame *frame = (AVFrame *) inData.data;
    //size = 通道数 * 单通道样本数 * 样本字节大小
    int outSize =
            outChannels * frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat) outFormat);
    if (outSize <= 0) {
        mux.unlock();
        return XData();
    }
    out.Alloc(outSize);
    uint8_t *outArr[2] = {0};
    outArr[0] = out.data;
    int len = swr_convert(swr_ctx, outArr, frame->nb_samples, (const uint8_t **) frame->data,
                          frame->nb_samples);
    if (len <= 0) {
        out.Drop();
        mux.unlock();
        return XData();
    }
    out.pts = inData.pts;
    //XLOGD("swr_convert success size: %d", len);

    mux.unlock();
    return out;
}