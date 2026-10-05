# XPlay 架构文档 / Architecture

## 中文

### 1. 数据流 Pipeline

```
        ┌────────────────────────────────────────────────────────────────┐
        │                     IPlayer（同步线程）                          │
        │       vdecode->syncPts = audioPlay->pts - GetBufferedMs()       │
        └────────────────────────────────────────────────────────────────┘

 文件/fd ──> FFDemux ──Notify(pkt)──┬──> FFDecode(视频) ──> GLVideoView ──> XTexture → XShader → XEGL ──> 屏幕
 (demux 线程)        (观察者模式)     └──> FFDecode(音频) ──> FFResample ──> SLAudioPlay ──> OpenSL ES ──> 扬声器
```

数据以 `XData` 为载体，通过观察者模式（`IObserver::AddObs` / `Notify`）沿链路传递；解封装、视频解码、音频解码各运行在独立线程，由队列深度形成背压（生产者阻塞等待）。

### 2. 类职责

| 类 | 职责 |
|---|---|
| `XData` | 数据包/帧的统一载体（AVPacket 或裸内存，靠 `dataType` 区分释放方式），携带 pts/宽高/格式 |
| `XThread` | 线程基类：`Start/Stop/SetPause`，子类实现 `Main()` |
| `IObserver` | 观察者 + 主体：`AddObs`/`Notify`/`Update`，继承 XThread |
| `IDemux` / `FFDemux` | 解封装接口 / FFmpeg 实现：`Open/Read/Seek/Close`，`ff_dmx_mutex` 保护 `AVFormatContext` |
| `IDecode` / `FFDecode` | 解码接口（Future 模型：`SendPacket` + `RecvFrame`）/ FFmpeg 实现，支持硬解（h264_mediacodec）与软解回退 |
| `IResample` / `FFResample` | 音频重采样接口 / swresample 实现（统一输出 S16） |
| `IVideoView` / `GLVideoView` | 渲染接口 / OpenGL 实现，持有 `XTexture` |
| `XTexture` / `XShader` / `XEGL` | 纹理（YUV420P/NV12/NV21）、GLSL shader（YUV→RGB）、EGL 封装（单例） |
| `IAudioPlay` / `SLAudioPlay` | 音频播放接口（帧队列 + `GetBufferedMs`）/ OpenSL ES 实现（回调线程驱动 `GetData`） |
| `IPlayer` | 门面：组装好的播放链路 + 同步线程 + `Open/Start/Close/Seek/SetPause/GetPlayMs/GetTotalMs` |
| `IPlayerBuilder` / `FFPlayerBuilder` | 构建者：创建组件并连接观察者链 |
| `IPlayerProxy` | 单例代理：对 `IPlayer` 的所有调用加锁（Java 层只接触它） |

### 3. 线程模型

| 线程 | 入口 | 说明 |
|---|---|---|
| demux 线程 | `IDemux::Main` | 循环 `Read()` → `Notify(pkt)`；EOF 空转等待 Seek |
| 视频解码线程 | `IDecode::Main`（vdecode） | 消费包队列 → 解码 → Notify 给 GLVideoView |
| 音频解码线程 | `IDecode::Main`（adecode） | 同上，Notify 给 FFResample |
| IPlayer 同步线程 | `IPlayer::Main` | 每 2ms 把音频进度写入 `vdecode->syncPts` |
| OpenSL 回调线程 | `SLAudioPlay::PlayCall` | 缓冲播放完成时回调，阻塞式 `GetData()` 取下一帧 |

锁：`ff_dmx_mutex`（FFDemux）、`packetMutex`（IDecode 包队列）、`framesMutex`（IAudioPlay 帧队列）、`sl_mux`（OpenSL 对象）、`muxtex`（IPlayer 成员保护）、`mux`（IPlayerProxy）。

### 4. 音视频同步

以音频为主时钟：视频解码线程在每轮循环比较 `syncPts`（音频当前播放位置，已扣除音频队列中未播放的缓冲时长）与本解码器已解出帧的 `pts`；`syncPts < pts` 说明视频超前，休眠 1ms 等待；否则继续解码渲染。

### 5. Seek 时序

`IPlayer::Seek(pos)`：`demux->Seek(pos)`（`av_seek_frame` + `AVSEEK_FLAG_BACKWARD`，先 seek 数据源）→ `vdecode/adecode->Clear()`（清空包队列 + `avcodec_flush_buffers`）→ `audioPlay->Clear()`（清空帧队列）。demux 线程在 EOF 时空转不退出，因此 Seek 后自然续播。

### 6. Close 时序

`IPlayer::Close`：停同步线程 → 停 demux/解码线程 → 清各缓冲队列 → `audioPlay->Close()`（先置 `isExit` 唤醒可能空转在 `GetData()` 的 OpenSL 回调，再 Destroy 并置空所有 OpenSL 对象）→ `videoView/vdecode/adecode/demux->Close()`。

### 7. JNI API（`XPlay.java`）

| 方法 | 说明 |
|---|---|
| `native_open(String path)` | 打开（支持 `/proc/self/fd/N` 形式的 SAF fd 路径） |
| `native_start()` | 启动播放链路 |
| `native_setPause(boolean)` | 暂停/继续 |
| `native_seek(double pos)` | 0.0~1.0 拖动 |
| `native_getProgress()` | 返回 `long[]{curMs, totalMs}` |
| `native_initView(Object surface)` | surfaceCreated 时传入渲染窗口 |
| `native_closeView()` | surfaceDestroyed 时关闭并释放 ANativeWindow |

