# XPlay (ffglslPlayer4An)

[English README](README.md)

一个教学参考的 Android 视频播放器：**FFmpeg**（解封装/解码）+ **OpenGL ES 2.0 GLSL Shader**（YUV→RGB 渲染）+ **OpenSL ES**（音频播放）。本项目是 XPlay 教程的完整实现，并在教程基础上修复了若干 bug、补齐了完整播放器功能。

![demo](docs/images/demo.gif)

## 特性

- FFmpeg 软解 + MediaCodec 硬解（失败自动回退软解）
- YUV420P / NV12 / NV21 GLSL shader 渲染
- OpenSL ES 音频播放
- Seek 拖动、暂停/继续、进度条与时间显示
- SAF 文件选择（免存储权限）
- armeabi-v7a + arm64-v8a 双 ABI

## 架构

```
 文件/fd ──> FFDemux ──Notify──┬──> FFDecode(视频) ──> GLVideoView ──> XTexture → XShader → XEGL
                               └──> FFDecode(音频) ──> FFResample ──> SLAudioPlay ──> OpenSL ES
```

观察者链 + Builder + Proxy。线程模型、音视频同步、Seek/Close 时序与 JNI API 详见 [docs/architecture.md](docs/architecture.md)。

## 构建

精确版本（本项目实际使用）：

| 工具 | 版本 |
|---|---|
| JDK | 17（`brew install openjdk@17`） |
| Android SDK | 35（cmdline-tools） |
| NDK | 29.0.14206865 |
| CMake | 3.22.1（sdkmanager 安装） |
| Gradle | 8.11.1（wrapper 自带） |

```bash
# 1. 安装 SDK 组件
sdkmanager "platforms;android-35" "build-tools;35.0.0" "ndk;29.0.14206865" "cmake;3.22.1"

# 2. 指向本机 SDK
echo "sdk.dir=/path/to/android/sdk" > local.properties

# 3. 构建（若默认 Java 不是 17，先指定 JAVA_HOME）
export JAVA_HOME=/opt/homebrew/opt/openjdk@17
./gradlew assembleDebug
```

## 运行

安装 APK → 点"选择文件" → 在系统选择器中选一个视频即可播放。底部控制栏提供播放/暂停、SeekBar 拖动与进度时间显示。

仓库已内置 FFmpeg 3.4.13 预编译库；如需自行编译（或裁剪模块），见 [docs/build-ffmpeg.md](docs/build-ffmpeg.md)。

## 文档

- [架构文档（双语）](docs/architecture.md)
- [FFmpeg 编译指南（双语）](docs/build-ffmpeg.md)

## 许可证

- 本项目自有代码采用 [MIT License](LICENSE)。
- `app/libs/` 与 `app/include/` 中的 FFmpeg 3.4.13 二进制与头文件遵循 **LGPL 2.1+**，详见 [NOTICE](NOTICE)。再分发这些二进制需遵守 LGPL 义务。

## 致谢

基于 XPlay 教程（FFmpeg + OpenGL ES Android 播放器）实现，并在教程内容之上做了修复与功能扩展。
