# XPlay 开源化 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 XPlay 补齐为一个"clone 下来能跑通、能看懂架构"的教学参考开源项目：中英双语文档、MIT+LGPL 合规、arm64-v8a 支持、效果素材。

**Architecture:** 纯文档与构建配置变更，唯一编译动作是用脚本从 FFmpeg 3.4 源码构建 arm64-v8a 的 6 个 `.so` 并入库。

**Tech Stack:** Markdown、MIT/LGPL 文本、FFmpeg 3.4.13 configure/make、NDK 29 clang、Gradle 8.11.1。

**路径约定：** 仓库根目录 `/Users/knox/Documents/GitWorkSpace/ffglslPlayer4An`。构建环境：`JAVA_HOME=/opt/homebrew/opt/openjdk@17`，SDK=/opt/homebrew/share/android-commandlinetools（`local.properties` 已配置）。

---

### Task 1: LICENSE 与 NOTICE

**Files:**
- Create: `LICENSE`
- Create: `NOTICE`

- [ ] **Step 1: 创建 LICENSE（MIT，版权持有者 zpvan）**

标准 MIT License 全文，`Copyright (c) 2018-2026 zpvan`。

- [ ] **Step 2: 创建 NOTICE**

内容（中英双语）：

```text
XPlay (ffglslPlayer4An)
Copyright (c) 2018-2026 zpvan
本项目自有代码采用 MIT License（见 LICENSE）。

This repository contains prebuilt FFmpeg 3.4.x binaries (app/libs/, app/include/).
本仓库包含 FFmpeg 3.4.x 预编译二进制（app/libs/ 与 app/include/ 头文件）。

FFmpeg is licensed under the GNU Lesser General Public License (LGPL) version 2.1 or later.
FFmpeg 依据 LGPL 2.1+ 授权（本项目的编译配置未启用 --enable-gpl）。

本项目以动态链接（.so）方式使用 FFmpeg，并在 docs/build-ffmpeg.md 提供对应
源码版本与完整编译方法，满足 LGPL 对"可替换库"的要求。
This project links FFmpeg dynamically (.so) and provides the corresponding source
version and complete build instructions in docs/build-ffmpeg.md, satisfying the
LGPL "replaceable library" requirement.

FFmpeg source: https://ffmpeg.org/releases/ (ffmpeg-3.4.13)
LGPL text: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html

如果你在作品中再分发这些二进制，需要继续遵守 LGPL 义务。
If you redistribute these binaries in your work, you must comply with the LGPL.
```

- [ ] **Step 3: Commit**

```bash
git add LICENSE NOTICE
git commit -m "docs: 添加 MIT LICENSE 与 FFmpeg LGPL 合规 NOTICE

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 2: FFmpeg 编译脚本与编译指南文档

**Files:**
- Create: `scripts/build_ffmpeg.sh`
- Create: `docs/build-ffmpeg.md`

- [ ] **Step 1: 创建 `scripts/build_ffmpeg.sh`**

```bash
#!/bin/bash
# FFmpeg 3.4.13 Android 交叉编译脚本（armeabi-v7a / arm64-v8a）
# 用法: NDK=/path/to/ndk ./build_ffmpeg.sh [armeabi-v7a|arm64-v8a|all]
# 环境: macOS，NDK r21+（推荐与工程一致的 NDK 29）
set -e

FF_VERSION=3.4.13
NDK=${NDK:-/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865}
ABI=${1:-all}
API=21
SRC_DIR=$(pwd)/ffmpeg-$FF_VERSION
BUILD_DIR=$(pwd)/build

# NDK prebuilt 宿主目录探测（Apple Silicon 的 NDK 仍为 darwin-x86_64）
HOST_TAG=darwin-x86_64
[ -d "$NDK/toolchains/llvm/prebuilt/darwin-arm64" ] && HOST_TAG=darwin-arm64
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_TAG

# 下载源码
if [ ! -d "$SRC_DIR" ]; then
    curl -O https://ffmpeg.org/releases/ffmpeg-$FF_VERSION.tar.xz
    tar xf ffmpeg-$FF_VERSION.tar.xz
fi

