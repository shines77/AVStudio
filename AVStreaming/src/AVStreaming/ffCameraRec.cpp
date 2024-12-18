#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <stdio.h>
#include <windows.h>

#include "global.h"
#include "AVConsole.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/log.h>
}

int transform_video_format(AVFrame * src_frame, AVPixelFormat src_format,
                           AVFrame * dest_frame, AVPixelFormat dest_format)
{
    if (src_frame->format != src_format) {
        console.error("Input frame is not in YUV422P format");
        return -1;
    }

    if (dest_frame->format != dest_format) {
        console.error("Output frame is not in YUV420P format");
        return -1;
    }

    // 检查输入和输出帧的宽度和高度是否一致
    if (src_frame->width != dest_frame->width || src_frame->height != dest_frame->height) {
        console.error("Input and output frame dimensions do not match");
        return -1;
    }

    // 创建 SwsContext 用于格式转换
    SwsContext * sws_ctx = sws_getContext(
        src_frame->width, src_frame->height, src_format,
        dest_frame->width, dest_frame->height, dest_format,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        console.error("Could not initialize the conversion context");
        return -1;
    }

    // 执行格式转换
    int ret = sws_scale(sws_ctx,
                        (const uint8_t * const *)src_frame->data, src_frame->linesize,
                        0, src_frame->height,
                        dest_frame->data, dest_frame->linesize);

    // 释放 SwsContext
    sws_freeContext(sws_ctx);
    return ret;
}

int avcodec_encode_video_frame(AVFormatContext * outputFormatCtx,
                               AVCodecContext * videoCodecCtx,
                               AVStream * inputVideoStream,
                               AVStream * outputVideoStream,
                               AVFrame * srcFrame, int frameIndex,
                               AVPixelFormat src_format,
                               AVPixelFormat dest_format)
{
    int ret;
    // 创建一个 YUV420P 格式的 AVFrame
    fSmartPtr<AVFrame> destFrame = av_frame_alloc();
    if (destFrame == NULL) {
        console.error("destFrame 创建失败");
        return -1;
    }

    ret = av_frame_copy_props(destFrame, srcFrame);

    destFrame->width  = srcFrame->width;
    destFrame->height = srcFrame->height;
    destFrame->pkt_duration = srcFrame->pkt_duration;
    destFrame->pkt_size = srcFrame->pkt_size;
    destFrame->format = (int)dest_format;
    destFrame->pict_type = AV_PICTURE_TYPE_NONE;

    // 为 YUV420P 格式的 AVFrame 分配内存
    ret = av_frame_get_buffer(destFrame, 32);
    if (ret < 0) {
        console.error("Could not allocate destination frame data");
        return -1;
    }

    ret = av_frame_make_writable(destFrame);

    // 调用转换函数
    ret = transform_video_format(srcFrame, src_format, destFrame, dest_format);
    if (ret < 0) {
        console.error("Failed to convert YUV422P to YUV420P");
        return -1;
    }

    // 输出视频编码
    ret = avcodec_send_frame(videoCodecCtx, destFrame);
    if (ret < 0) {
        console.error("发送视频数据包到编码器时出错");
        return ret;
    }
    while (1) {
        fSmartPtr<AVPacket> videoPacket = av_packet_alloc();
        ret = avcodec_receive_packet(videoCodecCtx, videoPacket);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            videoPacket->stream_index = outputVideoStream->index;
            if (videoPacket->pts == AV_NOPTS_VALUE) {
                // Write PTS
                AVRational time_base = inputVideoStream->time_base;
                // Duration between 2 frames (us)
                double duration = (double)AV_TIME_BASE / av_q2d(inputVideoStream->r_frame_rate);
                // Parameters
                videoPacket->pts = (int64_t)((double)(frameIndex * duration) / (double)(av_q2d(time_base) * AV_TIME_BASE));
                videoPacket->dts = videoPacket->pts;
                videoPacket->duration = (int64_t)((double)duration / (double)(av_q2d(time_base) * AV_TIME_BASE));
            }
            else {
                //if (videoPacket->duration == 0)
                //    videoPacket->duration = 1;
                av_packet_rescale_ts(videoPacket, videoCodecCtx->time_base, outputVideoStream->time_base);
                console.error("videoPacket->duration = %d\n", videoPacket->duration);
            }
            //ret = av_interleaved_write_frame(outputFormatCtx, videoPacket);
            ret = av_write_frame(outputFormatCtx, videoPacket);
            if (ret < 0) {
                console.error("保存编码器视频帧时出错");
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret == AVERROR_EOF) {
            console.error("EOF");
            break;
        }
        else if (ret < 0) {
            console.error("从编码器接收视频帧时出错");
            break;
        }
    }
    return ret;
}

