//
// Created by nireus on 2018/5/6.
//

#ifndef XPLAY_GLVIDEOVIEW_H
#define XPLAY_GLVIDEOVIEW_H


#include "IVideoView.h"

class XTexture;

class GLVideoView : public IVideoView {
public:
    virtual void SetRender(void *win);

    virtual void Render(XData data);

    virtual void Close();

protected:
    void *view = 0;
    XTexture *txt;
    std::mutex mux;
};


#endif //XPLAY_GLVIDEOVIEW_H
