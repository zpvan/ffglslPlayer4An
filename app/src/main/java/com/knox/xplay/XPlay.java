package com.knox.xplay;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by nireus on 2018/5/6.
 * EGL 完全由 native 层持有，这里只用纯 SurfaceView，避免 GLSurfaceView 与 native 双 EGL surface 冲突。
 */

public class XPlay extends SurfaceView implements SurfaceHolder.Callback {

    private static final String TAG = "XPlay";

    public XPlay(Context context) {
        this(context, null);
    }

    public XPlay(Context context, AttributeSet attrs) {
        super(context, attrs);
        getHolder().addCallback(this);
    }

    public static native boolean native_open(String path);

    public static native boolean native_start();

    public static native void native_setPause(boolean pause);

    public static native void native_seek(double pos);

    public static native long[] native_getProgress();

    private native void native_initView(Object surface);

    private native void native_closeView();

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.e(TAG, "surfaceCreated, holder: " + holder);
        native_initView(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        Log.e(TAG, "surfaceChanged, holder: " + holder);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.e(TAG, "surfaceDestroyed, holder: " + holder);
        native_closeView();
    }
}