int transform_audio_format(AVCodecContext * audioCodecCtx,
                           AVFrame * src_frame, AVSampleFormat src_format,
                           AVFrame * dest_frame, AVSampleFormat dest_format)
{
    int ret = 0;
#if 1
    SwrContext * swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        console.error("Could not allocate SwrContext");
        return -1;
    }

    // 配置 SwrContext
    ret = av_opt_set_int(swr_ctx, "in_channel_count",   src_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_count",  dest_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "in_channel_layout",  AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "in_sample_rate",  44100, 0);
    ret = av_opt_set_int(swr_ctx, "out_sample_rate", 48000, 0);
    ret = av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",  AV_SAMPLE_FMT_S16,  0);
    ret = av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        console.error("Could not initialize SwrContext");
        swr_free(&swr_ctx);
        return -1;
    }

    // 执行格式转换
    ret = swr_convert_frame(swr_ctx, dest_frame, src_frame);
    if (ret < 0) {
        console.error("Error converting PCM_S16LE to FLTP");
        swr_free(&swr_ctx);
        return -1;
    }

    swr_free(&swr_ctx);
#else
    // 初始化 SwrContext 用于样本格式转换
    SwrContext * swr_ctx = swr_alloc_set_opts(
        NULL,
        audioCodecCtx->channel_layout,
        dest_format,
        audioCodecCtx->sample_rate,
        src_frame->channel_layout,
        src_format,
        src_frame->sample_rate,
        0, NULL);
    if (swr_ctx == NULL)
        return -1;

    // 转换 PCM 数据到 AAC 帧
    ret = swr_convert(swr_ctx, dest_frame->data,
                      dest_frame->nb_samples,
                      (const uint8_t **)src_frame->data,
                      src_frame->nb_samples);

    // 释放 SwrContext
    swr_free(&swr_ctx);
#endif
    return ret;
}

