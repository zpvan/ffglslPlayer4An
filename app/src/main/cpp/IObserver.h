//
// Created by nireus on 2018/5/5.
//

#ifndef XPLAY_IOBSERVER_H
#define XPLAY_IOBSERVER_H

#include "XData.h"
#include "vector"
#include "mutex"
#include "XThread.h"

//观察者 和 主体
class IObserver : public XThread {
public:
    //观察者接收数据函数
    virtual void Update(XData data) {};

    //主体函数 添加观察者(线程安全)
    void AddObs(IObserver *obs);

    //通知所有观察者(线程安全)
    void Notify(XData data);

protected:
    std::vector<IObserver *> obss;
    std::mutex mutex;
};


#endif //XPLAY_IOBSERVER_H
