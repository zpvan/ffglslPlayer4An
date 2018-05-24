//
// Created by nireus on 2018/5/1.
//

extern "C" {
#include "libavformat/avformat.h"
}

#include "FFDemux.h"
#include "XLog.h"

//打开文件或者流媒体, rtmp, http, rtsp
bool FFDemux::Open(const char *url) {

    Close();
    ff_dmx_mutex.lock();
    int res = 0;
    res = avformat_open_input(&av_fmt_ctx, url, NULL, NULL);
    if (res != 0) {
        ff_dmx_mutex.unlock();
        XLOGE("open %s failed", url);
        return false;
    }
    XLOGD("open %s success", url);

    res = avformat_find_stream_info(av_fmt_ctx, NULL);
    if (res < 0) {
        XLOGE("parse %s failed", url);
    }

    durationMs = av_fmt_ctx->duration / (AV_TIME_BASE / 1000);
    ff_dmx_mutex.unlock();
    XLOGD("parse %s success, duration: %lld ms", url, durationMs);
    GetVPara();
    GetAPara();

    return true;
}

void FFDemux::Close() {
    ff_dmx_mutex.lock();
    if (av_fmt_ctx) {
        avformat_close_input(&av_fmt_ctx);
    }
    ff_dmx_mutex.unlock();
}

//获取视频参数
XParameter FFDemux::GetVPara() {
    ff_dmx_mutex.lock();
    if (!av_fmt_ctx) {
        ff_dmx_mutex.unlock();
        return XParameter();
    }
    //获取视频流索引
    int vIdx = av_find_best_stream(av_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vIdx < 0) {
        ff_dmx_mutex.unlock();
        XLOGE("find video stream fail");
        return XParameter();
    }
    videoStream = vIdx;
    XParameter para;
    para.para = av_fmt_ctx->streams[vIdx]->codecpar;
    ff_dmx_mutex.unlock();
    return para;
}

//获取视频参数
XParameter FFDemux::GetAPara() {
    ff_dmx_mutex.lock();
    if (!av_fmt_ctx) {
        ff_dmx_mutex.unlock();
        return XParameter();
    }
    //获取视频流索引
    int aIdx = av_find_best_stream(av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (aIdx < 0) {
        ff_dmx_mutex.unlock();
        XLOGE("find audio stream fail");
        return XParameter();
    }
    audioStream = aIdx;
    XParameter para;
    para.para = av_fmt_ctx->streams[aIdx]->codecpar;
    para.channels = av_fmt_ctx->streams[aIdx]->codecpar->channels;
    para.sample_rate = av_fmt_ctx->streams[aIdx]->codecpar->sample_rate;
    ff_dmx_mutex.unlock();
    return para;
}

//读取一帧数据, 数据由调用者清理
XData FFDemux::Read() {
    ff_dmx_mutex.lock();
    int res = 0;
    XData d;
    AVPacket *pkt = av_packet_alloc();
    res = av_read_frame(av_fmt_ctx, pkt);
    if (res < 0) {
        ff_dmx_mutex.unlock();
        av_packet_free(&pkt);
        return d;
    }
    d.data = (unsigned char *) pkt;
    d.size = pkt->size;
    d.pts = (int) (pkt->pts * 1000 * r2d(av_fmt_ctx->streams[pkt->stream_index]->time_base));
    //d.pts = pkt->pts;
    if (pkt->stream_index == videoStream) {
        //XLOGE("video time base: num: %d, den: %d, pts: %lld, real pts: %d", av_fmt_ctx->streams[pkt->stream_index]->time_base.num, av_fmt_ctx->streams[pkt->stream_index]->time_base.den, pkt->pts, d.pts);
        d.type = XDATA_TYPE_VIDEO;
    } else if (pkt->stream_index == audioStream) {
        //XLOGE("audio time base: num: %d, den: %d, pts: %lld, real pts: %d", av_fmt_ctx->streams[pkt->stream_index]->time_base.num, av_fmt_ctx->streams[pkt->stream_index]->time_base.den, pkt->pts, d.pts);
        d.type = XDATA_TYPE_AUDIO;
    } else {
        av_packet_free(&pkt);
        ff_dmx_mutex.unlock();
        return d;
    }
    pkt->pts = d.pts;

    ff_dmx_mutex.unlock();
    return d;
}

FFDemux::FFDemux()
        : av_fmt_ctx(NULL),
          videoStream(-1),
          audioStream(-1) {
    av_register_all();
    avformat_network_init();
}
