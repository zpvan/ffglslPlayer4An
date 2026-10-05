# XPlay (ffglslPlayer4An)

[中文 README](README_CN.md)

A teaching-oriented Android video player: **FFmpeg** (demux/decode) + **OpenGL ES 2.0 GLSL shaders** (YUV→RGB rendering) + **OpenSL ES** (audio). This is a complete, refined implementation of the XPlay course, with bug fixes and full player features on top.

![demo](docs/images/demo.gif)

## Features

- FFmpeg software decoding + MediaCodec hardware decoding (automatic software fallback on failure)
- YUV420P / NV12 / NV21 GLSL shader rendering
- OpenSL ES audio playback
- Seek, pause/resume, progress bar with time display
- SAF file picker (no storage permission needed)
- armeabi-v7a + arm64-v8a

## Architecture

```
 文件/fd ──> FFDemux ──Notify──┬──> FFDecode(video) ──> GLVideoView ──> XTexture → XShader → XEGL
                               └──> FFDecode(audio) ──> FFResample ──> SLAudioPlay ──> OpenSL ES
```

Observer chain + Builder + Proxy. See [docs/architecture.md](docs/architecture.md) for the threading model, A/V sync, Seek/Close sequences and the JNI API table.

## Build

Requirements (exact versions used by this project):

| Tool | Version |
|---|---|
| JDK | 17 (`brew install openjdk@17`) |
| Android SDK | 35 (cmdline-tools) |
| NDK | 29.0.14206865 |
| CMake | 3.22.1 (via sdkmanager) |
| Gradle | 8.11.1 (wrapper included) |

```bash
# 1. Install SDK components
sdkmanager "platforms;android-35" "build-tools;35.0.0" "ndk;29.0.14206865" "cmake;3.22.1"

# 2. Point the project at your SDK
echo "sdk.dir=/path/to/android/sdk" > local.properties

# 3. Build (point JAVA_HOME at JDK 17 if it's not your default)
export JAVA_HOME=/opt/homebrew/opt/openjdk@17
./gradlew assembleDebug
```

## Run

Install the APK, tap **选择文件** (Pick file) to choose a video via the system picker, and it plays. The bottom control bar provides play/pause, a seek bar and current/total time.

The prebuilt FFmpeg 3.4.13 binaries are included; to rebuild them yourself (or trim modules), see [docs/build-ffmpeg.md](docs/build-ffmpeg.md).

## Documentation

- [Architecture (bilingual)](docs/architecture.md)
- [FFmpeg build guide (bilingual)](docs/build-ffmpeg.md)

## License

- The project's own code is under the [MIT License](LICENSE).
- FFmpeg 3.4.13 binaries and headers in `app/libs/` / `app/include/` are under **LGPL 2.1+** — see [NOTICE](NOTICE). If you redistribute them, you must comply with the LGPL.

## Acknowledgements

Built following the XPlay course (FFmpeg + OpenGL ES Android player), extended with fixes and features beyond the course material.
