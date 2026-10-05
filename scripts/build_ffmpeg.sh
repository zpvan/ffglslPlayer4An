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
        *)
            echo "unknown abi: $abi"; exit 1
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
        --disable-demuxer=tty \
        --enable-jni --enable-mediacodec --enable-decoder=h264_mediacodec \
        --disable-symver \
        --extra-cflags="-Os -fPIC -Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types -Wno-int-conversion $EXTRA_CFLAGS" \
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
