# XPlay 播放器完善 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修复 XPlay 播放器的已确认 bug，补齐 Seek/暂停/进度 UI/播放完成处理，消除双 EGL 冲突与音视频同步偏移，播放源改为 SAF 文件选择器。

**Architecture:** 保留教程的观察者链（IDemux→IDecode→IVideoView / IResample→IAudioPlay）+ XThread + Builder + IPlayerProxy 架构，在接口层扩展 Seek/SetPause/进度查询。EGL 完全由 native 层持有，XPlay 改为纯 SurfaceView。

**Tech Stack:** Android (compileSdk 26, support library)、JNI、FFmpeg 3.x、OpenGL ES 2.0、OpenSL ES、CMake。

**验证方式说明：** 本工程 native 层依赖 Android 设备，无单元测试基础（设计文档已确认）。每个 Task 的验证门为 `./gradlew assembleDebug` 编译通过，最后由 Task 11 的手动真机 checklist 验收。

**路径约定：** 以下所有相对路径基于仓库根目录 `/Users/knox/Documents/GitWorkSpace/ffglslPlayer4An`。cpp 源码目录统一简写为 `app/src/main/cpp/`。

---

### Task 1: 仓库卫生（.gitignore + 清除已跟踪编译产物）

**Files:**
- Modify: `.gitignore`

- [ ] **Step 1: 重写 .gitignore**

将 `.gitignore` 整体替换为：

```gitignore
*.iml
.gradle
/local.properties
/.idea
.DS_Store
/build
/captures
.externalNativeBuild
.cxx
.settings
.project
.classpath
app/build
```

- [ ] **Step 2: 从 git 索引移除已跟踪的 IDE/编译产物（保留磁盘文件）**

```bash
cd /Users/knox/Documents/GitWorkSpace/ffglslPlayer4An
git rm -r --cached .idea .settings .project app/.cxx 2>/dev/null
git rm --cached app/.project app/.classpath 2>/dev/null
```

- [ ] **Step 3: 验证 git status 干净且产物文件仍在磁盘上**

Run: `git status --short | head -20 && ls app/.cxx >/dev/null && echo "cxx still on disk"`
Expected: 大量 `D`（仅索引删除）记录；输出 `cxx still on disk`

- [ ] **Step 4: Commit**

```bash
git add .gitignore
git commit -m "chore: 完善 .gitignore，移除已跟踪的 IDE 与 CMake 编译产物

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 2: Native 内存与小型 Bug 修复包

**Files:**
- Modify: `app/src/main/cpp/XData.cpp`
- Modify: `app/src/main/cpp/SLAudioPlay.cpp`
- Modify: `app/src/main/cpp/FFResample.cpp`
- Modify: `app/src/main/cpp/IDecode.h`
- Modify: `app/src/main/cpp/XEGL.cpp`
- Modify: `app/src/main/cpp/XData.h`
- Modify: `app/src/main/cpp/IAudioPlay.h`
- Modify: `app/src/main/cpp/FFDemux.cpp`
- Modify: `app/src/main/cpp/IPlayer.cpp`

- [ ] **Step 1: XData::Drop 数组释放修复（`XData.cpp`）**

将 `Drop()` 中的 `delete data;` 改为：

```cpp
    if (dataType == AVPACKET_TYPE)
        av_packet_free((AVPacket **) &data);
    else
        delete[] data;
```

- [ ] **Step 2: pts 类型 int → long long**

`XData.h`：`int pts = 0;` → `long long pts = 0;`

`IDecode.h`：`int syncPts = 0;` → `long long syncPts = 0;`；`int pts = 0;` → `long long pts = 0;`；`int mediaType;` → `int mediaType = -1;`（顺带初始化）

`IAudioPlay.h`：`int pts = 0;` → `long long pts = 0;`

`FFDemux.cpp` `Read()` 中：
```cpp
    d.pts = (int) (pkt->pts * 1000 * r2d(av_fmt_ctx->streams[pkt->stream_index]->time_base));
```
改为：
```cpp
    d.pts = (long long) (pkt->pts * 1000 * r2d(av_fmt_ctx->streams[pkt->stream_index]->time_base));
