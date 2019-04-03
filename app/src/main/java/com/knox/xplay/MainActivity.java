package com.knox.xplay;

import android.Manifest;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {

        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        final FrameLayout frameLayout = (FrameLayout) LayoutInflater.from(this).inflate(R.layout.activity_main, null, false);
        final FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            PermissionHelper.runOnPermissionGranted(MainActivity.this, new Runnable() {
                @Override
                public void run() {
                    setUpView(frameLayout, layoutParams);
                }
            }, new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, "can't have permision", Toast.LENGTH_LONG).show();
                }
            }, Manifest.permission.READ_EXTERNAL_STORAGE);
        } else {
            setUpView(frameLayout, layoutParams);
        }

        // Example of a call to a native method
        // TextView tv = (TextView) findViewById(R.id.sample_text);
        // tv.setText(stringFromJNI());
    }

    private void setUpView(FrameLayout frameLayout, FrameLayout.LayoutParams layoutParams) {
        XPlay xPlay = new XPlay(this);
        frameLayout.addView(xPlay, layoutParams);

        setContentView(frameLayout);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
