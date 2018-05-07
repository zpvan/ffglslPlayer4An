//
// Created by nireus on 2018/5/5.
//

#include "IDecode.h"
#include "XLog.h"

void IDecode::Update(XData pkt) {
    if (pkt.type != mediaType) {
        return;
    }

    while (!isExit) {
        packetMutex.lock();

        //阻塞
        if (pkts.size() < maxList) {
            //生产者
            pkts.push_back(pkt);
            packetMutex.unlock();
            break;
        }
        packetMutex.unlock();
        XSleep(1);
    }
}

void IDecode::Main() {
    XLOGD("idec-thread");
    while (!isExit) {
        packetMutex.lock();
        if (pkts.empty()) {
            packetMutex.unlock();
            XSleep(1);
            continue;
        }
        //取出packet
        XData packet = pkts.front();
        //消费者
        pkts.pop_front();

        //发送数据到解码线程, 一个数据包, 可能解码多个结果
        if (this->SendPacket(packet)) {
            while (!isExit) {
                //获取解码数据
                XData frame = RecvFrame();
                if (!frame.data)
                    break;
                //XLOGD("decoded frame size: %d", frame.size);
                //发送数据给观察者
                this->Notify(frame);
            }
        }
        packet.Drop();
        packetMutex.unlock();
    }
}