int avcodec_encode_audio_frame(AVFormatContext * outputFormatCtx,
                               AVCodecContext * audioCodecCtx,
                               AVStream * inputAudioStream,
                               AVStream * outputAudioStream,
                               AVFrame * srcFrame, int frameIndex,
                               AVSampleFormat src_format,
                               AVSampleFormat dest_format)
{
    int ret;
    // 创建一个 AAC 格式的 AVFrame
    fSmartPtr<AVFrame> destFrame = av_frame_alloc();
    if (destFrame == NULL) {
        console.error("destFrame 创建失败");
        return -1;
    }

    ret = av_frame_copy_props(destFrame, srcFrame);

    destFrame->channels  = audioCodecCtx->channels;
    destFrame->sample_rate = audioCodecCtx->sample_rate;
    destFrame->channel_layout = audioCodecCtx->channel_layout;
    destFrame->nb_samples = audioCodecCtx->frame_size;
    destFrame->pkt_duration = srcFrame->pkt_duration;
    //destFrame->pkt_size = audioCodecCtx->frame_size;
    destFrame->format = (int)dest_format;

    // 为 AAC 格式的 AVFrame 分配内存
    ret = av_frame_get_buffer(destFrame, 32);
    if (ret < 0) {
        console.error("Could not allocate destination frame data");
        return -1;
    }

    ret = av_frame_make_writable(destFrame);

    // 调用转换函数
    ret = transform_audio_format(audioCodecCtx, srcFrame, src_format, destFrame, dest_format);
    if (ret < 0) {
        console.error("Failed to convert PCM_S16LE to AAC");
        return -1;
    }

    // 输出视频编码
    ret = avcodec_send_frame(audioCodecCtx, destFrame);
    if (ret < 0) {
        console.error("发送音频数据包到编码器时出错");
        return ret;
    }
    while (1) {
        fSmartPtr<AVPacket> audioPacket = av_packet_alloc();
        ret = avcodec_receive_packet(audioCodecCtx, audioPacket);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            audioPacket->stream_index = outputAudioStream->index;
            if (audioPacket->pts == AV_NOPTS_VALUE) {
                // Write PTS
                AVRational time_base = inputAudioStream->time_base;
                // Duration between 2 frames (us)
                double duration = (double)AV_TIME_BASE / av_q2d(inputAudioStream->r_frame_rate);
                // Parameters
                audioPacket->pts = (int64_t)((double)(frameIndex * duration) / (double)(av_q2d(time_base) * AV_TIME_BASE));
                audioPacket->dts = audioPacket->pts;
                audioPacket->duration = (int64_t)((double)duration / (double)(av_q2d(time_base) * AV_TIME_BASE));
            }
            else {
                //if (audioPacket->duration == 0)
                //    audioPacket->duration = 1;
                if (av_q2d_eq(audioCodecCtx->time_base, outputAudioStream->time_base)) {
                    av_packet_rescale_ts(audioPacket, audioCodecCtx->time_base, outputAudioStream->time_base);
                }
                console.error("audioPacket->duration = %d\n", audioPacket->duration);
            }
            //ret = av_interleaved_write_frame(outputFormatCtx, audioPacket);
            ret = av_write_frame(outputFormatCtx, audioPacket);
            if (ret < 0) {
                console.error("保存编码器音频帧时出错");
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret == AVERROR_EOF) {
            console.error("EOF");
            break;
        }
        else if (ret < 0) {
            console.error("从编码器接收音频帧时出错");
            break;
        }
    }
    return ret;
}

