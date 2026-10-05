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

    //暂停/继续（线程不退出，主循环空转）
    virtual void SetPause(bool isPause);

    //入口主函数
    virtual void Main() {};

protected:
    bool isExit = false;
    bool isRunnig = false;
    bool isPause = false;
private:
    void ThreadMain();
};


#endif //XPLAY_XTHREAD_H
