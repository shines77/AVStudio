# AVStudio

## 简介

一套音视频处理工具集，包括采集、编码、推流、音视频播放器、音视频服务器等。

包含以下工具：

- AVStreaming: 音视频采集、编码、推流。
- AVPlayer: 音视频播放器。
- AVServer: 音视频服务器。
- AVMonitor: 视频监控管理。

## Git

- [https://gitee.com/shines77/AVStudio]

- [https://github.com/shines77/AVStudio]

## 生成测试文件

生成 H.264 视频文件：

```bash
ffmpeg -f lavfi -i testsrc=duration=10:size=640x480:rate=30 -c:v libx264 -pix_fmt yuv420p input_video.h264
```

生成 AAC 音频文件：

```bash
ffmpeg -f lavfi -i "sine=frequency=440:duration=10" -c:a aac input_audio.aac
```

## 其他测试

[CmdParser] 测试命令行参数：

```bash
## Windows
.\AVMuxer.exe -v input_video.h264 -a input_audio.aac -o output_264.mp4

## Linux
./AVMuxer.exe -v input_video.h264 -a input_audio.aac -o output_264.mp4
```