int ffmpeg_camera_recorder()
{
    av_log_set_level(AV_LOG_INFO);
    set_log_level(LOG_INFO);

    #define OUTPUT_FILENAME "output.mp4"

    int result = 0;

    // 打开视频设备
    AVInputFormat * videoInputFormat = av_find_input_format("dshow");
    if (videoInputFormat == NULL) {
        log_print(LOG_ERROR, "Could not find video input format");
        return -1;
    }

#if 0
    // 列出视频设备
    AVDeviceInfoList * videoDeviceList = NULL;
    result = avdevice_list_input_sources(videoInputFormat, NULL, NULL, &videoDeviceList);
    if (result < 0) {
        log_print(LOG_ERROR, "Could not list video devices");
        return -1;
    }

    // 选择视频设备
    const char * videoDeviceName = videoDeviceList->devices[0]->device_name;
    log_print(LOG_INFO, "Selected video device: %s\n", videoDeviceName);
#endif

    std::string vdDeviceName = Ansi2Utf8(videoDeviceList_[0]);
    const char * videoDeviceName = vdDeviceName.c_str();

    // 列出音频设备
    AVInputFormat * audioInputFormat = av_find_input_format("dshow");
    if (audioInputFormat == NULL) {
        log_print(LOG_ERROR, "Could not find audio input format");
        return -1;
    }

#if 0
    AVDeviceInfoList * audioDeviceList = NULL;
    result = avdevice_list_input_sources(audioInputFormat, NULL, NULL, &audioDeviceList);
    if (result == AVERROR(ENOSYS)) {
        // Not support
        result = -1;
    }
    if (result < 0) {
        log_print(LOG_ERROR, "Could not list audio devices");
        return -1;
    }

    // 选择音频设备
    const char * audioDeviceName = audioDeviceList->devices[0]->device_name;
    log_print(LOG_INFO, "Selected audio device: %s\n", audioDeviceName);
#endif

    std::string adDeviceName = Ansi2Utf8(audioDeviceList_[0]);
    const char * audioDeviceName = adDeviceName.c_str();

#if 0
    // 释放设备列表
    avdevice_free_list_devices(&videoDeviceList);
    avdevice_free_list_devices(&audioDeviceList);
#endif

    // 创建 AVFormatContext
    fSmartPtr<AVFormatContext> inputVideoFormatCtx = avformat_alloc_context();

    char video_url[256];
    snprintf(video_url, sizeof(video_url), "video=%s", videoDeviceName);

    result = avformat_open_input(&inputVideoFormatCtx, video_url, videoInputFormat, NULL);
    if (result != 0) {
        console.error("无法打开输入视频设备");
        return -1;
    }

    // 查找视频流
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < inputVideoFormatCtx->nb_streams; i++) {
        if (inputVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        console.error("找不到输入视频流");
        return -1;
    }

    // 创建 AVFormatContext
    fSmartPtr<AVFormatContext> inputAudioFormatCtx = avformat_alloc_context();

    char audio_url[256];
    snprintf(audio_url, sizeof(audio_url), "audio=%s", audioDeviceName);

    result = avformat_open_input(&inputAudioFormatCtx, audio_url, audioInputFormat, NULL);
    if (result != 0) {
        console.error("无法打开输入音频设备");
        return -1;
    }

    // 查找音频流
    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputAudioFormatCtx->nb_streams; i++) {
        if (inputAudioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        console.error("找不到输入音频流");
        return -1;
    }

    // 获取视频和音频的编解码器参数
    AVStream * inputVideoStream = inputVideoFormatCtx->streams[videoStreamIndex];
    AVStream * inputAudioStream = inputAudioFormatCtx->streams[audioStreamIndex];

    AVCodecParameters * videoCodecParams = inputVideoFormatCtx->streams[videoStreamIndex]->codecpar;
    AVCodecParameters * audioCodecParams = inputAudioFormatCtx->streams[audioStreamIndex]->codecpar;

    // 查找视频的编解码器
    AVCodec * inputVideoCodec = avcodec_find_decoder(videoCodecParams->codec_id);
    if (inputVideoCodec == NULL) {
        console.error("查找输入视频解码器 codec 失败");
        return -1;
    }

    fSmartPtr<AVCodecContext> inputVideoCodecCtx = avcodec_alloc_context3(inputVideoCodec);

    AVRational time_base = {333333, 10000000};
    AVRational framerate = {10000000, 333333};

    // AV_CODEC_ID_RAWVIDEO (13): "RawVideo"
    inputVideoCodecCtx->codec_id = videoCodecParams->codec_id;
    inputVideoCodecCtx->bit_rate = 2000000;
    inputVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    inputVideoCodecCtx->width = videoCodecParams->width;
    inputVideoCodecCtx->height = videoCodecParams->height;
    inputVideoCodecCtx->time_base = av_inv_q(inputVideoStream->r_frame_rate);
    inputVideoCodecCtx->framerate = inputVideoStream->r_frame_rate;
    inputVideoCodecCtx->gop_size = 12;
    inputVideoCodecCtx->max_b_frames = 0;
    // AV_PIX_FMT_YUYV422 (1)
    inputVideoCodecCtx->pix_fmt = (AVPixelFormat)videoCodecParams->format;

    //inputVideoStream->time_base = inputVideoCodecCtx->time_base;

    result = avcodec_open2(inputVideoCodecCtx, inputVideoCodec, NULL);
    if (result == AVERROR(EINVAL)) {
        // EINVAL: Invalid argument
    }
    if (result < 0) {
        console.error("无法打开输入视频编解码器");
        return -1;
    }

    // 查找音频的编解码器
    AVCodec * inputAudioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
    if (inputAudioCodec == NULL) {
        console.error("查找输入音频解码器 codec 失败");
        return -1;
    }
    fSmartPtr<AVCodecContext> inputAudioCodecCtx = avcodec_alloc_context3(inputAudioCodec);

    AVRational audio_time_base = { 1, audioCodecParams->sample_rate };

    // AV_CODEC_ID_PCM_S16LE (65536)
    inputAudioCodecCtx->codec_id = audioCodecParams->codec_id;
    inputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    inputAudioCodecCtx->channels = audioCodecParams->channels;
    inputAudioCodecCtx->sample_rate = audioCodecParams->sample_rate;
    // 麦克风应该是单声道的, 备选值：AV_CH_LAYOUT_STEREO
    //inputAudioCodecCtx->channel_layout = audioCodecParams->channel_layout;
    inputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    inputAudioCodecCtx->time_base = audio_time_base;
    // AV_SAMPLE_FMT_S16 (1)
    inputAudioCodecCtx->sample_fmt = (AVSampleFormat)audioCodecParams->format;

    //inputAudioStream->time_base = inputAudioCodecCtx->time_base;

    result = avcodec_open2(inputAudioCodecCtx, inputAudioCodec, NULL);
    if (result < 0) {
        console.error("无法打开输入音频编解码器");
        return -1;
    }

    // 创建输出格式上下文
    fSmartPtr<AVFormatContext, SMT_Output> outputFormatCtx = NULL;
    result = avformat_alloc_output_context2(&outputFormatCtx, NULL, "mp4", OUTPUT_FILENAME);
    if (result < 0) {
        console.error("无法创建输出格式context");
        return -1;
    }

    // 打开视频的编解码器
    AVCodec * outputVideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (outputVideoCodec == NULL) {
        console.error("查找输出视频解码器 codec 失败");
        return -1;
    }
    fSmartPtr<AVCodecContext> outputVideoCodecCtx = avcodec_alloc_context3(outputVideoCodec);

    outputVideoCodecCtx->bit_rate = 2000000;
    //outputVideoCodecCtx->codec_id = AV_CODEC_ID_H263;
    outputVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    outputVideoCodecCtx->width = videoCodecParams->width;
    outputVideoCodecCtx->height = videoCodecParams->height;
    outputVideoCodecCtx->time_base = time_base;
    outputVideoCodecCtx->framerate = framerate;
    outputVideoCodecCtx->gop_size = 30 * 3;
    outputVideoCodecCtx->max_b_frames = 0;
    outputVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //outputVideoCodecCtx->pix_fmt = AV_PIX_FMT_NV12;

    if (outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        outputVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //
    // See: https://blog.csdn.net/weixin_50873490/article/details/141899535
    //
    if (outputVideoCodecCtx->codec_id == AV_CODEC_ID_H264) {
        result = av_opt_set(outputVideoCodecCtx->priv_data, "profile", "main", 0);
        // 0 latency
        result = av_opt_set(outputVideoCodecCtx->priv_data, "tune", "zerolatency", 0);
    }

    result = avcodec_open2(outputVideoCodecCtx.ptr(), outputVideoCodec, NULL);
    if (result == AVERROR(ENOSYS) || result == AVERROR(EINVAL)) {
        // EINVAL: Invalid argument
        //result = 0;
    }
    if (result < 0) {
        console.error("无法打开输出视频编解码器");
        return -1;
    }

    // 打开音频的编解码器
    AVCodec * outputAudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (outputAudioCodec == NULL) {
        console.error("查找输出音频解码器 codec 失败");
        return -1;
    }
    fSmartPtr<AVCodecContext> outputAudioCodecCtx = avcodec_alloc_context3(outputAudioCodec);

    audio_time_base.num = 1;
    audio_time_base.den = 48000;

    outputAudioCodecCtx->bit_rate = 160000;
    outputAudioCodecCtx->codec_id = AV_CODEC_ID_AAC;
    outputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    outputAudioCodecCtx->channels = 2;
    outputAudioCodecCtx->sample_rate = 48000;
    outputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    outputAudioCodecCtx->time_base = audio_time_base;
    outputAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;

    if (outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        outputAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    result = avcodec_open2(outputAudioCodecCtx, outputAudioCodec, NULL);
    if (result < 0) {
        console.error("无法打开输出音频编解码器");
        return -1;
    }

    // 添加视频流
    AVStream * outputVideoStream = avformat_new_stream(outputFormatCtx, outputVideoCodec);

    outputVideoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    outputVideoStream->codecpar->codec_id = outputVideoCodec->id;

    // 复制视频编解码器参数
    //avcodec_parameters_copy(outputVideoStream->codecpar, videoCodecParams);
    avcodec_parameters_from_context(outputVideoStream->codecpar, outputVideoCodecCtx);
    // 注：读取视频文件时使用
    //avcodec_parameters_to_context(outputVideoCodecCtx, outputVideoStream->codecpar);

    //outputVideoStream->codec = outputVideoCodecCtx->codec;
    outputVideoStream->id    = outputVideoCodecCtx->codec_id;
    outputVideoStream->codecpar->codec_tag = 0;

    // 添加音频流
    AVStream * outputAudioStream = avformat_new_stream(outputFormatCtx, outputAudioCodec);

    outputAudioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    outputAudioStream->codecpar->codec_id = outputAudioCodec->id;

    // 复制音频的编解码器参数
    //avcodec_parameters_copy(outputAudioStream->codecpar, audioCodecParams);
    avcodec_parameters_from_context(outputAudioStream->codecpar, outputAudioCodecCtx);
    // 注：读取音频文件时使用
    //avcodec_parameters_to_context(outputAudioCodecCtx, outputAudioStream->codecpar);

    //outputAudioStream->codec = outputAudioCodecCtx->codec;
    outputAudioStream->id    = outputAudioCodecCtx->codec_id;
    outputAudioStream->codecpar->codec_tag = 0;

    //outputVideoStream->time_base = outputVideoCodecCtx->time_base;
    //outputAudioStream->time_base = outputAudioCodecCtx->time_base;

#if 0
    outputFormatCtx->video_codec = outputVideoCodec;
    outputFormatCtx->video_codec_id = outputVideoCodec->id;

    outputFormatCtx->audio_codec = outputAudioCodec;
    outputFormatCtx->audio_codec_id = outputAudioCodec->id;
#endif

    // 打开输出文件
    if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        result = avio_open(&outputFormatCtx->pb, OUTPUT_FILENAME, AVIO_FLAG_WRITE);
        if (result < 0) {
            console.error("无法打开输出文件");
            return -1;
        }
    }

    // 写入文件头
    result = avformat_write_header(outputFormatCtx, NULL);
    if (result < 0) {
        console.error("无法写入文件头");
        return -1;
    }

    // 读取和编码视频帧

    int ret, ret2;
    int nVideoFrameCnt = 0;
    int nAudioFrameCnt = 0;
    bool isExit = false;
    while (1) {
        // Video
        if (1) {
            fSmartPtr<AVPacket> videoPacket = av_packet_alloc();
            ret = av_read_frame(inputVideoFormatCtx, videoPacket);
            if (ret < 0) {
                isExit = true;
                break;
            }
            // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
            av_packet_rescale_ts(videoPacket, inputVideoStream->time_base,
                                 inputVideoCodecCtx->time_base);
            if (videoPacket->stream_index == videoStreamIndex) {
                // 输入视频解码
                ret = avcodec_send_packet(inputVideoCodecCtx, videoPacket);
                if (ret < 0) {
                    console.error("发送视频数据包到解码器时出错");
                    isExit = true;
                    break;
                }

                while (1) {
                    fSmartPtr<AVFrame> frame = av_frame_alloc();
                    ret = av_frame_is_writable(frame);
                    ret = avcodec_receive_frame(inputVideoCodecCtx, frame);
                    if (ret == 0) {
                        ret = av_frame_make_writable(frame);
                        // 将编码后的视频帧写入输出文件
                        //frame->pts = av_rescale_q(frame->pts, inputVideoCodecCtx->time_base, outputVideoStream->time_base);
                        //frame->pts = nFrameCount;
                        if (frame->pts == AV_NOPTS_VALUE) {
                            frame->pts = frame->best_effort_timestamp;
                            frame->pkt_dts = frame->pts;
                            frame->pkt_duration = av_rescale_q(1, inputVideoStream->time_base, outputVideoStream->time_base);
                        }

                        ret2 = avcodec_encode_video_frame(outputFormatCtx, outputVideoCodecCtx,
                                                          inputVideoStream, outputVideoStream,
                                                          frame, nVideoFrameCnt,
                                                          inputVideoCodecCtx->pix_fmt,
                                                          outputVideoCodecCtx->pix_fmt);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            isExit = true;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                nVideoFrameCnt++;
                                console.error("nFrameCount = %d\n", nVideoFrameCnt + 1);
                                if (nVideoFrameCnt > 100) {
                                    isExit = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        console.error("avcodec_receive_frame(): AVERROR(EAGAIN)");
                        continue;
                    }
                    else if (ret == AVERROR_EOF) {
                        console.error("EOF");
                        break;
                    } else if (ret < 0) {
                        console.error("从解码器接收视频帧时出错");
                        isExit = true;
                        break;
                    }
                }
                if (isExit)
                    break;
            }
        }

        if (isExit)
            break;

        // Audio
        if (1) {
            fSmartPtr<AVPacket> audioPacket = av_packet_alloc();
            ret = av_read_frame(inputAudioFormatCtx, audioPacket);
            if (ret < 0) {
                isExit = true;
                break;
            }
            if (audioPacket->stream_index == audioStreamIndex) {
                // 输入音频解码
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(audioPacket, inputAudioStream->time_base,
                                     inputAudioCodecCtx->time_base);
                fSmartPtr<AVFrame> frame = av_frame_alloc();
                int ret = avcodec_send_packet(inputAudioCodecCtx, audioPacket);
                if (ret < 0) {
                    console.error("发送音频数据包到解码器时出错");
                    break;
                }
                while (1) {
                    ret = avcodec_receive_frame(inputAudioCodecCtx, frame);
                    if (ret == 0) {
                        // 将编码后的音频帧写入输出文件
                        //frame->pts = av_rescale_q(frame->pts, inputAudioCodecCtx->time_base, outputAudioStream->time_base);
                        //frame->pkt_dts = frame->pts;
                        //frame->pkt_duration = av_rescale_q(1, inputAudioCodecCtx->time_base, outputAudioStream->time_base);
                        if (frame->pts == AV_NOPTS_VALUE) {
                            frame->pts = frame->best_effort_timestamp;
                            frame->pkt_dts = frame->pts;
                            frame->pkt_duration = av_rescale_q(1, inputVideoStream->time_base, outputVideoStream->time_base);
                        }

                        ret2 = avcodec_encode_audio_frame(outputFormatCtx, outputAudioCodecCtx,
                                                          inputAudioStream, outputAudioStream,
                                                          frame, nAudioFrameCnt,
                                                          inputAudioCodecCtx->sample_fmt,
                                                          outputAudioCodecCtx->sample_fmt);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            isExit = true;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                nAudioFrameCnt++;
                                console.error("nAudioCount = %d\n", nAudioFrameCnt + 1);
                                if (nAudioFrameCnt > 50) {
                                    isExit = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        break;
                    }
                    else if (ret == AVERROR_EOF) {
                        break;
                    }
                    else if (ret < 0) {
                        console.error("从解码器接收音频帧时出错");
                        isExit = true;
                        break;
                    }
                }
            }
        }
        if (isExit)
            break;
    }

    // 写入文件尾
    ret = av_write_trailer(outputFormatCtx);
    ret = avformat_flush(outputFormatCtx);

    if (outputFormatCtx && !(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(outputFormatCtx->pb);
    }

    // 释放资源
    //avformat_close_input(&inputVideoFormatCtx);
    //avformat_close_input(&inputAudioFormatCtx);
    //avcodec_free_context(&outputVideoCodecCtx);
    //avcodec_free_context(&outputAudioCodecCtx);
    //avformat_free_context(outputFormatCtx);

    avformat_network_deinit();
    return 0;
}

int main(int argc, char const *argv[])
{
#if LIBAVFORMAT_BUILD < AV_VERSION_INT(58, 9, 100)
    // 该函数从 avformat 库的 58.9.100 版本开始被废弃
    // (2018-02-06, 大约是 FFmepg 4.0 版本)
    av_register_all();
#endif

    avdevice_register_all();
    avformat_network_init();

    av_log_set_level(AV_LOG_INFO);
    console.set_log_level(av::LogLevel::Info);

    int ret = ffmpeg_camera_recorder();

    avformat_network_deinit();

#if defined(_WIN32) || defined(_WIN64)
    //console.println("");
    //::system("pause");
#endif
    return 0;
}
