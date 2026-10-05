//
// Created by nireus on 2018/5/1.
//

#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    XLOGD("idmx-thread");
    while (!isExit) {
        if (isPause) {
            XSleep(2);
            continue;
        }
        XData d = Read();
        //XLOGD("IDemux Read %d", d.size);
        if (d.size <= 0) {
            // EOF 或读取失败：空转等待 Seek，由 Close 的 isExit 终止线程
            XSleep(2);
            continue;
        }
        Notify(d);
    }
}