//
// Created by nireus on 2018/5/5.
//

#ifndef XPLAY_XTHREAD_H
#define XPLAY_XTHREAD_H

//sleep 毫秒
void XSleep(int ms);


//c++ 11 线程库
class XThread {
public:
    //启动线程
    virtual bool Start();

    //通过控制isExit安全停止线程(不一定成功)
    virtual void Stop();

    //入口主函数
    virtual void Main() {};

protected:
    bool isExit = false;
    bool isRunnig = false;
private:
    void ThreadMain();
};


#endif //XPLAY_XTHREAD_H
