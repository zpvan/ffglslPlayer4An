//
// Created by nireus on 2018/5/5.
//

#ifndef XPLAY_XLOG_H
#define XPLAY_XLOG_H

#ifdef ANDROID

#include "android/log.h"

#define XLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "XPLAY", __VA_ARGS__)
#define XLOGE(...) __android_log_print(ANDROID_LOG_ERROR, "XPLAY", __VA_ARGS__)

#else
#define XLOGD(...) printf("XPLAY", __VA_ARGS__)
#define XLOGE(...) printf("XPLAY", __VA_ARGS__)

#endif

class XLog {

};


#endif //XPLAY_XLOG_H
