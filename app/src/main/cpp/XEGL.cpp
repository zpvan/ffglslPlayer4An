//
// Created by nireus on 2018/5/6.
//

#include <android/native_window.h>
#include "XEGL.h"
#include "EGL/egl.h"
#include "XLog.h"

class CXEGL : public XEGL {
public:

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    virtual bool Init(void *win) {

        ANativeWindow *nwin = (ANativeWindow *) win;

        //初始化EGL

        //1 获取EGLDisplay对象, 显示设备
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            XLOGE("eglGetDisplay failed");
            return false;
        }
        XLOGD("eglGetDisplay success");

        //2 初始化Display
        if (EGL_TRUE != eglInitialize(display, 0, 0)) {
            XLOGE("eglInitialize failed");
            return false;
        }
        XLOGD("eglInitialize success");

        //3 获取配置
        EGLConfig config = 0;
        EGLint configNum = 0;
        EGLint configSpec[] = {
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_NONE
        };
        if (EGL_TRUE != eglChooseConfig(display, configSpec, &config, 1, &configNum)) {
            XLOGE("eglChooseConfig failed");
            return false;
        }
        XLOGD("eglChooseConfig success");

        //4 创建surface
        surface = eglCreateWindowSurface(display, config, nwin, NULL);
        if (surface == EGL_NO_SURFACE) {
            XLOGE("eglCreateWindowSurface failed");
            return false;
        }
        XLOGD("eglCreateWindowSurface success");

        //5 创建并打开EGL上下文
        const EGLint ctxAttr[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
        };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
        if (config == EGL_NO_CONTEXT) {
            XLOGE("eglCreateContext failed");
            return false;
        }

        //6 上下文切换
        if (EGL_TRUE != eglMakeCurrent(display, surface, surface, context)) {
            XLOGE("eglMakeCurrent failed");
            return false;
        }
        XLOGD("EGL Init Success");

        return true;
    }

    virtual void Draw() {
        if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE) {
            return;
        }

        //窗口显示
        eglSwapBuffers(display, surface);
    }
};

//static
XEGL *XEGL::Get() {
    static CXEGL egl;
    return &egl;
}