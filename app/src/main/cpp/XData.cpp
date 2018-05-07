//
// Created by nireus on 2018/5/1.
//

extern "C" {
#include "libavformat/avformat.h"
}

#include "XData.h"
#include "XLog.h"

bool XData::Alloc(int size, const char *data) {

    Drop();
    dataType = UCHAR_TYPE;
    if (size <= 0)
        return false;

    this->data = new unsigned char[size];

    if (!this->data)
        return false;
    if (data) {
        memcpy(this->data, data, size);
    }
    this->size = size;
    return true;
}

void XData::Drop() {
    if (!data) {
        return;
    }
    if (dataType == AVPACKET_TYPE)
        av_packet_free((AVPacket **) &data);
    else
        delete data;
    data = 0;
    size = 0;
}