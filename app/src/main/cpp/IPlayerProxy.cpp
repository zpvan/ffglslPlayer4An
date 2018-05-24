//
// Created by nireus on 2018/5/16.
//

#include "IPlayerProxy.h"
#include "FFPlayerBuilder.h"

void IPlayerProxy::Init(void *vm) {
    mux.lock();
    if (vm) {
        FFPlayerBuilder::InitHard(vm);
    }
    if (!player) {
        player = FFPlayerBuilder::Get()->BuilderPlayer();
    }
    mux.unlock();
}

void IPlayerProxy::Close() {
    mux.lock();
    if (player)
        player->Close();
    mux.unlock();
}

bool IPlayerProxy::Open(const char *path) {
    mux.lock();
    if (player) {
        player->Open(path);
    }
    mux.unlock();
    return true;
}

bool IPlayerProxy::Start() {
    mux.lock();
    if (player) {
        player->Start();
    }
    mux.unlock();
    return true;
}

bool IPlayerProxy::InitView(void *win) {
    mux.lock();
    if (player) {
        player->InitView(win);
    }
    mux.unlock();
    return true;
}