---

## English

### 1. Data-flow pipeline

```
        ┌────────────────────────────────────────────────────────────────┐
        │                     IPlayer (sync thread)                       │
        │       vdecode->syncPts = audioPlay->pts - GetBufferedMs()       │
        └────────────────────────────────────────────────────────────────┘

 file/fd ──> FFDemux ──Notify(pkt)──┬──> FFDecode(video) ──> GLVideoView ──> XTexture → XShader → XEGL ──> screen
 (demux thread)     (observer)      └──> FFDecode(audio) ──> FFResample ──> SLAudioPlay ──> OpenSL ES ──> speaker
```

Data travels as `XData` through the observer chain (`IObserver::AddObs` / `Notify`). Demuxing, video decoding and audio decoding each run on their own thread; queue depth provides backpressure (producers block while waiting).

### 2. Class responsibilities

| Class | Responsibility |
|---|---|
| `XData` | Unified packet/frame carrier (AVPacket or raw memory, distinguished by `dataType`), carries pts/width/height/format |
| `XThread` | Thread base: `Start/Stop/SetPause`, subclasses implement `Main()` |
| `IObserver` | Observer + subject: `AddObs`/`Notify`/`Update`, extends XThread |
| `IDemux` / `FFDemux` | Demux interface / FFmpeg impl: `Open/Read/Seek/Close`; `ff_dmx_mutex` guards the `AVFormatContext` |
| `IDecode` / `FFDecode` | Decode interface (future model: `SendPacket` + `RecvFrame`) / FFmpeg impl, hardware decode (h264_mediacodec) with software fallback |
| `IResample` / `FFResample` | Audio resample interface / swresample impl (unified S16 output) |
| `IVideoView` / `GLVideoView` | Render interface / OpenGL impl, owns an `XTexture` |
| `XTexture` / `XShader` / `XEGL` | Textures (YUV420P/NV12/NV21), GLSL shaders (YUV→RGB), EGL wrapper (singleton) |
| `IAudioPlay` / `SLAudioPlay` | Audio playback interface (frame queue + `GetBufferedMs`) / OpenSL ES impl (callback thread drives `GetData`) |
| `IPlayer` | Facade: assembled pipeline + sync thread + `Open/Start/Close/Seek/SetPause/GetPlayMs/GetTotalMs` |
| `IPlayerBuilder` / `FFPlayerBuilder` | Builder: creates components and wires the observer chain |
| `IPlayerProxy` | Singleton proxy: mutex-guards all `IPlayer` calls (the only class the Java layer touches) |

### 3. Threading model

| Thread | Entry | Notes |
|---|---|---|
| demux thread | `IDemux::Main` | Loops `Read()` → `Notify(pkt)`; idles at EOF waiting for Seek |
| video decode thread | `IDecode::Main` (vdecode) | Consumes packet queue → decode → Notify GLVideoView |
| audio decode thread | `IDecode::Main` (adecode) | Same, Notify FFResample |
| IPlayer sync thread | `IPlayer::Main` | Writes audio progress into `vdecode->syncPts` every 2ms |
| OpenSL callback thread | `SLAudioPlay::PlayCall` | Fired when a queued buffer finishes; blocks in `GetData()` for the next frame |

Mutexes: `ff_dmx_mutex` (FFDemux), `packetMutex` (IDecode packet queue), `framesMutex` (IAudioPlay frame queue), `sl_mux` (OpenSL objects), `muxtex` (IPlayer members), `mux` (IPlayerProxy).

### 4. Audio/video sync

Audio is the master clock. Each video-decode loop compares `syncPts` (the current audio playback position, minus the queued-but-unplayed buffered duration) against the pts of frames it has already decoded: if `syncPts < pts` the video is ahead and the thread sleeps 1ms; otherwise it keeps decoding and rendering.

### 5. Seek sequence

`IPlayer::Seek(pos)`: `demux->Seek(pos)` (`av_seek_frame` with `AVSEEK_FLAG_BACKWARD`, seek the source first) → `vdecode/adecode->Clear()` (drop queued packets + `avcodec_flush_buffers`) → `audioPlay->Clear()` (drop queued frames). The demux thread idles at EOF instead of exiting, so playback resumes naturally after a Seek.

### 6. Close sequence

`IPlayer::Close`: stop sync thread → stop demux/decode threads → clear all queues → `audioPlay->Close()` (set `isExit` first to wake the OpenSL callback potentially spinning in `GetData()`, then Destroy and NULL all OpenSL objects) → `videoView/vdecode/adecode/demux->Close()`.

### 7. JNI API (`XPlay.java`)

| Method | Description |
|---|---|
| `native_open(String path)` | Open (supports SAF fd paths like `/proc/self/fd/N`) |
| `native_start()` | Start the pipeline |
| `native_setPause(boolean)` | Pause/resume |
| `native_seek(double pos)` | Seek, 0.0~1.0 |
| `native_getProgress()` | Returns `long[]{curMs, totalMs}` |
| `native_initView(Object surface)` | Hand the render window in surfaceCreated |
| `native_closeView()` | Close and release the ANativeWindow in surfaceDestroyed |
