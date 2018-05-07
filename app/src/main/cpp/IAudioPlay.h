//
// Created by nireus on 2018/5/7.
//

#ifndef XPLAY_IAUDIOPLAY_H
#define XPLAY_IAUDIOPLAY_H


#include <list>
#include "IObserver.h"
#include "XParameter.h"

class IAudioPlay : public IObserver {
public:
    //缓冲满了之后阻塞
    virtual void Update(XData data);

    virtual bool StartPlay(XParameter out) = 0;

    //获取缓冲数据, 如没有则阻塞
    virtual XData GetData();

    //最大的队列缓冲
    int maxFrame = 100;

protected:
    std::list<XData> frames;
    std::mutex framesMutex;
};


#endif //XPLAY_IAUDIOPLAY_H
