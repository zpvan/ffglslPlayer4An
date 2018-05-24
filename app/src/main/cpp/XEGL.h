//
// Created by nireus on 2018/5/6.
//

#ifndef XPLAY_XEGL_H
#define XPLAY_XEGL_H


class XEGL {
public:
    virtual bool Init(void *nwin) = 0;

    virtual void Draw() = 0;

    virtual void Close() = 0;

    static XEGL *Get();

protected:
    XEGL() {};
};


#endif //XPLAY_XEGL_H
