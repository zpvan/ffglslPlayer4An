package com.knox.xplay;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    private static final int REQ_PICK_VIDEO = 1;

    private Button btnPlay;
    private SeekBar seekBar;
    private TextView txtTime;

    private ParcelFileDescriptor pfd;
    private boolean isPause = false;
    private boolean isPlaying = false;
    private boolean isSeeking = false;

    private final Handler handler = new Handler();
    private final Runnable progressRunnable = new Runnable() {
        @Override
        public void run() {
            updateProgress();
            handler.postDelayed(this, 500);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        setContentView(R.layout.activity_main);

        Button btnPick = (Button) findViewById(R.id.btnPick);
        btnPlay = (Button) findViewById(R.id.btnPlay);
        seekBar = (SeekBar) findViewById(R.id.seekBar);
        txtTime = (TextView) findViewById(R.id.txtTime);

        btnPick.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("video/*");
                startActivityForResult(intent, REQ_PICK_VIDEO);
            }
        });

        btnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!isPlaying)
                    return;
                isPause = !isPause;
                XPlay.native_setPause(isPause);
                btnPlay.setText(isPause ? "继续" : "暂停");
            }
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                isSeeking = false;
                if (!isPlaying)
                    return;
                XPlay.native_seek(seekBar.getProgress() / 1000.0);
                // seek 后若处于暂停则恢复播放
                if (isPause) {
                    isPause = false;
                    XPlay.native_setPause(false);
                    btnPlay.setText("暂停");
                }
            }
        });

        handler.postDelayed(progressRunnable, 500);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK_VIDEO || resultCode != RESULT_OK || data == null)
            return;

        Uri uri = data.getData();
        if (uri == null)
            return;

        closePfd();
        try {
            pfd = getContentResolver().openFileDescriptor(uri, "r");
        } catch (IOException e) {
            Toast.makeText(this, "无法打开文件", Toast.LENGTH_LONG).show();
            return;
        }
        if (pfd == null) {
            Toast.makeText(this, "无法打开文件", Toast.LENGTH_LONG).show();
            return;
        }

        // native 通过 /proc/self/fd/N 读取，pfd 由本 Activity 持有至下次选择或销毁
        String path = "/proc/self/fd/" + pfd.getFd();
        if (XPlay.native_open(path)) {
            XPlay.native_start();
            isPlaying = true;
            isPause = false;
            btnPlay.setText("暂停");
        } else {
            Toast.makeText(this, "打开失败: " + uri, Toast.LENGTH_LONG).show();
        }
    }

    private void updateProgress() {
        if (!isPlaying || isSeeking)
            return;
        long[] progress = XPlay.native_getProgress();
        if (progress == null || progress.length < 2)
            return;
        long cur = progress[0];
        long total = progress[1];
        if (total > 0) {
            seekBar.setProgress((int) (cur * 1000 / total));
            txtTime.setText(String.format(Locale.US, "%s/%s", formatMs(cur), formatMs(total)));
            // 播放完成：置为暂停，等待用户 seek 或重新选择
            if (cur >= total - 500) {
                isPause = true;
                XPlay.native_setPause(true);
                btnPlay.setText("继续");
            }
        }
    }

    private static String formatMs(long ms) {
        long totalSec = ms / 1000;
        return String.format(Locale.US, "%02d:%02d", totalSec / 60, totalSec % 60);
    }

    private void closePfd() {
        if (pfd != null) {
            try {
                pfd.close();
            } catch (IOException ignored) {
            }
            pfd = null;
        }
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(progressRunnable);
        closePfd();
        super.onDestroy();
    }
}