build_abi() {
    local abi=$1
    case $abi in
        armeabi-v7a)
            ARCH=arm; CPU=armv7-a; TRIPLE=armv7a-linux-androideabi
            EXTRA_CFLAGS="-mfloat-abi=softfp -mfpu=neon"
            ;;
        arm64-v8a)
            ARCH=aarch64; CPU=armv8-a; TRIPLE=aarch64-linux-android
            EXTRA_CFLAGS=""
            ;;
    esac
    local prefix=$BUILD_DIR/$abi
    cd $SRC_DIR
    make distclean 2>/dev/null || true
    ./configure \
        --prefix=$prefix \
        --arch=$ARCH --cpu=$CPU \
        --target-os=android \
        --enable-cross-compile \
        --cross-prefix=$TOOLCHAIN/bin/llvm- \
        --cc=$TOOLCHAIN/bin/$TRIPLE$API-clang \
        --cxx=$TOOLCHAIN/bin/$TRIPLE$API-clang++ \
        --nm=$TOOLCHAIN/bin/llvm-nm \
        --sysroot=$TOOLCHAIN/sysroot \
        --enable-shared --disable-static \
        --disable-programs --disable-doc --disable-avdevice \
        --enable-jni --enable-mediacodec --enable-decoder=h264_mediacodec \
        --disable-symver \
        --extra-cflags="-Os -fPIC $EXTRA_CFLAGS" \
        --extra-ldflags=""
    make -j$(sysctl -n hw.ncpu)
    make install
    cd ..
    echo "=== $abi done: $prefix/lib ==="
    ls -l $prefix/lib/*.so
}

if [ "$ABI" = "all" ]; then
    build_abi armeabi-v7a
    build_abi arm64-v8a
else
    build_abi $ABI
fi
```

`chmod +x scripts/build_ffmpeg.sh`

- [ ] **Step 2: 创建 `docs/build-ffmpeg.md`（双语章节）**

结构：中文章在前、英文章在后（`## 中文` / `## English`），内容对应：
1. 概述：本仓库预编译二进制的来源与版本（FFmpeg 3.4.13）
2. 环境要求：macOS、NDK（与工程一致的 NDK 29，或 r21+）
3. 一键编译：`NDK=... ./scripts/build_ffmpeg.sh all`
4. configure 关键参数逐项解释（--enable-shared/--disable-static 动态库满足 LGPL；--enable-jni/--enable-mediacodec 支持硬解；--disable-symver 避免 Android 链接版本符号问题；--disable-programs/doc/avdevice 裁剪体积）
5. 产物布局：`build/<abi>/lib/*.so` → 复制到 `app/libs/<abi>/`；`build/<abi>/include/` → `app/include/`（仅需一次）
6. 与工程集成：`CMakeLists.txt` 通过 `${ANDROID_ABI}` 引用 `libs/<abi>` 的说明
7. 常见问题：FFmpeg 3.4 + NDK 29 若 configure 报错，改用 NDK r21e 的方法（下载地址、NDK 环境变量替换）

- [ ] **Step 3: Commit**

```bash
git add scripts/build_ffmpeg.sh docs/build-ffmpeg.md
git commit -m "docs: FFmpeg 3.4.13 Android 交叉编译脚本与双语编译指南

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 3: 实际编译 arm64-v8a .so 并启用双 ABI

**Files:**
- Create: `app/libs/arm64-v8a/*.so`（6 个，脚本产物）
- Modify: `app/build.gradle`

- [ ] **Step 1: 运行编译脚本（arm64-v8a）**

```bash
cd /tmp && mkdir -p ffmpeg-build && cd ffmpeg-build
cp /Users/knox/Documents/GitWorkSpace/ffglslPlayer4An/scripts/build_ffmpeg.sh .
./build_ffmpeg.sh arm64-v8a
```

Expected: `build/arm64-v8a/lib/` 下产出 libavcodec.so libavfilter.so libavformat.so libavutil.so libswresample.so libswscale.so 共 6 个文件。
若 configure/make 报错（FFmpeg 3.4 与 NDK 29 兼容性问题）：按 `docs/build-ffmpeg.md` 常见问题节，用 sdkmanager 以外的渠道下载 NDK r21e（https://developer.android.com/ndk/downloads/older_releases）解压后 `NDK=/path/to/r21e ./build_ffmpeg.sh arm64-v8a` 重试。

- [ ] **Step 2: 校验产物**

```bash
file /tmp/ffmpeg-build/build/arm64-v8a/lib/libavcodec.so
```

Expected: `ELF 64-bit LSB shared object, ARM aarch64`

- [ ] **Step 3: 复制入库**

```bash
mkdir -p /Users/knox/Documents/GitWorkSpace/ffglslPlayer4An/app/libs/arm64-v8a
cp /tmp/ffmpeg-build/build/arm64-v8a/lib/*.so /Users/knox/Documents/GitWorkSpace/ffglslPlayer4An/app/libs/arm64-v8a/
```

- [ ] **Step 4: 开启双 ABI（`app/build.gradle`）**

```groovy
            ndk {
                abiFilters "armeabi-v7a", "arm64-v8a"
            }
```

- [ ] **Step 5: 编译验证 APK 双 ABI**

```bash
export JAVA_HOME=/opt/homebrew/opt/openjdk@17
./gradlew assembleDebug
unzip -l app/build/outputs/apk/debug/app-debug.apk | grep -E "lib/(armeabi-v7a|arm64-v8a)/libavcodec.so"
```

Expected: BUILD SUCCESSFUL；两行分别匹配两个 ABI 的 libavcodec.so

- [ ] **Step 6: Commit**

```bash
git add app/libs/arm64-v8a app/build.gradle
git commit -m "feat: 新增 arm64-v8a FFmpeg 预编译库，启用双 ABI

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 4: 架构文档 docs/architecture.md（双语）

**Files:**
- Create: `docs/architecture.md`

- [ ] **Step 1: 编写架构文档**

结构：`## 中文` 在前、`## English` 在后，章节一一对应：

1. **数据流 Pipeline（ASCII 图）**：

```
        ┌─────────────────────────────────────────────────────────┐
        │                     IPlayer (同步线程)                    │
        │         vdecode->syncPts = audioPlay->pts - bufferedMs  │
        └─────────────────────────────────────────────────────────┘
 文件/fd ──> FFDemux ──Notify(pkt)──┬──> FFDecode(视频) ──> GLVideoView ──> XTexture/XShader/XEGL ──> 屏幕
 (FFDemux线程)                      └──> FFDecode(音频) ──> FFResample ──> SLAudioPlay ──> OpenSL ES ──> 扬声器
```

2. **类职责表**：IObserver/XThread/XData、IDemux/FFDemux、IDecode/FFDecode、IResample/FFResample、IVideoView/GLVideoView/XTexture/XShader/XEGL、IAudioPlay/SLAudioPlay、IPlayer/IPlayerBuilder/FFPlayerBuilder/IPlayerProxy（每个一行职责说明）
3. **线程模型**：demux 线程、视频解码线程、音频解码线程、IPlayer 同步线程、OpenSL 回调线程；各锁的保护范围（ff_dmx_mutex / packetMutex / framesMutex / sl_mux / muxtex）
4. **音视频同步**：`syncPts = 音频pts − GetBufferedMs()`，视频线程 `syncPts < pts` 时等待
5. **Seek 时序**：demux->Seek → vdecode/adecode/audioPlay Clear（含 avcodec_flush_buffers）
6. **Close 时序**：IPlayer::Close 停线程 → 清队列 → audioPlay Close（先置 isExit 唤醒回调）→ videoView/decode/demux Close
7. **Builder + Proxy 模式**：FFPlayerBuilder 组装观察者链，IPlayerProxy 单例加锁对外
8. **JNI API 表**：native_open/native_start/native_setPause/native_seek/native_getProgress/native_initView/native_closeView

- [ ] **Step 2: Commit**

```bash
git add docs/architecture.md
git commit -m "docs: 中英双语架构文档（pipeline/线程模型/同步/Seek/Close 时序）

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 5: README 双语

**Files:**
- Create: `README.md`（英文）
- Create: `README_CN.md`（中文）

- [ ] **Step 1: 编写 README_CN.md**

章节：
1. 项目简介：XPlay 教程（FFmpeg + OpenGL ES + OpenSL ES Android 播放器）的完整实现与完善；[English README](README.md)
2. 效果展示：`docs/images/demo.gif`（见 Task 6）
3. 特性：FFmpeg 软解 / MediaCodec 硬解（失败自动回退软解）、YUV420P/NV12/NV21 GLSL shader 渲染、OpenSL ES 音频、Seek、暂停/继续、SAF 文件选择（免存储权限）、armeabi-v7a + arm64-v8a 双 ABI
4. 架构：pipeline 简图 + 链接 docs/architecture.md
5. 构建指南（精确版本与命令）：
   - JDK 17：`brew install openjdk@17`，`export JAVA_HOME=/opt/homebrew/opt/openjdk@17`
   - Android cmdline-tools + `sdkmanager "platforms;android-35" "build-tools;35.0.0" "ndk;29.0.14206865" "cmake;3.22.1"`
   - `local.properties` 写 `sdk.dir=<SDK 路径>`
   - `./gradlew assembleDebug`
6. 运行：安装 APK → 点"选择文件" → 选视频播放；控制栏：播放/暂停、SeekBar 拖动、进度时间
7. 文档：架构文档、FFmpeg 编译指南
8. 许可证：自有代码 MIT；FFmpeg 部分 LGPL 2.1+（见 NOTICE）
9. 致谢：XPlay 教程

- [ ] **Step 2: 编写 README.md（英文，结构对称）**

与 README_CN.md 章节一一对应，开头互链 `[中文 README](README_CN.md)`。

- [ ] **Step 3: 链接有效性检查**

```bash
grep -oE '\]\(([^)]+)\)' README.md README_CN.md | grep -v http | sort -u
```

逐个确认相对路径文件存在（docs/architecture.md、docs/build-ffmpeg.md、LICENSE、NOTICE、docs/images/demo.gif——demo.gif 由 Task 6 提供）。

- [ ] **Step 4: Commit**

```bash
git add README.md README_CN.md
git commit -m "docs: 中英双语 README（简介/特性/构建指南/许可证/致谢）

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 6: 效果素材与最终验证

**Files:**
- Create: `docs/images/`（截图；GIF 视设备可用性）

- [ ] **Step 1: 检查设备并录制**

```bash
/opt/homebrew/share/android-commandlinetools/platform-tools/adb devices
```

- 有设备：`./gradlew installDebug` → 播放视频 → `adb shell screenrecord`（≤30s）→ pull → 转 GIF（`ffmpeg -i demo.mp4 -vf "fps=10,scale=540:-1" docs/images/demo.gif`，控制在 5MB 内）→ 附一张控制栏截图 `docs/images/screenshot.png`（`adb exec-out screencap -p > docs/images/screenshot.png`）
- 无设备：创建 `docs/images/README.md` 说明素材待补充（"录屏 GIF 待真机录制后放于此目录，文件名 demo.gif"），README 中 demo.gif 引用保留

- [ ] **Step 2: 全新 clone 构建复现验证**

```bash
cd /tmp && rm -rf xplay-verify && git clone /Users/knox/Documents/GitWorkSpace/ffglslPlayer4An xplay-verify && cd xplay-verify
echo "sdk.dir=/opt/homebrew/share/android-commandlinetools" > local.properties
export JAVA_HOME=/opt/homebrew/opt/openjdk@17
./gradlew assembleDebug
```

Expected: BUILD SUCCESSFUL（验证 README 构建指南可复现、.gitignore 未误删必要文件）

- [ ] **Step 3: Commit**

```bash
git add docs/images
git commit -m "docs: 效果素材目录与演示素材

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

## Self-Review 记录

- **Spec 覆盖**：文档体系 → Task 2/4/5；许可证 → Task 1；arm64 → Task 2/3；素材 → Task 6；验证（双 ABI、clone 复现、链接有效）→ Task 3/5/6。全覆盖。
- **占位符扫描**：LICENSE 正文为标准 MIT 文本、NOTICE 全文已给出、编译脚本全文已给出；README/架构文档给出完整章节结构与关键内容块，散文在执行时按结构撰写。
- **一致性**：FFmpeg 版本全文统一 3.4.13；NDK 29.0.14206865 与 `app/build.gradle` 的 `ndkVersion` 一致；README 引用的 demo.gif 路径与 Task 6 产出一致。
