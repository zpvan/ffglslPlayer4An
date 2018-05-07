//
// Created by nireus on 2018/5/5.
//

#include "IObserver.h"

//主体函数
void IObserver::AddObs(IObserver *obs) {
    if (!obs)
        return;

    mutex.lock();
    obss.push_back(obs);
    mutex.unlock();
}

//通知所有观察者
void IObserver::Notify(XData data) {
    mutex.lock();
    for (int i = 0; i < obss.size(); ++i) {
        obss[i]->Update(data);
    }
    mutex.unlock();
}