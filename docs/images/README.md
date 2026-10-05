# 演示素材 / Demo assets

`demo.gif`（应用录屏）待真机录制后放置于此目录。

录制方法（连接设备后）：

```bash
./gradlew installDebug
# 在设备上选择并播放视频，同时：
adb shell screenrecord --time-limit 30 /sdcard/demo.mp4
adb pull /sdcard/demo.mp4
ffmpeg -i demo.mp4 -vf "fps=10,scale=540:-1" docs/images/demo.gif
```

附一张控制栏截图更佳：

```bash
adb exec-out screencap -p > docs/images/screenshot.png
```

`demo.gif` (screen recording of the app) is pending on-device capture; place it in this directory.
See the commands above (requires a connected device).
