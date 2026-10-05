//
// Created by nireus on 2018/5/7.
//

#include "IAudioPlay.h"

void IAudioPlay::Update(XData data) {
    //压入缓冲队列
    if (data.size <= 0 || !data.data) {
        return;
    }

    while (!isExit) {
        framesMutex.lock();
        if (frames.size() > maxFrame) {
            framesMutex.unlock();
            XSleep(1);
            continue;
        }
        frames.push_back(data);
        bufferedBytes += data.size;
        framesMutex.unlock();
        break;
    }
}

void IAudioPlay::Clear() {
    framesMutex.lock();
    while (!frames.empty()) {
        frames.front().Drop();
        frames.pop_front();
    }
    bufferedBytes = 0;
    framesMutex.unlock();
}

long long IAudioPlay::GetBufferedMs() {
    long long bytesPerSec = (long long) sampleRate * channels * 2; // S16
    if (bytesPerSec <= 0)
        return 0;
    framesMutex.lock();
    long long bytes = bufferedBytes;
    framesMutex.unlock();
    return bytes * 1000 / bytesPerSec;
}

XData IAudioPlay::GetData() {
    XData d;
    while (!isExit) {
        framesMutex.lock();
        if (!frames.empty()) {
            d = frames.front();
            frames.pop_front();
            bufferedBytes -= d.size;
            if (bufferedBytes < 0)
                bufferedBytes = 0;
            framesMutex.unlock();
            pts = d.pts;
            return d;
        }
        framesMutex.unlock();
        XSleep(1);
    }

    return d;
}