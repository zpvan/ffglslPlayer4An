//
// Created by nireus on 2018/5/1.
//

#ifndef XPLAY_XDATA_H
#define XPLAY_XDATA_H

#define XDATA_TYPE_VIDEO  0
#define XDATA_TYPE_AUDIO  1

enum XDataType {
    AVPACKET_TYPE = 0,
    UCHAR_TYPE = 1
};

class XData {
public:
    unsigned char *data = 0;
    unsigned char *datas[8] = {0};
    int size = 0;
    int type = -1;
    int width = 0;
    int height = 0;
    XDataType dataType = AVPACKET_TYPE;
    //
    int format = 0;
    int pts = 0;

    bool Alloc(int size, const char *data = 0);

    void Drop();
};


#endif //XPLAY_XDATA_H
