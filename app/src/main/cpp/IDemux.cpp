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
            XLOGD("IDemux End of Stream");
            isExit = true;
            break;
        }
        Notify(d);
    }
}