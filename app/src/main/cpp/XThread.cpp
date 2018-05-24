//
// Created by nireus on 2018/5/5.
//

#include "XThread.h"
#include "thread"
#include "XLog.h"

using namespace std;

void XSleep(int ms) {
    chrono::milliseconds du(ms);
    this_thread::sleep_for(du);
}

//启动线程
bool XThread::Start() {
    isExit = false;
    thread th(&XThread::ThreadMain, this);
    th.detach();

    return true;
}

void XThread::ThreadMain() {
    isRunnig = true;
    XLOGD("线程函数进入");
    Main();
    XLOGD("线程函数退出");
    isRunnig = false;
}

//通过控制isExit安全停止线程(不一定成功)
void XThread::Stop() {
    isExit = true;
    for(int i = 0; i < 200; i++) {
        if (!isRunnig) {
            XLOGD("Stop 停止线程成功");
            return;
        }
        XSleep(1);
    }
    XLOGD("Stop 停止线程超时");
}