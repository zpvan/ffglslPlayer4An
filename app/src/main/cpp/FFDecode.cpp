//
// Created by nireus on 2018/5/5.
//

#include "FFDecode.h"
#include "XLog.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

FFDecode::FFDecode()
        : av_cdc_ctx(NULL),
          frame(NULL) {
    avcodec_register_all();

}

bool FFDecode::Open(XParameter para, bool isHard) {
    if (!para.para)
        return false;

    int res = 0;
    AVCodecParameters *p = para.para;
    //查找解码器
    AVCodec *av_cdc = avcodec_find_decoder(p->codec_id);
    if (isHard) {
        av_cdc = avcodec_find_decoder_by_name("h264_mediacodec");
    }
    if (!av_cdc) {
        XLOGE("find decoder fail, id: %d, hardware decoder: %d", p->codec_id, isHard);
        return false;
    }

    //创建解码上下文, 并复制参数
    av_cdc_ctx = avcodec_alloc_context3(av_cdc);
    avcodec_parameters_to_context(av_cdc_ctx, p);
    av_cdc_ctx->thread_count = 8;

    //打开解码器
    res = avcodec_open2(av_cdc_ctx, NULL, NULL);
    if (res < 0) {
        XLOGE("open decoder fail, id: %d", p->codec_id);
        return false;
    }

    mediaType = (p->codec_type == AVMEDIA_TYPE_VIDEO ? XDATA_TYPE_VIDEO : XDATA_TYPE_AUDIO);
    XLOGD("open decoder success, id: %d, mediaType: %d, isHard: %d", p->codec_id, mediaType, isHard);
    return true;
}

bool FFDecode::SendPacket(XData packet) {

    if (packet.size <= 0 || !packet.data) {
        return false;
    }

    if (!av_cdc_ctx) {
        return false;
    }
    int res = avcodec_send_packet(av_cdc_ctx, (const AVPacket *) packet.data);
    if (res != 0) {
        return false;
    }

    return true;
}

XData FFDecode::RecvFrame() {

    XData d;
    if (!av_cdc_ctx) {
        return d;
    }
    if (!frame) {
        frame = av_frame_alloc();
    }
    int res = avcodec_receive_frame(av_cdc_ctx, frame);
    if (res != 0) {
        return d;
    }
    d.data = (unsigned char *) frame;
    if (av_cdc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        d.size = (frame->linesize[0] + frame->linesize[1] + frame->linesize[2]) * frame->height;
        d.width = frame->width;
        d.height = frame->height;
    } else if (av_cdc_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        // 样本字节数 * 单通道样本数 * 通道数
        d.size = av_get_bytes_per_sample((AVSampleFormat) frame->format) * frame->nb_samples *
                 frame->channels;
    }
    d.format = frame->format;
    XLOGE("data format is %d", frame->format);
    memcpy(d.datas, frame->data, sizeof(d.datas));
    return d;
}