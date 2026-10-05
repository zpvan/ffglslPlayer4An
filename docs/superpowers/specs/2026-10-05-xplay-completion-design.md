# XPlay 播放器完善设计：修 Bug + 补功能 + 架构隐患重构

日期：2026-10-05
状态：已确认
方案：演进式重构（保留教程的观察者链 + XThread + Builder 架构）

## 背景

本工程是 XPlay 教程项目（FFmpeg 解封装/解码 → OpenGL ES 2.0 Shader(YUV→RGB) 渲染 → OpenSL ES 音频播放）。当前 commit 历史进展到 "PlayerProxy + 关闭流程 + 真机运行通过"，核心播放链路已打通，但存在若干 bug，且缺少 Seek、暂停、进度 UI、播放完成处理等功能。

## 目标

1. 修复全部已确认的严重 bug 与主要中等 bug
2. 补齐功能：Seek 拖动、暂停/继续、进度条 UI、播放完成回调与生命周期处理
3. 消除架构隐患：双 EGL surface 冲突、音视频同步固定偏移
4. 播放源从硬编码路径改为 SAF 文件选择器

## 非目标

- 不重写线程模型（不引入 condition_variable pipeline）
- 不引入 MediaCodec 硬解之外的解码器变更（仅做硬解失败回退软解）
- 不支持网络流媒体的专项优化（保留现有能力即可）

---

## 第 1 部分：Native 层 Bug 修复

### 内存与 UB

| 位置 | 问题 | 修复 |
|---|---|---|
| `XData::Drop` | `delete data` 应为数组释放 | 改为 `delete[] data` |
| `SLAudioPlay::~SLAudioPlay` | `delete buf` 同上 | 改为 `delete[] buf` |
| `FFResample::Open` | `new uint8_t[4800*4*2]` 分配后未使用未释放 | 删除该分配 |
| `SLAudioPlay::PlayCall` | `memcpy(buf, ...)` 未检查 1MB 缓冲上限 | 超限则丢弃该帧并记日志 |
| `IDecode::mediaType` | 未初始化，Open 失败后 `Update()` 行为未定义 | 初始化为 -1 |
| pts 类型 | `int` 截断 int64 pts | `XData::pts`、`IDecode::pts/syncPts`、`IAudioPlay::pts` 改为 `long long` |

### OpenSL 生命周期

- `SLAudioPlay::Close` 销毁后将 6 个静态指针（engineObject、engineItf、pcmQueue、audioPlayer、audioPlayerItf、outputMixObject）置 NULL，支持重复 Close 与二次 StartPlay
- Close 顺序：先置 `isExit=true` + `Clear()`（唤醒可能空转在 `GetData()` 的 OpenSL 回调线程），再 Destroy OpenSL 对象，消除死锁窗口；`StartPlay` 开头复位 `isExit=false`
- `PlayCall` 中 `GetData()` 返回空时不 Enqueue

### 错误处理与回退

- `IPlayer::Open`：硬解失败自动回退软解（`vdecode->Open(para, true)` 失败则 `Open(para, false)`）；adecode/resample 失败返回 false
- `IPlayerProxy::Open`：返回 `player->Open()` 的真实结果
- `FFDemux::Read`：判空 `av_fmt_ctx`
- `FFDemux::Open`：`avformat_find_stream_info` 失败即返回 false
- `FFResample::Resample`：输出样本数改用 `swr_get_out_samples()` 计算缓冲大小，消除重采样截断
- `XEGL::Init`：`if (config == EGL_NO_CONTEXT)` 修正为 `context`

---

## 第 2 部分：线程、同步与生命周期重构

### SurfaceView 化

- `XPlay.java` 从 `GLSurfaceView` 改为纯 `SurfaceView implements SurfaceHolder.Callback`，删除 `GLSurfaceView.Renderer` 空实现，EGL 完全由 native 层持有，消除同一 ANativeWindow 双 EGL surface 冲突
- `surfaceCreated` → `native_initView(surface)`（保留）
- `surfaceDestroyed` → 新增 `native_closeView()` → `IPlayerProxy::Close()`，并 `ANativeWindow_release` 释放引用

### 暂停/继续

- `XThread` 基类增加 `isPause` 成员与 `SetPause(bool)`
- demux / 解码线程主循环开头检查 `isPause`（暂停时空转，线程不退出）
- `IAudioPlay` 增加 `SetPause(bool)` → OpenSL `SetPlayState(SL_PLAYSTATE_PAUSED/PLAYING)`
- 画面定格由 SurfaceFlinger 保留 surface 内容自然实现
- `IPlayer::SetPause(bool)` 统一分发到各组件

### 音视频同步改进

