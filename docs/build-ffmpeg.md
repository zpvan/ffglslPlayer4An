# FFmpeg 编译指南 / FFmpeg Build Guide

## 中文

本仓库 `app/libs/` 与 `app/include/` 中的预编译产物来自 **FFmpeg 3.4.13**，由 `scripts/build_ffmpeg.sh` 交叉编译生成。如需自行编译（例如裁剪模块、替换新版本），按本指南操作。

### 环境要求

- macOS（脚本探测 `darwin-x86_64` / `darwin-arm64` 宿主目录）
- Android NDK r21+，推荐与工程一致的 **NDK 29.0.14206865**（`sdkmanager "ndk;29.0.14206865"`）

### 一键编译

```bash
# 在任意工作目录执行（脚本会自动下载并解压 ffmpeg-3.4.13 源码）
export NDK=/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865
./scripts/build_ffmpeg.sh all            # armeabi-v7a + arm64-v8a
./scripts/build_ffmpeg.sh arm64-v8a      # 只编 arm64
```

### 产物布局

```
build/<abi>/lib/*.so      → 复制到 app/libs/<abi>/
build/<abi>/include/      → app/include/（头文件两个 ABI 相同，只需复制一次）
```

工程集成方式：`app/CMakeLists.txt` 通过 `libs/${ANDROID_ABI}` 引用对应 ABI 的 `.so`，`app/build.gradle` 的 `abiFilters` 控制打包哪些 ABI。

### configure 关键参数

| 参数 | 作用 |
|---|---|
| `--enable-shared --disable-static` | 产出动态库（.so），动态链接是满足 LGPL "可替换库"要求的方式 |
| `--enable-jni --enable-mediacodec --enable-decoder=h264_mediacodec` | 启用 MediaCodec 硬解（工程 `FFDecode` 的 `isHardDecode` 依赖它） |
| `--disable-programs --disable-doc --disable-avdevice` | 裁剪 ffmpeg/ffprobe 命令行程序与 avdevice，减小体积 |
| `--disable-demuxer=tty` | 绕过 FFmpeg 3.4 源码与新版 clang 的不兼容（tty.c 缺 `internal.h` 包含且 probe 签名过时）；播放器用不到 tty 字幕 demuxer |
| `--disable-symver` | 关闭符号版本，避免 Android 链接期版本符号问题 |
| `--extra-cflags="-Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types -Wno-int-conversion"` | FFmpeg 3.4 的部分历史代码在新版 clang（NDK 29，默认高版本 C 标准）下会从警告升级为报错，这里降级回警告 |

### 常见问题

- **configure 报 compiler 相关错误**：确认 `NDK` 环境变量指向 NDK 根目录（内含 `toolchains/llvm/prebuilt/`）。
- **仍想使用旧版 NDK**：NDK r21e 等旧版可从 https://developer.android.com/ndk/downloads/older_releases 下载，解压后 `NDK=/path/to/r21e ./scripts/build_ffmpeg.sh all`。FFmpeg 3.4 与 NDK r17~r21 原生兼容，此时可移除 `--disable-demuxer=tty` 与 `-Wno-*` 参数。

---

## English

The prebuilt binaries in `app/libs/` and headers in `app/include/` are built from **FFmpeg 3.4.13** by `scripts/build_ffmpeg.sh`. Follow this guide if you want to rebuild them yourself (e.g. to trim modules or bump the version).

### Requirements

- macOS (the script auto-detects the `darwin-x86_64` / `darwin-arm64` host tag)
- Android NDK r21+, ideally **NDK 29.0.14206865** to match the project (`sdkmanager "ndk;29.0.14206865"`)

### One-command build

```bash
# Run in any working directory; the script downloads and extracts ffmpeg-3.4.13 itself
export NDK=/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865
./scripts/build_ffmpeg.sh all            # armeabi-v7a + arm64-v8a
./scripts/build_ffmpeg.sh arm64-v8a      # arm64 only
```

### Output layout

```
build/<abi>/lib/*.so      → copy to app/libs/<abi>/
build/<abi>/include/      → app/include/ (headers are ABI-independent, copy once)
```

Integration: `app/CMakeLists.txt` references `libs/${ANDROID_ABI}`, and `abiFilters` in `app/build.gradle` controls which ABIs get packaged.

### Key configure flags

| Flag | Purpose |
|---|---|
| `--enable-shared --disable-static` | Build shared libraries (.so); dynamic linking satisfies the LGPL "replaceable library" requirement |
| `--enable-jni --enable-mediacodec --enable-decoder=h264_mediacodec` | Enable MediaCodec hardware decoding (used by `FFDecode`'s `isHardDecode`) |
| `--disable-programs --disable-doc --disable-avdevice` | Trim ffmpeg/ffprobe CLI programs and avdevice to reduce size |
| `--disable-demuxer=tty` | Works around an FFmpeg 3.4 source incompatibility with modern clang (tty.c misses an `internal.h` include and uses an outdated probe signature); the player never uses the tty subtitle demuxer |
| `--disable-symver` | Disable symbol versioning to avoid Android link-time versioned-symbol issues |
| `--extra-cflags="-Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types -Wno-int-conversion"` | FFmpeg 3.4 legacy code triggers hard errors under modern clang (NDK 29, newer default C standard); these downgrade them back to warnings |

### Troubleshooting

- **Compiler errors in configure**: make sure `NDK` points to the NDK root (the directory containing `toolchains/llvm/prebuilt/`).
- **Prefer an older NDK**: NDK r21e and friends are available at https://developer.android.com/ndk/downloads/older_releases. FFmpeg 3.4 is natively compatible with NDK r17–r21; with those you can drop `--disable-demuxer=tty` and the `-Wno-*` flags.