```

`IPlayer.cpp` `Main()` 中 `int apts = audioPlay->pts;` → `long long apts = audioPlay->pts;`

- [ ] **Step 3: SLAudioPlay 缓冲保护与数组释放（`SLAudioPlay.cpp`）**

析构函数 `delete (buf);` → `delete[] buf;`

`PlayCall()` 中 memcpy 前增加大小检查：

```cpp
void SLAudioPlay::PlayCall(void *bufq) {
    if (!bufq)
        return;

    SLAndroidSimpleBufferQueueItf bf = (SLAndroidSimpleBufferQueueItf) bufq;
    //阻塞函数
    XData d = GetData();
    if (d.size <= 0) {
        return;
    }
    if (!buf) {
        d.Drop();
        return;
    }
    if (d.size > 1024 * 1024) {
        XLOGE("SLAudioPlay::PlayCall frame too large: %d, dropped", d.size);
        d.Drop();
        return;
    }
    memcpy(buf, d.data, (size_t) d.size);
    sl_mux.lock();
    (*bf)->Enqueue(bf, buf, (size_t) d.size);
    d.Drop();
    sl_mux.unlock();
}
```

- [ ] **Step 4: FFResample 泄漏修复（`FFResample.cpp` `Open()`）**

删除该行：`uint8_t *pcm = new uint8_t[4800 * 4 * 2];`

- [ ] **Step 5: XEGL 复制粘贴错误修复（`XEGL.cpp` `Init()`）**

`if (config == EGL_NO_CONTEXT) {` → `if (context == EGL_NO_CONTEXT) {`

- [ ] **Step 6: FFDemux 健壮性（`FFDemux.cpp`）**

`Open()` 中 `avformat_find_stream_info` 失败即失败返回：

```cpp
    res = avformat_find_stream_info(av_fmt_ctx, NULL);
    if (res < 0) {
        ff_dmx_mutex.unlock();
        XLOGE("parse %s failed", url);
        return false;
    }
```

`Read()` 开头增加判空：

```cpp
XData FFDemux::Read() {
    ff_dmx_mutex.lock();
    if (!av_fmt_ctx) {
        ff_dmx_mutex.unlock();
        return XData();
    }
    int res = 0;
    XData d;
    // ... 后续不变
```

- [ ] **Step 7: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 8: Commit**

```bash
git add app/src/main/cpp/
git commit -m "fix: 内存释放 UB、pts 截断、XEGL 判断错误、FFDemux 健壮性等小型 bug

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 3: OpenSL 生命周期修复（IAudioPlay / SLAudioPlay）

**Files:**
- Modify: `app/src/main/cpp/IAudioPlay.cpp`
- Modify: `app/src/main/cpp/SLAudioPlay.cpp`

- [ ] **Step 1: IAudioPlay::Clear 同时置 isExit（`IAudioPlay.cpp`）**

`Clear()` 保持原逻辑不变（它由 Close 调用，Close 会先置 isExit）。无需改本文件——跳过。

- [ ] **Step 2: 重写 SLAudioPlay::Close（`SLAudioPlay.cpp`）**

```cpp
void SLAudioPlay::Close() {

    // 先唤醒可能空转在 GetData() 的 OpenSL 回调线程
    isExit = true;
    IAudioPlay::Clear();

    sl_mux.lock();
    // 停止播放
    if (audioPlayerItf && (*audioPlayerItf)) {
        (*audioPlayerItf)->SetPlayState(audioPlayerItf, SL_PLAYSTATE_STOPPED);
    }
    // 清理播放队列
    if (pcmQueue && (*pcmQueue)) {
        (*pcmQueue)->Clear(pcmQueue);
    }
    // 销毁队列
    if (audioPlayer && (*audioPlayer)) {
        (*audioPlayer)->Destroy(audioPlayer);
    }
    // 销毁混音器
    if (outputMixObject && (*outputMixObject)) {
        (*outputMixObject)->Destroy(outputMixObject);
    }
    // 销毁播放引擎
    if (engineObject && (*engineObject)) {
        (*engineObject)->Destroy(engineObject);
    }
    // 置空，支持重复 Close 与二次 StartPlay
    audioPlayer = NULL;
    audioPlayerItf = NULL;
    pcmQueue = NULL;
    outputMixObject = NULL;
    engineObject = NULL;
    engineItf = NULL;
    sl_mux.unlock();
}
```

- [ ] **Step 3: StartPlay 开头复位 isExit（`SLAudioPlay.cpp`）**

`StartPlay()` 函数体第一行 `Close();` 之后增加：

```cpp
    isExit = false;
```

- [ ] **Step 4: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 5: Commit**

```bash
git add app/src/main/cpp/SLAudioPlay.cpp
git commit -m "fix: OpenSL 销毁后指针置空，Close 先唤醒空转回调消除死锁窗口

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 4: 解码错误处理、硬解回退与重采样缓冲修复

**Files:**
- Modify: `app/src/main/cpp/IPlayer.cpp`
- Modify: `app/src/main/cpp/IPlayerProxy.cpp`
- Modify: `app/src/main/cpp/FFResample.cpp`

- [ ] **Step 1: IPlayer::Open 硬解回退与失败返回（`IPlayer.cpp`）**

替换 `Open()` 函数体为：

```cpp
bool IPlayer::Open(const char *path) {

    Close();

    muxtex.lock();
    if (!demux || !demux->Open(path)) {
        XLOGE("demux->Open %s failed!", path);
        muxtex.unlock();
        return false;
    }
    if (!vdecode) {
        muxtex.unlock();
        return false;
    }
    // 硬解失败回退软解
    if (!vdecode->Open(demux->GetVPara(), isHardDecode)) {
        XLOGE("vdecode->Open %s failed(hard=%d), fallback to software", path, isHardDecode);
        if (!vdecode->Open(demux->GetVPara(), false)) {
            XLOGE("vdecode->Open %s failed(software)", path);
            muxtex.unlock();
            return false;
        }
    }
    if (!adecode || !adecode->Open(demux->GetAPara())) {
        XLOGE("adecode->Open %s failed!", path);
        muxtex.unlock();
        return false;
    }

    if (outPara.sample_rate <= 0)
        outPara = demux->GetAPara();
    if (!resample || !resample->Open(demux->GetAPara(), outPara)) {
        XLOGE("resample->Open %s failed!", path);
        muxtex.unlock();
        return false;
    }
    muxtex.unlock();
    return true;
}
```

- [ ] **Step 2: IPlayerProxy::Open 返回真实结果（`IPlayerProxy.cpp`）**

```cpp
bool IPlayerProxy::Open(const char *path) {
    mux.lock();
    bool ret = false;
    if (player) {
        ret = player->Open(path);
    }
    mux.unlock();
    return ret;
}
```

- [ ] **Step 3: FFResample::Resample 输出缓冲按重采样后样本数计算（`FFResample.cpp`）**

替换 `Resample()` 中 outSize 计算与 Alloc 部分：

```cpp
    XData out;
    AVFrame *frame = (AVFrame *) inData.data;
    // 重采样后实际输出样本数（含采样率变换与内部缓冲延迟）
    int outSamples = swr_get_out_samples(swr_ctx, frame->nb_samples);
    if (outSamples <= 0) {
        mux.unlock();
        return XData();
    }
    //size = 通道数 * 单通道样本数 * 样本字节大小
    int outSize = outChannels * outSamples * av_get_bytes_per_sample((AVSampleFormat) outFormat);
    if (outSize <= 0) {
        mux.unlock();
        return XData();
    }
    out.Alloc(outSize);
    uint8_t *outArr[2] = {0};
    outArr[0] = out.data;
    int len = swr_convert(swr_ctx, outArr, outSamples, (const uint8_t **) frame->data,
                          frame->nb_samples);
    if (len <= 0) {
        out.Drop();
        mux.unlock();
        return XData();
    }
    out.size = outChannels * len * av_get_bytes_per_sample((AVSampleFormat) outFormat);
    out.pts = inData.pts;
```

注意：原实现中 `out.Alloc(outSize)` 之后 size 即为估算值，现按实际转换样本数 `len` 修正 `out.size`，避免尾部垃圾数据送入 OpenSL。

- [ ] **Step 4: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 5: Commit**

```bash
git add app/src/main/cpp/IPlayer.cpp app/src/main/cpp/IPlayerProxy.cpp app/src/main/cpp/FFResample.cpp
git commit -m "fix: 硬解失败回退软解、Proxy 透传 Open 结果、重采样输出缓冲按实际样本数计算

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 5: 暂停/继续链路（XThread → 各线程 → OpenSL → IPlayer）

**Files:**
- Modify: `app/src/main/cpp/XThread.h`
- Modify: `app/src/main/cpp/XThread.cpp`
- Modify: `app/src/main/cpp/IDemux.cpp`
- Modify: `app/src/main/cpp/IDecode.cpp`
- Modify: `app/src/main/cpp/IAudioPlay.h`
- Modify: `app/src/main/cpp/SLAudioPlay.h`
- Modify: `app/src/main/cpp/SLAudioPlay.cpp`
- Modify: `app/src/main/cpp/IPlayer.h`
- Modify: `app/src/main/cpp/IPlayer.cpp`
- Modify: `app/src/main/cpp/IPlayerProxy.h`
- Modify: `app/src/main/cpp/IPlayerProxy.cpp`

- [ ] **Step 1: XThread 增加 isPause（`XThread.h`）**

```cpp
class XThread {
public:
    //启动线程
    virtual bool Start();

    //通过控制isExit安全停止线程(不一定成功)
    virtual void Stop();

    //暂停/继续（线程不退出，主循环空转）
    virtual void SetPause(bool isPause);

    //入口主函数
    virtual void Main() {};

protected:
    bool isExit = false;
    bool isRunnig = false;
    bool isPause = false;
private:
    void ThreadMain();
};
```

`XThread.cpp` 末尾增加：

```cpp
void XThread::SetPause(bool isPause) {
    this->isPause = isPause;
}
```

- [ ] **Step 2: demux/解码线程主循环支持暂停**

`IDemux.cpp` `Main()` 循环开头：

```cpp
void IDemux::Main() {
    XLOGD("idmx-thread");
    while (!isExit) {
        if (isPause) {
            XSleep(2);
            continue;
        }
        // ... 原逻辑不变
```

`IDecode.cpp` `Main()` 循环开头（`packetMutex.lock();` 之前）：

```cpp
    while (!isExit) {
        if (isPause) {
            XSleep(2);
            continue;
        }
        packetMutex.lock();
        // ... 原逻辑不变
```

- [ ] **Step 3: IAudioPlay/SLAudioPlay 暂停（OpenSL SetPlayState）**

`IAudioPlay.h` 增加虚函数声明（默认空实现）：

```cpp
    virtual bool StartPlay(XParameter out) = 0;

    //暂停/继续（子类实现具体播放器的暂停）
    virtual void SetPause(bool isPause) {}
```

`SLAudioPlay.h` 增加声明：

```cpp
    virtual bool StartPlay(XParameter out);
    virtual void SetPause(bool isPause);
```

`SLAudioPlay.cpp` 末尾增加实现：

```cpp
void SLAudioPlay::SetPause(bool isPause) {
    sl_mux.lock();
    if (audioPlayerItf && (*audioPlayerItf)) {
        if (isPause) {
            (*audioPlayerItf)->SetPlayState(audioPlayerItf, SL_PLAYSTATE_PAUSED);
        } else {
            (*audioPlayerItf)->SetPlayState(audioPlayerItf, SL_PLAYSTATE_PLAYING);
        }
    }
    sl_mux.unlock();
}
```

- [ ] **Step 4: IPlayer::SetPause 统一分发（`IPlayer.h` / `IPlayer.cpp`）**

`IPlayer.h` 增加声明：

```cpp
    virtual bool Start();
    virtual void SetPause(bool isPause);
```

`IPlayer.cpp` 末尾增加实现：

```cpp
void IPlayer::SetPause(bool isPause) {
    muxtex.lock();
    XThread::SetPause(isPause);
    if (demux)
        demux->SetPause(isPause);
    if (vdecode)
        vdecode->SetPause(isPause);
    if (adecode)
        adecode->SetPause(isPause);
    if (audioPlay)
        audioPlay->SetPause(isPause);
    muxtex.unlock();
}
```

- [ ] **Step 5: IPlayerProxy 透传（`IPlayerProxy.h` / `IPlayerProxy.cpp`）**

`IPlayerProxy.h` 增加声明：

```cpp
    virtual bool Start();

    virtual void SetPause(bool isPause);
```

`IPlayerProxy.cpp` 末尾增加：

```cpp
void IPlayerProxy::SetPause(bool isPause) {
    mux.lock();
    if (player) {
        player->SetPause(isPause);
    }
    mux.unlock();
}
```

- [ ] **Step 6: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 7: Commit**

```bash
git add app/src/main/cpp/
git commit -m "feat: 暂停/继续链路（XThread isPause + OpenSL SetPlayState）

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 6: Seek 实现（IDemux::Seek + 解码器 flush + IPlayer::Seek）

**Files:**
- Modify: `app/src/main/cpp/IDemux.h`
- Modify: `app/src/main/cpp/IDemux.cpp`
- Modify: `app/src/main/cpp/FFDemux.h`
- Modify: `app/src/main/cpp/FFDemux.cpp`
- Modify: `app/src/main/cpp/FFDecode.h`
- Modify: `app/src/main/cpp/FFDecode.cpp`
- Modify: `app/src/main/cpp/IPlayer.h`
- Modify: `app/src/main/cpp/IPlayer.cpp`
- Modify: `app/src/main/cpp/IPlayerProxy.h`
- Modify: `app/src/main/cpp/IPlayerProxy.cpp`

- [ ] **Step 1: IDemux 增加 Seek 接口与 EOF 空转（`IDemux.h` / `IDemux.cpp`）**

`IDemux.h` 增加声明：

```cpp
    virtual void Close() = 0;

    //seek 位置 pos 0.0~1.0
    virtual bool Seek(double pos) = 0;
```

`IDemux.cpp` `Main()` 的 EOF 处理从退出线程改为空转等待（支持 Seek 后续播）：

```cpp
void IDemux::Main() {
    XLOGD("idmx-thread");
    while (!isExit) {
        if (isPause) {
            XSleep(2);
            continue;
        }
        XData d = Read();
        if (d.size <= 0) {
            // EOF 或读取失败：空转等待 Seek，由 Close 的 isExit 终止线程
            XSleep(2);
            continue;
        }
        Notify(d);
    }
}
```

- [ ] **Step 2: FFDemux::Seek 实现（`FFDemux.h` / `FFDemux.cpp`）**

`FFDemux.h` 增加声明：

```cpp
    virtual void Close();

    //seek 位置 pos 0.0~1.0
    virtual bool Seek(double pos);
```

`FFDemux.cpp` 末尾增加：

```cpp
//seek 位置 pos 0.0~1.0
bool FFDemux::Seek(double pos) {
    ff_dmx_mutex.lock();
    if (!av_fmt_ctx) {
        ff_dmx_mutex.unlock();
        return false;
    }
    // av_fmt_ctx->duration 单位为 AV_TIME_BASE
    int64_t target = (int64_t) (pos * av_fmt_ctx->duration);
    int res = av_seek_frame(av_fmt_ctx, -1, target, AVSEEK_FLAG_BACKWARD);
    ff_dmx_mutex.unlock();
    if (res < 0) {
        XLOGE("FFDemux::Seek %f failed", pos);
        return false;
    }
    XLOGD("FFDemux::Seek %f success", pos);
    return true;
}
```

- [ ] **Step 3: FFDecode::Clear 增加 avcodec_flush_buffers（`FFDecode.h` / `FFDecode.cpp`）**

`FFDecode.h` 增加声明：

```cpp
    virtual void Close();

    //清理缓冲队列并 flush 解码器内部缓冲
    virtual void Clear();
```

`FFDecode.cpp` 末尾增加：

```cpp
void FFDecode::Clear() {
    IDecode::Clear();
    mux.lock();
    if (av_cdc_ctx) {
        avcodec_flush_buffers(av_cdc_ctx);
    }
    mux.unlock();
}
```

- [ ] **Step 4: IPlayer::Seek（`IPlayer.h` / `IPlayer.cpp`）**

`IPlayer.h` 增加声明：

```cpp
    virtual void SetPause(bool isPause);

    //seek 位置 pos 0.0~1.0
    virtual bool Seek(double pos);
```

`IPlayer.cpp` 末尾增加：

```cpp
bool IPlayer::Seek(double pos) {
    bool ret = false;
    muxtex.lock();
    if (demux) {
        // 先 seek 数据源，再清空各级缓冲，避免旧数据进入新位置
        ret = demux->Seek(pos);
    }
    if (ret) {
        if (vdecode)
            vdecode->Clear();
        if (adecode)
            adecode->Clear();
        if (audioPlay)
            audioPlay->Clear();
    }
    muxtex.unlock();
    return ret;
}
```

- [ ] **Step 5: IPlayerProxy 透传（`IPlayerProxy.h` / `IPlayerProxy.cpp`）**

`IPlayerProxy.h` 增加声明：

```cpp
    virtual void SetPause(bool isPause);

    virtual bool Seek(double pos);
```

`IPlayerProxy.cpp` 末尾增加：

```cpp
bool IPlayerProxy::Seek(double pos) {
    mux.lock();
    bool ret = false;
    if (player) {
        ret = player->Seek(pos);
    }
    mux.unlock();
    return ret;
}
```

- [ ] **Step 6: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 7: Commit**

```bash
git add app/src/main/cpp/
git commit -m "feat: Seek（demux av_seek_frame + 解码器 flush + 缓冲清空，EOF 空转支持续播）

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 7: 音视频同步改进与播放进度查询

**Files:**
- Modify: `app/src/main/cpp/IAudioPlay.h`
- Modify: `app/src/main/cpp/IAudioPlay.cpp`
- Modify: `app/src/main/cpp/SLAudioPlay.cpp`
- Modify: `app/src/main/cpp/IPlayer.h`
- Modify: `app/src/main/cpp/IPlayer.cpp`
- Modify: `app/src/main/cpp/IPlayerProxy.h`
- Modify: `app/src/main/cpp/IPlayerProxy.cpp`

- [ ] **Step 1: IAudioPlay 缓冲时长统计（`IAudioPlay.h`）**

```cpp
    //最大的队列缓冲
    int maxFrame = 100;

    long long pts = 0;

    //获取队列中未播放数据折算的时长(ms)
    long long GetBufferedMs();

protected:
    //由 StartPlay 设置，用于字节数折算时长
    int sampleRate = 44100;
    int channels = 2;
    long long bufferedBytes = 0;
```

（保留 Task 2 已改的 `long long pts`。）

- [ ] **Step 2: 维护 bufferedBytes（`IAudioPlay.cpp`）**

`Update()` 压入成功后累加：

```cpp
        frames.push_back(data);
        bufferedBytes += data.size;
        framesMutex.unlock();
        break;
```

`GetData()` 取出后扣减：

```cpp
        if (!frames.empty()) {
            d = frames.front();
            frames.pop_front();
            bufferedBytes -= d.size;
            if (bufferedBytes < 0)
                bufferedBytes = 0;
            framesMutex.unlock();
            pts = d.pts;
            return d;
        }
```

`Clear()` 中清零：

```cpp
void IAudioPlay::Clear() {
    framesMutex.lock();
    while (!frames.empty()) {
        frames.front().Drop();
        frames.pop_front();
    }
    bufferedBytes = 0;
    framesMutex.unlock();
}
```

文件末尾增加：

```cpp
long long IAudioPlay::GetBufferedMs() {
    long long bytesPerSec = (long long) sampleRate * channels * 2; // S16
    if (bytesPerSec <= 0)
        return 0;
    framesMutex.lock();
    long long bytes = bufferedBytes;
    framesMutex.unlock();
    return bytes * 1000 / bytesPerSec;
}
```

- [ ] **Step 3: SLAudioPlay::StartPlay 记录输出参数（`SLAudioPlay.cpp`）**

`StartPlay()` 中 `Close();` 与 `isExit = false;` 之后增加：

```cpp
    sampleRate = out.sample_rate;
    channels = out.channels;
```

- [ ] **Step 4: IPlayer::Main 同步基准扣除缓冲（`IPlayer.cpp`）**

```cpp
        // 获取音频的pts, 扣除已入队未播放的缓冲时长, 告诉视频
        long long apts = audioPlay->pts - audioPlay->GetBufferedMs();
        vdecode->syncPts = apts;
```

- [ ] **Step 5: IPlayer 进度查询（`IPlayer.h` / `IPlayer.cpp`）**

`IPlayer.h` 增加声明：

```cpp
    virtual bool Seek(double pos);

    //当前播放位置(ms)与总时长(ms)
    long long GetPlayMs();
    long long GetTotalMs();
```

`IPlayer.cpp` 末尾增加：

```cpp
long long IPlayer::GetPlayMs() {
    muxtex.lock();
    long long ret = 0;
    if (audioPlay)
        ret = audioPlay->pts;
    muxtex.unlock();
    return ret;
}

long long IPlayer::GetTotalMs() {
    muxtex.lock();
    long long ret = 0;
    if (demux)
        ret = demux->durationMs;
    muxtex.unlock();
    return ret;
}
```

- [ ] **Step 6: IPlayerProxy 透传（`IPlayerProxy.h` / `IPlayerProxy.cpp`）**

`IPlayerProxy.h` 增加声明：

```cpp
    virtual bool Seek(double pos);

    long long GetPlayMs();
    long long GetTotalMs();
```

`IPlayerProxy.cpp` 末尾增加：

```cpp
long long IPlayerProxy::GetPlayMs() {
    mux.lock();
    long long ret = 0;
    if (player) {
        ret = player->GetPlayMs();
    }
    mux.unlock();
    return ret;
}

long long IPlayerProxy::GetTotalMs() {
    mux.lock();
    long long ret = 0;
    if (player) {
        ret = player->GetTotalMs();
    }
    mux.unlock();
    return ret;
}
```

- [ ] **Step 7: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 8: Commit**

```bash
git add app/src/main/cpp/
git commit -m "feat: 音视频同步扣除音频缓冲时长；新增播放进度查询接口

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 8: XPlay SurfaceView 化与 native 窗口生命周期

**Files:**
- Modify: `app/src/main/java/com/knox/xplay/XPlay.java`
- Modify: `app/src/main/cpp/native-lib.cpp`

- [ ] **Step 1: 重写 XPlay.java 为纯 SurfaceView**

```java
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
```

- [ ] **Step 2: native-lib.cpp 窗口生命周期管理**

在 `native-lib.cpp` 文件顶部（`MEDIA_FILE` 定义处，该常量随 Task 9 删除）增加：

```cpp
static ANativeWindow *g_window = 0;
```

替换 `Java_com_knox_xplay_XPlay_native_1initView` 实现：

```cpp
extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1initView(JNIEnv *env, jobject instance, jobject surface) {
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
    g_window = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(g_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1closeView(JNIEnv *env, jobject instance) {
    IPlayerProxy::Get()->Close();
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
}
```

注意：`native_closeView` 的 JNI 函数名为 `Java_com_knox_xplay_XPlay_native_1closeView`（下划线转义规则）。

- [ ] **Step 3: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`（此时 Java 层新增的 static native 方法尚无实现，只要不调用即可编译通过；native 侧改动为新增函数）

- [ ] **Step 4: Commit**

```bash
git add app/src/main/java/com/knox/xplay/XPlay.java app/src/main/cpp/native-lib.cpp
git commit -m "refactor: XPlay 改为纯 SurfaceView，补 surfaceDestroyed 关闭流程与 ANativeWindow 释放

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 9: JNI API 重构（移除 JNI_OnLoad 自动播放）

**Files:**
- Modify: `app/src/main/cpp/native-lib.cpp`

- [ ] **Step 1: 重写 native-lib.cpp**

整体替换为：

```cpp
#include <jni.h>
#include <string>

#include "android/native_window_jni.h"
#include "IPlayerProxy.h"
#include "FFDecode.h"
#include "XLog.h"

static ANativeWindow *g_window = 0;

extern "C"
JNIEXPORT

jint JNI_OnLoad(JavaVM *vm, void *res) {
    IPlayerProxy::Get()->Init(vm);
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_knox_xplay_XPlay_native_1open(JNIEnv *env, jclass clazz, jstring path) {
    const char *cpath = env->GetStringUTFChars(path, 0);
    bool ret = IPlayerProxy::Get()->Open(cpath);
    env->ReleaseStringUTFChars(path, cpath);
    return ret ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_knox_xplay_XPlay_native_1start(JNIEnv *env, jclass clazz) {
    return IPlayerProxy::Get()->Start() ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1setPause(JNIEnv *env, jclass clazz, jboolean pause) {
    IPlayerProxy::Get()->SetPause(pause == JNI_TRUE);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1seek(JNIEnv *env, jclass clazz, jdouble pos) {
    IPlayerProxy::Get()->Seek(pos);
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_knox_xplay_XPlay_native_1getProgress(JNIEnv *env, jclass clazz) {
    jlongArray result = env->NewLongArray(2);
    if (!result)
        return 0;
    jlong vals[2];
    vals[0] = IPlayerProxy::Get()->GetPlayMs();
    vals[1] = IPlayerProxy::Get()->GetTotalMs();
    env->SetLongArrayRegion(result, 0, 2, vals);
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1initView(JNIEnv *env, jobject instance, jobject surface) {
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
    g_window = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(g_window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_knox_xplay_XPlay_native_1closeView(JNIEnv *env, jobject instance) {
    IPlayerProxy::Get()->Close();
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = 0;
    }
}
```

（Task 8 Step 2 中临时添加的 initView/closeView 实现被本步骤整体替换覆盖，属预期。）

同时删除：硬编码 `MEDIA_FILE` 常量、`TextObs` 测试类、`stringFromJNI` 实现（Java 侧声明在 Task 10 删除）、不再需要的头文件包含（XEGL/XShader/GLVideoView/FFResample/IAudioPlay/SLAudioPlay/FFDemux）。

`IPlayerProxy.h` 中 `Init(void *vm)` 签名不变，无需改动。

- [ ] **Step 2: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`（若 Java 层 MainActivity 仍引用 `stringFromJNI` 会编译失败——此时先注释掉，Task 10 会重写 MainActivity）

- [ ] **Step 3: Commit**

```bash
git add app/src/main/cpp/native-lib.cpp
git commit -m "refactor: JNI 改为显式 open/start/pause/seek/progress API，移除 JNI_OnLoad 自动播放

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 10: MainActivity + 控制栏 UI + SAF 文件选择

**Files:**
- Modify: `app/src/main/res/layout/activity_main.xml`
- Modify: `app/src/main/java/com/knox/xplay/MainActivity.java`
- Delete: `app/src/main/java/com/knox/xplay/PermissionHelper.java`
- Modify: `app/src/main/AndroidManifest.xml`

- [ ] **Step 1: 重写布局 `activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<FrameLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent"
    android:layout_height="match_parent">

    <com.knox.xplay.XPlay
        android:id="@+id/xplay"
        android:layout_width="match_parent"
        android:layout_height="match_parent" />

    <LinearLayout
        android:layout_width="match_parent"
        android:layout_height="wrap_content"
        android:layout_gravity="bottom"
        android:background="#66000000"
        android:gravity="center_vertical"
        android:orientation="horizontal"
        android:padding="8dp">

        <Button
            android:id="@+id/btnPick"
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="选择文件" />

        <Button
            android:id="@+id/btnPlay"
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:text="暂停" />

        <SeekBar
            android:id="@+id/seekBar"
            android:layout_width="0dp"
            android:layout_height="wrap_content"
            android:layout_weight="1"
            android:max="1000" />

        <TextView
            android:id="@+id/txtTime"
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            android:textColor="#FFFFFF"
            android:text="00:00/00:00" />
    </LinearLayout>
</FrameLayout>
```

- [ ] **Step 2: 重写 MainActivity.java**

```java
package com.knox.xplay;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    private static final int REQ_PICK_VIDEO = 1;

    private XPlay xPlay;
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

        xPlay = (XPlay) findViewById(R.id.xplay);
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
            // 播放完成：自动回零并暂停，等待用户 seek 或重新选择
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
```

注意：播放完成处理采用"置为暂停"而非自动 Seek(0)，避免循环重播；用户可拖动 SeekBar 或点"继续"重新观看。

- [ ] **Step 3: 删除 PermissionHelper.java，清理 Manifest**

```bash
git rm app/src/main/java/com/knox/xplay/PermissionHelper.java
```

`AndroidManifest.xml` 删除以下两行（SAF 无需存储权限），保留 `INTERNET`：

```xml
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"/>
```

同时删除不再声明的 `REQUEST_INSTALL_PACKAGES`：

```xml
    <uses-permission android:name="android.permission.REQUEST_INSTALL_PACKAGES"/>
```

- [ ] **Step 4: 编译验证**

Run: `./gradlew assembleDebug`
Expected: `BUILD SUCCESSFUL`

- [ ] **Step 5: Commit**

```bash
git add app/src/main/
git commit -m "feat: SAF 文件选择 + 底部控制栏（播放/暂停、SeekBar、进度时间），移除存储权限

Co-Authored-By: Claude Code <noreply@anthropic.com>"
```

---

### Task 11: 真机验收（手动 checklist）

**Files:** 无代码改动

- [ ] **Step 1: 完整构建并安装**

Run: `./gradlew assembleDebug installDebug`
Expected: 安装成功

- [ ] **Step 2: 手动验收 checklist（逐项真机/模拟器确认）**

1. 首次启动 → 不黑屏，显示控制栏（不自动播放）
2. 点"选择文件" → 系统选择器弹出 → 选择视频 → 有图有声播放
3. 播放中点"暂停" → 声音停、画面定格；点"继续" → 音视频同步恢复
4. 拖动 SeekBar 到中部 → 从对应关键帧位置继续播放，短暂花屏可接受（AVSEEK_FLAG_BACKWARD 预期行为），声音同步
5. 拖到底部附近 → 播到结尾进度约 100%，自动暂停
6. 播完后点"继续"或拖回开头 → 能继续播放
7. 播放中切后台 → 回前台 → 不崩溃（surface 重建后可重新选择文件播放）
8. 连续选择 3 个不同文件播放 → 无崩溃、无明显内存增长
9. logcat 过滤 XPLAY → 无持续错误日志

- [ ] **Step 3: 验收通过后打 tag**

```bash
git tag -a v1.0-xplay-complete -m "修 bug + Seek/暂停/进度UI/SAF 文件选择完成"
```

---

## Self-Review 记录

- **Spec 覆盖**：设计文档第 1 部分（bug 修复）→ Task 2/3/4；第 2 部分（暂停/同步/Seek/进度）→ Task 5/6/7；第 3 部分（SurfaceView、SAF、UI、JNI、仓库卫生）→ Task 1/8/9/10；第 4 部分（验证）→ Task 11。全覆盖。
- **类型一致性**：`SetPause(bool)`（XThread/IPlayer/IPlayerProxy/IAudioPlay/SLAudioPlay）、`Seek(double)`（IDemux/FFDemux/IPlayer/IPlayerProxy）、`GetPlayMs()/GetTotalMs()/GetBufferedMs()` 返回 `long long`、JNI 层 `native_setPause(boolean)`/`native_seek(double)`/`native_getProgress()→long[]`，前后一致。
- **已知取舍**：Task 9 Step 2 中 MainActivity 旧代码可能暂时引用 `stringFromJNI`，允许临时注释，Task 10 重写后解决。