- `IAudioPlay` 增加 `GetBufferedMs()`：队列中未播放数据字节数折算为时长
- `IPlayer::Main` 同步基准改为 `vdecode->syncPts = audioPlay->pts - audioPlay->GetBufferedMs()`，消除固定偏移
- 视频侧保留现有等待/追赶逻辑

### Seek

- `IDemux` 增加 `virtual bool Seek(double pos)`（0.0~1.0）；`FFDemux::Seek` 实现：`av_seek_frame(av_fmt_ctx, -1, (int64_t)(pos * av_fmt_ctx->duration), AVSEEK_FLAG_BACKWARD)`（`av_fmt_ctx->duration` 单位为 `AV_TIME_BASE`），`ff_dmx_mutex` 保护
- `IPlayer::Seek(double pos)` 流程：
  1. `demux->Seek(pos)`
  2. `vdecode->Clear(); adecode->Clear(); audioPlay->Clear()`
  3. `FFDecode::Clear()` 中补充 `avcodec_flush_buffers(av_cdc_ctx)`
  4. pts/syncPts 随 Clear 复位
- `IDemux::Main` EOF 行为从"退出线程"改为"空转等待"，使 Seek 后可直接续播（重播 = Seek(0)）

### 播放完成

- `IPlayer` 增加 `GetPlayMs()`（取 `audioPlay->pts`）与 `GetTotalMs()`（取 `demux->durationMs`）
- 完成判定由 Java 层轮询：`curMs >= totalMs - 500`（容差 500ms，因 EOF 后 `audioPlay->pts` 停在最后一帧，可能略小于总时长）视为播放完成，UI 切换为可重播状态（重播 = Seek(0) + 继续播放），不引入额外状态机

---

## 第 3 部分：Java / UI 层

### 文件选择（SAF）

- `MainActivity` 通过 `ACTION_OPEN_DOCUMENT`（`video/*`）启动系统选择器
- `onActivityResult` 中 `ContentResolver.openFileDescriptor(uri, "r")` 获取 `ParcelFileDescriptor`，Java 层持有引用直到播放结束
- 将 `/proc/self/fd/N` 路径传给 native 的 `avformat_open_input`（本地文件可 Seek）
- SAF 不需要存储权限，`MainActivity` 中动态权限逻辑随之移除；`PermissionHelper.java` 删除（原 `remove(mRequestCode)` key 错误与回调断链问题随之消解），manifest 中 `READ/WRITE_EXTERNAL_STORAGE` 权限移除，`INTERNET` 保留

### native API 重构

- 移除 `JNI_OnLoad` 中的自动 `Open/Start` 与硬编码路径
- `XPlay.java` 提供静态 native 方法：
  - `native_open(String path)` → `IPlayerProxy::Open`，返回 boolean
  - `native_start()` → `IPlayerProxy::Start`
  - `native_playOrPause()` → `IPlayer::SetPause` 取反
  - `native_seek(double pos)` → `IPlayer::Seek`
  - `native_getProgress()` → 返回 `long[]{curMs, totalMs}`
  - `native_initView(Object surface)`（保留）/ `native_closeView()`（新增）

### UI

- `activity_main.xml`：FrameLayout 内 SurfaceView 全屏 + 底部控制栏 LinearLayout（选择文件按钮、播放/暂停按钮、SeekBar、当前时间/总时长文本）
- Handler 每 500ms 轮询 `native_getProgress()` 刷新 SeekBar 与时间文本
- SeekBar `onStopTrackingTouch` 时调 `native_seek(progress / max)`

### 仓库卫生

- `.gitignore` 增加 `.idea/`、`.cxx/`、`.settings/`、`local.properties` 等
- `git rm -r --cached` 移除已跟踪的 IDE 与 CMake 编译产物

---

## 第 4 部分：验证

无单元测试基础（native 依赖设备），采用：

1. 编译验证：`./gradlew assembleDebug` 通过
2. 真机/模拟器手动 checklist：
   - 选择文件 → 播放有图有声
   - 拖动 SeekBar → 画面/声音从对应关键帧位置继续，无长时间花屏
   - 暂停 → 声音停、画面定格；继续 → 音视频同步恢复
   - 播放到结尾 → 进度 100%，可点击重播
   - 播放中切后台再回来 → 不崩溃
   - 连续打开多个不同文件 → 无崩溃、无内存持续增长
   - 硬解不可用场景回退软解正常播放

## 风险与权衡

- SAF 的 `/proc/self/fd/N` 方案要求 Java 层持有 fd 至播放结束，由 `MainActivity` 成员变量保证
- EOF 空转等待会使 demux 线程常驻，由 `IPlayer::Close` 的 `isExit` 机制负责终止
- Seek 到非关键帧位置会短暂花屏直至下一个 IDR 帧，属 `AVSEEK_FLAG_BACKWARD` 的预期行为
