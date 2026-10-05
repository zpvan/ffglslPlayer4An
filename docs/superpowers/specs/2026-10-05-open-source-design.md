# XPlay 开源化设计（教学参考项目）

日期：2026-10-05
状态：已确认
方案：方案 A —— 可跑通的教学项目
文档语言：中英双语并行（README.md 英文 + README_CN.md 中文）

## 背景

本工程是基于 FFmpeg 3.4 + OpenGL ES 2.0 + OpenSL ES 的 Android 视频播放器（XPlay 教程的完整实现与完善）。当前状态：无 README、无 LICENSE、FFmpeg 3.4 预编译二进制（12MB，仅 armeabi-v7a）直接入库、无构建文档。开源定位为**教学参考项目**，第一价值是"学习者 clone 下来能跑通、能看懂架构"。

## 目标

1. 中英双语文档体系（README、架构文档、FFmpeg 编译指南）
2. 许可证与 LGPL 合规（MIT + NOTICE）
3. arm64-v8a 支持（现代手机必需）
4. 效果素材（GIF/截图）

## 非目标

- 不做 CI、Issue/PR 模板、CONTRIBUTING、CHANGELOG 等社区化设施（YAGNI，有协作者时再加）
- 不升级 FFmpeg 大版本（保持与教程一致的 3.4 API）
- 不重构代码结构

---

## 第 1 部分：文档体系

### README.md（英文）/ README_CN.md（中文）

两文件结构对称，内容一一对应：

1. 项目简介：基于 FFmpeg + OpenGL ES 2.0 + OpenSL ES 的 Android 视频播放器教学项目（XPlay 教程的完整实现与完善）
2. 效果展示：GIF 录屏 + 截图（`docs/images/`）
3. 特性列表：FFmpeg 软解 / MediaCodec 硬解（失败自动回退软解）、YUV420P/NV12/NV21 GLSL shader 渲染、OpenSL ES 音频、Seek、暂停/继续、SAF 文件选择（免存储权限）
4. 架构图：数据流 pipeline（Demux → Decode → VideoView / Resample → AudioPlay）
5. 构建指南（精确版本）：
   - JDK 17（`export JAVA_HOME=...`）
   - Android SDK 35（cmdline-tools）、NDK 29、CMake 3.22.1（sdkmanager 安装）
   - Gradle 8.11.1（wrapper 自带）
   - `local.properties` 配置 `sdk.dir`
   - `./gradlew assembleDebug`
6. 运行指南：clone → 构建安装 → "选择文件"播视频
7. 文档索引：架构文档、FFmpeg 编译指南
8. 许可证说明（自有代码 MIT / FFmpeg 部分 LGPL，见 NOTICE）
9. 致谢：原 XPlay 教程

### docs/architecture.md（双语章节）

- 观察者模式数据流图（ASCII 或 mermaid）
- 接口类职责表：IDemux / IDecode / IResample / IVideoView / IAudioPlay / IPlayer / IPlayerProxy / IPlayerBuilder
- 线程模型：demux 线程、视频解码线程、音频解码线程、IPlayer 同步线程、OpenSL 回调线程
- 音视频同步机制：`vdecode->syncPts = audioPlay->pts - GetBufferedMs()`
- Seek 与 Close 时序
- Builder + Proxy 模式说明

### docs/build-ffmpeg.md（双语章节）

- FFmpeg 3.4.x 源码获取
- NDK clang 交叉编译脚本（armeabi-v7a / arm64-v8a 两个 target 各一段完整脚本）
- 关键 configure 参数解释（--enable-shared --disable-static --disable-programs 等）
- 产物布局与 `app/libs/` 目录对应关系
- 头文件（app/include/）来源说明

## 第 2 部分：许可证与 LGPL 合规

- `LICENSE`：自有代码 MIT License（版权持有者 zpvan）
- `NOTICE`：声明仓库内含 FFmpeg 3.4 预编译二进制；FFmpeg 默认配置为 LGPL 2.1+；本项目以动态链接（.so）方式使用，并通过 `docs/build-ffmpeg.md` 提供对应源码版本与编译方法（满足 LGPL "可替换库"要求）；再分发者需遵守相同义务
- README 许可证章节显式说明该结构

## 第 3 部分：arm64-v8a 支持

- 按 `docs/build-ffmpeg.md` 的脚本，在本机用 NDK 29 的 clang 实际编译 arm64-v8a 的 6 个 `.so`（avcodec/avfilter/avformat/avutil/swresample/swscale），放入 `app/libs/arm64-v8a/` 并入库
- `app/build.gradle`：`ndk { abiFilters "armeabi-v7a", "arm64-v8a" }`（CMake 的 `${ANDROID_ABI}` 已参数化，CMakeLists.txt 无需改动）
- 验证：`./gradlew assembleDebug` 通过，APK 内同时含两个 ABI 的 so
- 风险预案：若 FFmpeg 3.4 与 NDK 29 不兼容，改用 NDK r21 等旧版编译（脚本中注明），产物仍入库

## 第 4 部分：效果素材

- 真机/模拟器录屏转 GIF（≤5MB）存入 `docs/images/`，README 引用；附控制栏截图
- 若无可用设备：先用静态截图（可从布局渲染），GIF 留为手动步骤并在 README 中引用预期路径

## 验证

1. `./gradlew assembleDebug` 通过，APK 含 armeabi-v7a + arm64-v8a 双 ABI
2. 全新目录 `git clone` 后按 README 构建指南可复现构建（逐步核对文档命令）
3. arm64 设备/模拟器安装运行，选择文件可播放
4. 文档内部链接（README → docs/、LICENSE/NOTICE）全部有效
