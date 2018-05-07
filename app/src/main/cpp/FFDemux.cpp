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

    int res = 0;
    res = avformat_open_input(&av_fmt_ctx, url, NULL, NULL);
    if (res != 0) {
        XLOGE("open %s failed", url);
        return false;
    }
    XLOGD("open %s success", url);

    res = avformat_find_stream_info(av_fmt_ctx, NULL);
    if (res < 0) {
        XLOGE("parse %s failed", url);
    }

    durationMs = av_fmt_ctx->duration / (AV_TIME_BASE / 1000);
    XLOGD("parse %s success, duration: %lld ms", url, durationMs);
    GetVPara();
    GetAPara();

    return true;
}

//获取视频参数
XParameter FFDemux::GetVPara() {
    if (!av_fmt_ctx) {
        return XParameter();
    }
    //获取视频流索引
    int vIdx = av_find_best_stream(av_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vIdx < 0) {
        XLOGE("find video stream fail");
        return XParameter();
    }
    videoStream = vIdx;
    XParameter para;
    para.para = av_fmt_ctx->streams[vIdx]->codecpar;
    return para;
}

//获取视频参数
XParameter FFDemux::GetAPara() {
    if (!av_fmt_ctx) {
        return XParameter();
    }
    //获取视频流索引
    int aIdx = av_find_best_stream(av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (aIdx < 0) {
        XLOGE("find audio stream fail");
        return XParameter();
    }
    audioStream = aIdx;
    XParameter para;
    para.para = av_fmt_ctx->streams[aIdx]->codecpar;
    para.channels = av_fmt_ctx->streams[aIdx]->codecpar->channels;
    para.sample_rate = av_fmt_ctx->streams[aIdx]->codecpar->sample_rate;
    return para;
}

//读取一帧数据, 数据由调用者清理
XData FFDemux::Read() {
    int res = 0;
    XData d;
    AVPacket *pkt = av_packet_alloc();
    res = av_read_frame(av_fmt_ctx, pkt);
    if (res < 0) {
        av_packet_free(&pkt);
        return d;
    }
    d.data = (unsigned char *) pkt;
    d.size = pkt->size;
    if (pkt->stream_index == videoStream) {
        d.type = XDATA_TYPE_VIDEO;
    } else if (pkt->stream_index == audioStream) {
        d.type = XDATA_TYPE_AUDIO;
    }
    return d;
}

FFDemux::FFDemux()
        : av_fmt_ctx(NULL),
          videoStream(-1),
          audioStream(-1) {
    av_register_all();
    avformat_network_init();
}
