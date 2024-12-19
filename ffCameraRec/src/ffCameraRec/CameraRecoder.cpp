
#include "CameraRecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/log.h>
#include <libavutil/mathematics.h>
}

#include <stdio.h>
#include <conio.h>      // For _getch()
#include <math.h>
#include <assert.h>

#include "global.h"
#include "utils.h"
#include "string_utils.h"
#include "fSmartPtr.h"
#include "DShowDevice.h"
#include "StopWatch.h"

#include "sws_video.h"
#include "swr_audio.h"

//
// About libavdivice: https://blog.csdn.net/leixiaohua1020/article/details/39702113
//
CameraRecoder::CameraRecoder()
{
    inited_ = false;

    // Input
    av_input_fmt_ = nullptr;

    v_ifmt_ctx_ = nullptr;
    a_ifmt_ctx_ = nullptr;

    v_in_stream_ = nullptr;
    a_in_stream_ = nullptr;

    v_input_codec_ = nullptr;
    a_input_codec_ = nullptr;

    v_icodec_ctx_ = nullptr;
    a_icodec_ctx_ = nullptr;

    // Output
    v_ofmt_ctx_ = nullptr;
    a_ofmt_ctx_ = nullptr;

    v_out_stream_ = nullptr;
    a_out_stream_ = nullptr;

    v_output_codec_ = nullptr;
    a_output_codec_ = nullptr;

    v_ocodec_ctx_ = nullptr;
    a_ocodec_ctx_ = nullptr;

    av_ofmt_ctx_ = nullptr;

    sws_video_ = nullptr;
    swr_audio_ = nullptr;

    v_stream_index_ = -1;
    a_stream_index_ = -1;

    v_enc_entered_ = false;
    a_enc_entered_ = false;

    v_stopflag_ = true;
    a_stopflag_ = true;

    timestamp_reset();
}

CameraRecoder::~CameraRecoder()
{
    release();
}

void CameraRecoder::release()
{
    int ret = stop();
    cleanup();
}

void CameraRecoder::cleanup()
{
    if (swr_audio_) {
        swr_audio_->cleanup();
        delete swr_audio_;
        swr_audio_ = nullptr;
    }

    if (sws_video_) {
        sws_video_->cleanup();
        delete sws_video_;
        sws_video_ = nullptr;
    }

    if (a_icodec_ctx_) {
        avcodec_free_context(&a_icodec_ctx_);
        a_icodec_ctx_ = nullptr;
    }

    if (v_icodec_ctx_) {
        avcodec_free_context(&v_icodec_ctx_);
        v_icodec_ctx_ = nullptr;
    }

    if (a_ifmt_ctx_) {
        avformat_close_input(&a_ifmt_ctx_);
        a_ifmt_ctx_ = nullptr;
    }

    if (v_ifmt_ctx_) {
        avformat_close_input(&v_ifmt_ctx_);
        v_ifmt_ctx_ = nullptr;
    }

    if (a_ocodec_ctx_) {
        avcodec_free_context(&a_ocodec_ctx_);
        a_ocodec_ctx_ = nullptr;
    }

    if (v_ocodec_ctx_) {
        avcodec_free_context(&v_ocodec_ctx_);
        v_ocodec_ctx_ = nullptr;
    }

    if (av_ofmt_ctx_) {
        if (av_ofmt_ctx_->pb) {
            if (!(av_ofmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&av_ofmt_ctx_->pb);
                av_ofmt_ctx_->pb = nullptr;
            }
        }
        avformat_free_context(av_ofmt_ctx_);
        av_ofmt_ctx_ = nullptr;
    }

    v_stream_index_ = -1;
    a_stream_index_ = -1;

    // Input
    v_ifmt_ctx_ = nullptr;
    a_ifmt_ctx_ = nullptr;

    v_input_codec_ = nullptr;
    a_input_codec_ = nullptr;

    v_in_stream_ = nullptr;
    a_in_stream_ = nullptr;

    v_icodec_ctx_ = nullptr;
    a_icodec_ctx_ = nullptr;

    // Output
    v_ofmt_ctx_ = nullptr;
    a_ofmt_ctx_ = nullptr;

    v_output_codec_ = nullptr;
    a_output_codec_ = nullptr;

    v_out_stream_ = nullptr;
    a_out_stream_ = nullptr;

    v_ocodec_ctx_ = nullptr;
    a_ocodec_ctx_ = nullptr;

    // Input video & audio
    av_input_fmt_ = nullptr;

    v_enc_entered_ = false;
    a_enc_entered_ = false;

    v_stopflag_ = true;
    a_stopflag_ = true;

    inited_ = false;
}

const char * get_display_ptr(uint8_t * ptr)
{
    return ((ptr != nullptr) ? (const char *)(ptr) : "nullptr");
}

int CameraRecoder::init(const std::string & output_file)
{
    int ret = AVERROR_UNKNOWN;
    if (inited_) {
        return AVERROR(EEXIST);
    }
    
    release();
    output_file_ = output_file;

    // 设置设备枚举选项
    fSmartPtr<AVDictionary> device_options = nullptr;
    ret = av_dict_set(&device_options, "list_devices", "true", 0);

    // 创建 dshow 视频 input format
    assert(av_input_fmt_ == nullptr);
    av_input_fmt_ = av_find_input_format("dshow");
    if (av_input_fmt_ == nullptr) {
        console.error("Could not find input video format");
        return AVERROR(ENOSYS);
    }

    char video_url[256];
    char audio_url[256];

    // 使用 dshow 枚举所有视频和音频设备
    {
        DShowDevice dshowDevice;
        dshowDevice.init();

        if (dshowDevice.videoDeviceList_.size() > 0)
            videoDeviceName_ = string_utils::ansi_to_utf8(dshowDevice.videoDeviceList_[0]);
        else
            videoDeviceName_ = "";

        if (dshowDevice.audioDeviceList_.size() > 0)
            audioDeviceName_ = string_utils::ansi_to_utf8(dshowDevice.audioDeviceList_[0]);
        else
            audioDeviceName_ = "";
    }

    // 选择视频设备
    if (!videoDeviceName_.empty())
        snprintf(video_url, sizeof(video_url), "video=%s", videoDeviceName_.c_str());
    else
        snprintf(video_url, sizeof(video_url), "video=dummy");

    // 选择音频设备
    if (!audioDeviceName_.empty())
        snprintf(audio_url, sizeof(audio_url), "audio=%s", audioDeviceName_.c_str());
    else
        snprintf(audio_url, sizeof(audio_url), "audio=dummy");

#if 0
    // 打开一个虚拟的输入流以枚举设备 (从 FFmpeg 4.2.10 已失效, 仅用于查询设备列表, 不能返回)
    assert(v_ifmt_ctx_ == nullptr);
    ret = avformat_open_input(&v_ifmt_ctx_, "video=dummy", av_input_fmt_, &device_options);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, sizeof(error_buf));
        // ret = AVERROR_EXIT, Error = "dummy: Immediate exit requested"
        // See FFmpeg 4.2.10, /libavdivce/dshow.c, line 1131 ~ 1138
        console.error("Could not open input video device: %s", error_buf);
        return ret;
    }
#endif

    AVDictionary * input_video_options = NULL;

    // 设置 dshow 的参数
    //ret = av_dict_set(&input_video_options, "rtbufsize", "2048", 0);
    ret = av_dict_set(&input_video_options, "video_size", "640x480", 0);      // 设置图像大小
    ret = av_dict_set(&input_video_options, "video_buffer_size", "50", 0);    // 设置缓冲区大小, 单位是 毫秒? Kb? 
    ret = av_dict_set(&input_video_options, "framerate", "30", 0);            // 帧率: 30
    //ret = av_dict_set(&input_video_options, "pixel_format", "yuyv422", 0);    // 设置像素格式为 yuyv422

    // 当视频设备被移除后, 读取数据不阻塞 (不能设置, 设置了程序不能正常运行)
    // v_ifmt_ctx_->flags |= AVFMT_FLAG_NONBLOCK; 

    // 打开视频设备
    assert(v_ifmt_ctx_ == nullptr);
    ret = avformat_open_input(&v_ifmt_ctx_, video_url, av_input_fmt_, &input_video_options);
    if (ret < 0) {
        console.error("Could not open input video device: %d", ret);
        return ret;
    }
    console.info("Selected video device: %s", videoDeviceName_.c_str());

    av_dict_free(&input_video_options);

    ret = avformat_find_stream_info(v_ifmt_ctx_, NULL);
    if (ret < 0) {
        console.error("Could not open input video stream: %d", ret);
        return ret;
    }

    // 查找视频流
    int v_stream_index = -1;
    for (unsigned int i = 0; i < v_ifmt_ctx_->nb_streams; i++) {
        if (v_ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            v_stream_index = i;
            break;
        }
    }
    if (v_stream_index == -1) {
        console.error("Could not find the input video stream");
        return AVERROR(ENOSYS);
    }
    v_stream_index_ = v_stream_index;

    // 获取输入视频的视频流 stream
    v_in_stream_ = v_ifmt_ctx_->streams[v_stream_index_];
    assert(v_in_stream_ != nullptr);

#if 0
    if (v_ifmt_ctx_) {
        std::cout << "========== Device Info ===========" << std::endl;
        if (v_ifmt_ctx_->nb_streams > 0) {
            for (unsigned int i = 0; i < v_ifmt_ctx_->nb_streams; ++i) {
                AVStream * stream = v_ifmt_ctx_->streams[i];
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    std::cout << "Video Device: " << stream->codecpar->codec_tag << " "
                              << get_display_ptr(stream->codecpar->extradata) << std::endl;
                } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    std::cout << "Audio Device: " << stream->codecpar->codec_tag << " "
                              << get_display_ptr(stream->codecpar->extradata) << std::endl;
                }
            }
        }
        std::cout << "==================================" << std::endl;
    }
#endif

#if 0
    // 列出视频设备 (此方法已失效)
    AVDeviceInfoList * videoDeviceList = NULL;
    ret = avdevice_list_input_sources(av_input_fmt_, NULL, NULL, &videoDeviceList);
    if (ret < 0) {
        console.error("Could not list video devices: %d", ret);
        return ret;
    }
#endif

    AVDictionary * input_audio_options = NULL;

    // 设置 dshow 的参数
    //ret = av_dict_set(&input_audio_options, "bitrate", "128000", 0);
    //ret = av_dict_set(&input_audio_options, "rtbufsize", "2048", 0);
    //ret = av_dict_set(&input_audio_options, "start_time_realtime", "0", 0);
    ret = av_dict_set(&input_audio_options, "sample_rate", "44100", 0);     // 设置采样率为 44100 Hz
    ret = av_dict_set(&input_audio_options, "audio_buffer_size", "24", 0);  // 设置缓冲区大小, 单位是 毫秒?
    //ret = av_dict_set(&input_audio_options, "audio_latency", "24ms", 0);    // 设置延迟, 单位是 毫秒?
    //ret = av_dict_set(&input_audio_options, "max_delay", "1000000", 0);     // FFmpeg默认是 0.7s
    //ret = av_dict_set(&input_audio_options, "channels", "2", 0);            // 设置双声道

    // 设置 FFmpeg 内部缓冲区为最小值 (不能设置, 设置了程序不能正常读取音频)
    // a_ifmt_ctx_->flags |= AVFMT_FLAG_NOBUFFER;    

    // 打开音频设备
    assert(a_ifmt_ctx_ == nullptr);
    ret = avformat_open_input(&a_ifmt_ctx_, audio_url, av_input_fmt_, &input_audio_options);
    if (ret < 0) {
        console.error("Could not open input audio device: %d", ret);
        return ret;
    }
    console.info("Selected audio device: %s", audioDeviceName_.c_str());

    av_dict_free(&input_audio_options);

    ret = avformat_find_stream_info(a_ifmt_ctx_, NULL);
    if (ret < 0) {
        console.error("Could not open input audio stream: %d", ret);
        return ret;
    }

    // 查找音频流
    int a_stream_index = -1;
    for (unsigned int i = 0; i < a_ifmt_ctx_->nb_streams; i++) {
        if (a_ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            a_stream_index = i;
            break;
        }
    }
    if (a_stream_index == -1) {
        console.error("Could not find the input audio stream");
        return AVERROR(ENOSYS);
    }
    a_stream_index_ = a_stream_index;

    // 获取输入音频的音频流 stream
    assert(a_in_stream_ == nullptr);
    a_in_stream_ = a_ifmt_ctx_->streams[a_stream_index_];
    assert(a_in_stream_ != nullptr);

    // 查找输入视频的解码器 codec
    assert(v_input_codec_ == nullptr);
    v_input_codec_ = avcodec_find_decoder(v_in_stream_->codecpar->codec_id);
    if (v_input_codec_ == nullptr) {
        console.error("Failed to find the input video decoder codec");
        return AVERROR(ENOSYS);
    }

    // 创建输入视频的解码器 context
    assert(v_icodec_ctx_ == nullptr);
    v_icodec_ctx_ = avcodec_alloc_context3(v_input_codec_);
    assert(v_icodec_ctx_ != nullptr);

#if 1
    ret = avcodec_parameters_to_context(v_icodec_ctx_, v_in_stream_->codecpar);
    if (ret < 0) {
        console.error("Failed to copy input video decoder codec parameters: %d", ret);
        return ret;
    }
#endif

    // AV_CODEC_ID_RAWVIDEO (13): "RawVideo"
    v_icodec_ctx_->codec_id = v_in_stream_->codecpar->codec_id;
    //v_icodec_ctx_->bit_rate = 4000000;
    v_icodec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    v_icodec_ctx_->width = v_in_stream_->codecpar->width;
    v_icodec_ctx_->height = v_in_stream_->codecpar->height;
    v_icodec_ctx_->framerate = av_fixed_time_base(v_in_stream_->r_frame_rate);
    v_icodec_ctx_->time_base = av_fixed_time_base(av_inv_q(v_in_stream_->r_frame_rate));
    //v_icodec_ctx_->time_base = v_in_stream_->time_base;
    v_icodec_ctx_->gop_size = 12;
    v_icodec_ctx_->max_b_frames = 0;
    // AV_PIX_FMT_YUYV422 (1)
    v_icodec_ctx_->pix_fmt = (AVPixelFormat)v_in_stream_->codecpar->format;

    //v_in_stream_->time_base = v_icodec_ctx_->time_base;

    // 打开输入视频的解码器 context
    ret = avcodec_open2(v_icodec_ctx_, v_input_codec_, NULL);
    if (ret < 0) {
        console.error("Failed to open input video decoder context: %d", ret);
        return ret;
    }

    // 查找输入音频的解码器 codec
    assert(a_input_codec_ == nullptr);
    a_input_codec_ = avcodec_find_decoder(a_in_stream_->codecpar->codec_id);
    if (a_input_codec_ == nullptr) {
        console.error("Failed to find the input audio decoder codec");
        return AVERROR(ENOSYS);
    }

    // 创建输入音频的解码器 context
    assert(a_icodec_ctx_ == nullptr);
    a_icodec_ctx_ = avcodec_alloc_context3(a_input_codec_);
    assert(a_icodec_ctx_ != nullptr);

#if 1
    ret = avcodec_parameters_to_context(a_icodec_ctx_, a_in_stream_->codecpar);
    if (ret < 0) {
        console.error("Failed to copy input audio decoder codec parameters: %d", ret);
        return ret;
    }
#endif

    AVRational audio_time_base = { 1, a_in_stream_->codecpar->sample_rate };

    // AV_CODEC_ID_PCM_S16LE (65536)
    a_icodec_ctx_->codec_id = a_in_stream_->codecpar->codec_id;
    a_icodec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
    //a_icodec_ctx_->bit_rate = a_ifmt_ctx_->bit_rate;
    a_icodec_ctx_->channels = a_in_stream_->codecpar->channels;
    a_icodec_ctx_->sample_rate = a_in_stream_->codecpar->sample_rate;
    // 麦克风默认是单声道的, 实际值是：AV_CH_LAYOUT_STEREO
    a_icodec_ctx_->channel_layout = av_get_default_channel_layout(a_icodec_ctx_->channels);
    a_icodec_ctx_->time_base = audio_time_base;
    //a_icodec_ctx_->time_base = a_in_stream_->time_base;
    // AV_SAMPLE_FMT_S16 (1)
    a_icodec_ctx_->sample_fmt = (AVSampleFormat)a_in_stream_->codecpar->format;

    //a_in_stream_->time_base = a_icodec_ctx_->time_base;

    // 打开输入音频的解码器 context
    ret = avcodec_open2(a_icodec_ctx_, a_input_codec_, NULL);
    if (ret < 0) {
        console.error("Failed to open input audio decoder context: %d", ret);
        return ret;
    }

    inited_ = true;

    ret = create_encoders();
    if (ret < 0) {
        return ret;
    }

    inited_ = true;
    return ret;
}

int CameraRecoder::create_encoders()
{
    int ret = AVERROR_UNKNOWN;
    if (!inited_) {
        return AVERROR(EINVAL);
    }

    inited_ = false;

    // 创建输出格式上下文
    assert(av_ofmt_ctx_ == nullptr);
    ret = avformat_alloc_output_context2(&av_ofmt_ctx_, NULL, "mp4", output_file_.c_str());
    if (ret < 0) {
        console.error("Failed to alloc output format context: %d", ret);
        return ret;
    }

    // 查找输出视频的编解码器 codec
    assert(v_output_codec_ == nullptr);
    v_output_codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (v_output_codec_ == nullptr) {
        console.error("Failed to open output video encoder codec");
        return AVERROR(ENOSYS);
    }

    assert(v_ocodec_ctx_ == nullptr);
    v_ocodec_ctx_ = avcodec_alloc_context3(v_output_codec_);
    assert(v_ocodec_ctx_ != nullptr);

    v_ocodec_ctx_->bit_rate = 2000000;
    assert(v_ocodec_ctx_->codec_id == AV_CODEC_ID_H264);
    v_ocodec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    v_ocodec_ctx_->width = v_in_stream_->codecpar->width;
    v_ocodec_ctx_->height = v_in_stream_->codecpar->height;
    v_ocodec_ctx_->framerate = av_fixed_time_base(v_in_stream_->r_frame_rate);
    v_ocodec_ctx_->time_base = av_fixed_time_base(av_inv_q(v_in_stream_->r_frame_rate));
    //v_ocodec_ctx_->time_base = v_in_stream_->time_base;
    v_ocodec_ctx_->qmin = 25;
    v_ocodec_ctx_->qmax = 30;
    v_ocodec_ctx_->gop_size = 30 * 3;
    v_ocodec_ctx_->max_b_frames = 0;
    v_ocodec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    //v_ocodec_ctx_->pix_fmt = AV_PIX_FMT_NV12;

    if (av_ofmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        v_ocodec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 推流时, 低延迟设置
    v_ocodec_ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

    //
    // See: https://blog.csdn.net/weixin_50873490/article/details/141899535
    //
    if (v_ocodec_ctx_->codec_id == AV_CODEC_ID_H264) {
        // profile: superfast 是最快的, 但是质量最低
        ret = av_opt_set(v_ocodec_ctx_->priv_data, "profile", "main", 0);
        // 0 latency
        ret = av_opt_set(v_ocodec_ctx_->priv_data, "tune", "zerolatency", 0);
        // Q 值的范围通常为 0-51，0 表示无损压缩，51 表示最低质量
        ret = av_opt_set(v_ocodec_ctx_->priv_data, "crf", "25", 0);
    }

    // 打开输出视频编码器 context
    ret = avcodec_open2(v_ocodec_ctx_, v_output_codec_, NULL);
    if (ret < 0) {
        console.error("Failed to open output video encoder context: %d", ret);
        return ret;
    }

    // 查找音频的编解码器 codec
    assert(a_output_codec_ == nullptr);
    a_output_codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (a_output_codec_ == NULL) {
        console.error("Failed to open output audio encoder codec");
        return AVERROR(ENOSYS);
    }

    assert(a_ocodec_ctx_ == nullptr);
    a_ocodec_ctx_ = avcodec_alloc_context3(a_output_codec_);
    assert(a_ocodec_ctx_ != nullptr);

    AVRational audio_time_base = { 1, 44100 };

    a_ocodec_ctx_->bit_rate = 128000;
    a_ocodec_ctx_->codec_id = AV_CODEC_ID_AAC;
    a_ocodec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
    a_ocodec_ctx_->channels = 2;
    a_ocodec_ctx_->sample_rate = 44100;
    a_ocodec_ctx_->channel_layout = av_get_default_channel_layout(a_ocodec_ctx_->channels);
    a_ocodec_ctx_->time_base = audio_time_base;
    //a_ocodec_ctx_->time_base = a_in_stream_->time_base;
    //a_ocodec_ctx_->time_base = v_ocodec_ctx_->time_base;
    a_ocodec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;

    if (av_ofmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        a_ocodec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 推流时, 低延迟设置
    a_ocodec_ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

    // 打开输出音频编码器 context
    ret = avcodec_open2(a_ocodec_ctx_, a_output_codec_, NULL);
    if (ret < 0) {
        console.error("Failed to open output audio encoder context: %d", ret);
        return ret;
    }

    // 添加视频流
    assert(v_out_stream_ == nullptr);
    v_out_stream_ = avformat_new_stream(av_ofmt_ctx_, v_output_codec_);
    assert(v_out_stream_ != nullptr);

    v_out_stream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    v_out_stream_->codecpar->codec_id = v_output_codec_->id;

    // 复制视频编解码器参数
    ret = avcodec_parameters_from_context(v_out_stream_->codecpar, v_ocodec_ctx_);
    // 注：读取视频文件时使用
    //ret = avcodec_parameters_to_context(v_out_stream_, v_out_stream_->codecpar);

    //v_out_stream_->codec = v_ocodec_ctx_->codec;
    v_out_stream_->id    = v_ocodec_ctx_->codec_id;
    v_out_stream_->codecpar->codec_tag = 0;

    v_out_stream_->codec->qmin = 25;
    v_out_stream_->codec->qmax = 30;

    v_out_stream_->r_frame_rate = v_ocodec_ctx_->framerate;
    v_out_stream_->avg_frame_rate = v_ocodec_ctx_->framerate;

    // 添加音频流
    assert(a_out_stream_ == nullptr);
    a_out_stream_ = avformat_new_stream(av_ofmt_ctx_, a_output_codec_);
    assert(a_out_stream_ != nullptr);

    a_out_stream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    a_out_stream_->codecpar->codec_id = a_output_codec_->id;

    // 复制音频的编解码器参数
    ret = avcodec_parameters_from_context(a_out_stream_->codecpar, a_ocodec_ctx_);
    // 注：读取音频文件时使用
    //ret = avcodec_parameters_to_context(a_out_stream_, a_out_stream_->codecpar);

    //a_out_stream_->codec = a_ocodec_ctx_->codec;
    a_out_stream_->id    = a_ocodec_ctx_->codec_id;
    a_out_stream_->codecpar->codec_tag = 0;

    return ret;
}

void av_frame_rescale_ts(AVFrame * frame, AVRational src_tb, AVRational dest_tb)
{
    if (frame->pts != AV_NOPTS_VALUE)
        frame->pts = av_rescale_q(frame->pts, src_tb, dest_tb);
#if 0
    if (frame->pkt_pts != AV_NOPTS_VALUE)
        frame->pkt_pts = av_rescale_q(frame->pkt_pts, src_tb, dest_tb);
    if (frame->pkt_dts != AV_NOPTS_VALUE)
        frame->pkt_dts = av_rescale_q(frame->pkt_dts, src_tb, dest_tb);
    if (frame->pkt_duration > 0)
        frame->pkt_duration = av_rescale_q(frame->pkt_duration, src_tb, dest_tb);
    if (frame->pkt_duration <= 0)
        frame->pkt_duration = 1;
#endif
}

int transform_video_format(AVFrame * src_frame, AVPixelFormat src_format,
                           AVFrame * dest_frame, AVPixelFormat dest_format)
{
    // 创建 SwsContext 用于格式转换
    SwsContext * sws_ctx = sws_getContext(
        src_frame->width, src_frame->height, src_format,
        dest_frame->width, dest_frame->height, dest_format,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (sws_ctx == nullptr) {
        console.error("Could not initialize the conversion context");
        return AVERROR(ENOMEM);
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

int CameraRecoder::encode_video_frame(AVFormatContext * av_ofmt_ctx,
                                      AVCodecContext * v_icodec_ctx,
                                      AVCodecContext * v_ocodec_ctx,
                                      AVStream * v_in_stream,
                                      AVStream * v_out_stream,
                                      AVFrame * src_frame, size_t & frame_index)
{
    int ret;
    // 创建一个 YUV420P 格式的 AVFrame
    fSmartPtr<AVFrame> dest_frame = nullptr;

    if (src_frame != nullptr) {
        dest_frame = av_frame_alloc();
        if (dest_frame == NULL) {
            console.error("Failed to create destination video framen");
            return AVERROR(ENOMEM);
        }

        AVPixelFormat src_format = v_icodec_ctx->pix_fmt;
        AVPixelFormat dest_format = v_ocodec_ctx->pix_fmt;

        ret = av_frame_copy_props(dest_frame, src_frame);

        dest_frame->width = src_frame->width;
        dest_frame->height = src_frame->height;
        dest_frame->format = (int)dest_format;
        dest_frame->pict_type = AV_PICTURE_TYPE_NONE;

        if (!av_q2d_eq(v_icodec_ctx->time_base, v_ocodec_ctx->time_base)) {
            av_frame_rescale_ts(dest_frame, v_icodec_ctx->time_base, v_ocodec_ctx->time_base);
        }

        // 为 YUV420P 格式的 Frame 分配内存
        ret = av_frame_get_buffer(dest_frame, 32);
        if (ret < 0) {
            console.error("Could not allocate destination video frame data: %d", ret);
            return ret;
        }

        ret = av_frame_make_writable(dest_frame);
        assert(ret == 0);

        if (sws_video_ == nullptr) {
            sws_video_ = new sws_video();
            ret = sws_video_->init(src_frame, src_format, dest_frame, dest_format);
            if (ret < 0) {
                console.error("Failed to convert YUV422P to YUV420P: %d", ret);
                return ret;
            }
        }

        // 调用转换函数
#if 1
        if (sws_video_ != nullptr) {
            ret = sws_video_->convert(src_frame, src_format, dest_frame, dest_format);
        } else {
            return AVERROR(ENOMEM);
        }
#else
        ret = transform_video_format(src_frame, src_format, dest_frame, dest_format);
#endif
        if (ret < 0) {
            console.error("Failed to convert YUV422P to YUV420P: %d", ret);
            return ret;
        }
    }

    // 输出视频编码
    ret = avcodec_send_frame(v_ocodec_ctx, dest_frame);
    if (ret < 0) {
        console.error("发送视频数据包到编码器时出错: %d\n", ret);
        return ret;
    }
    do {
        fSmartPtr<AVPacket> packet = av_packet_alloc();
        ret = avcodec_receive_packet(v_ocodec_ctx, packet);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            packet->stream_index = v_out_stream->index;
#if 0
            vi_time_stamp_.rescale_ts(packet, frame_index);
#else
            if (packet->pts == AV_NOPTS_VALUE) {
                packet->pts = vi_time_stamp_.pts(frame_index);
                packet->dts = packet->pts;
                packet->duration = vi_time_stamp_.duration();
            }
#if 0
            // Record the time of duplicate pts value
            if (packet->pts == v_last_pts_) {
                v_pts_addition_++;
                //console.error("v_pts_addition_ = %d", (int)v_pts_addition_);
            }
            v_last_pts_ = packet->pts;
            // Correct the duplicate pts value
            packet->pts = packet->pts + v_pts_addition_;
            packet->dts = packet->dts + v_pts_addition_;
#endif
            if (packet->duration == 0)
                packet->duration = 1;
#if 0
            fSmartPtr<AVPacket> packet2 = av_packet_alloc();
            vi_time_stamp_.rescale_ts(packet2, frame_index);
            console.error("packet->pts = %d, packet2->pts = %d, diff = %d",
                          (int)packet->pts, (int)packet2->pts, (int)(packet->pts - packet2->pts));
#endif
#endif
            av_packet_rescale_ts(packet, v_ocodec_ctx->time_base, v_out_stream->time_base);
            //console.debug("videoPacket->duration = %d", packet->duration);

            {
                std::unique_lock<std::mutex> lock(w_mutex_);
                //ret = av_interleaved_write_frame(av_ofmt_ctx, packet);
                ret = av_write_frame(av_ofmt_ctx, packet);
            }
            if (ret < 0) {
                console.error("保存编码器视频帧时出错: %d\n", ret);
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            console.error("[video] AVERROR(EAGAIN)\n");
            break;
        }
        else if (ret == AVERROR_EOF) {
            console.error("[video] EOF\n");
            break;
        }
        else if (ret < 0) {
            console.error("从编码器接收视频帧时出错: %d\n", ret);
            break;
        }
    } while (0);
    return ret;
}

int transform_audio_format(AVFrame * src_frame, AVSampleFormat src_format,
                           AVFrame * dest_frame, AVSampleFormat dest_format)
{
    int ret = 0;
#if 1
    SwrContext * swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        console.error("Could not allocate SwrContext");
        return AVERROR(ENOMEM);
    }

    // 配置 SwrContext
    ret = av_opt_set_int(swr_ctx, "in_channel_count",   src_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_count",  dest_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "in_channel_layout",  AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "in_sample_rate",  src_frame->sample_rate, 0);
    ret = av_opt_set_int(swr_ctx, "out_sample_rate", dest_frame->sample_rate, 0);
    ret = av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",  AV_SAMPLE_FMT_S16,  0);
    ret = av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        console.error("Could not initialize SwrContext: %d", ret);
        swr_free(&swr_ctx);
        return ret;
    }

    // 执行格式转换
    ret = swr_convert_frame(swr_ctx, dest_frame, src_frame);
    if (ret < 0) {
        console.error("Error converting PCM_S16LE to FLTP: %d", ret);
        swr_free(&swr_ctx);
        return ret;
    }

    // 释放 SwrContext
    swr_free(&swr_ctx);
#else
    // 初始化 SwrContext 用于样本格式转换
    SwrContext * swr_ctx = swr_alloc_set_opts(
        NULL,
        dest_frame->channel_layout,
        dest_format,
        dest_frame->sample_rate,
        src_frame->channel_layout,
        src_format,
        src_frame->sample_rate,
        0, NULL);
    if (swr_ctx == NULL) {
        return -1;
    }

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        console.error("Could not initialize SwrContext: %d", ret);
        swr_free(&swr_ctx);
        return ret;
    }

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

int CameraRecoder::encode_audio_frame(AVFormatContext * av_ofmt_ctx,
                                      AVCodecContext * a_icodec_ctx,
                                      AVCodecContext * a_ocodec_ctx,
                                      AVStream * a_in_stream,
                                      AVStream * a_out_stream,
                                      AVFrame * src_frame, size_t & frame_index)
{
    int ret;
    // 创建一个 AAC 格式的 Frame
    fSmartPtr<AVFrame> dest_frame = nullptr;
        
    if (src_frame != nullptr) {
        dest_frame = av_frame_alloc();
        if (dest_frame == NULL) {
            console.error("Failed to create destination audio frame");
            return AVERROR(ENOMEM);
        }

        AVSampleFormat src_format = a_icodec_ctx->sample_fmt;
        AVSampleFormat dest_format = a_ocodec_ctx->sample_fmt;

        ret = av_frame_copy_props(dest_frame, src_frame);

        dest_frame->channels = a_ocodec_ctx->channels;
        dest_frame->sample_rate = a_ocodec_ctx->sample_rate;
        dest_frame->channel_layout = a_ocodec_ctx->channel_layout;
        dest_frame->nb_samples = a_ocodec_ctx->frame_size;
        dest_frame->format = (int)dest_format;

        if (!av_q2d_eq(a_icodec_ctx->time_base, a_ocodec_ctx->time_base)) {
            av_frame_rescale_ts(dest_frame, a_icodec_ctx->time_base, a_ocodec_ctx->time_base);
        }

        // 为 AAC 格式的 Frame 分配内存
        ret = av_frame_get_buffer(dest_frame, 32);
        if (ret < 0) {
            console.error("Could not allocate destination audio frame data: %d", ret);
            return ret;
        }

        ret = av_frame_make_writable(dest_frame);
        assert(ret == 0);

        if (swr_audio_ == nullptr) {
            swr_audio_ = new swr_audio();
            ret = swr_audio_->init(src_frame, src_format, dest_frame, dest_format);
            if (ret < 0) {
                console.error("Failed to convert PCM_S16LE to AAC: %d", ret);
                return ret;
            }
        }

        // 调用转换函数
#if 1
        if (swr_audio_ != nullptr) {
            ret = swr_audio_->convert(src_frame, src_format, dest_frame, dest_format);
        } else {
            return AVERROR(ENOMEM);
        }
#else
        ret = transform_audio_format(src_frame, src_format, dest_frame, dest_format);
#endif
        if (ret < 0) {
            console.error("Failed to convert PCM_S16LE to AAC: %d", ret);
            return ret;
        }
    }

    // 输出音频编码
    ret = avcodec_send_frame(a_ocodec_ctx, dest_frame);
    if (ret < 0) {
        console.error("发送音频数据包到编码器时出错: %d\n", ret);
        return ret;
    }
    do {
        fSmartPtr<AVPacket> packet = av_packet_alloc();
        ret = avcodec_receive_packet(a_ocodec_ctx, packet);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            packet->stream_index = a_out_stream->index;
#if 0
            ai_time_stamp_.rescale_ts(packet, frame_index);
#else
            if (packet->pts == AV_NOPTS_VALUE) {
                packet->pts = ai_time_stamp_.pts(frame_index);
                packet->dts = packet->pts;
                packet->duration = ai_time_stamp_.duration();
            }
#if 0
            fSmartPtr<AVPacket> packet2 = av_packet_alloc();
            ai_time_stamp_.rescale_ts(packet2, frame_index);
            console.error("packet->pts = %d, packet2->pts = %d, diff = %d",
                          (int)packet->pts, (int)packet2->pts,
                          (int)((packet->pts - packet2->pts) / a_ocodec_ctx->frame_size));
#endif
#endif
            if (!av_q2d_eq(a_ocodec_ctx->time_base, a_out_stream->time_base)) {
                av_packet_rescale_ts(packet, a_ocodec_ctx->time_base, a_out_stream->time_base);
            }
            //console.debug("audioPacket->duration = %d", packet->duration);

            {
                std::unique_lock<std::mutex> lock(w_mutex_);
                //ret = av_interleaved_write_frame(av_ofmt_ctx, packet);
                ret = av_write_frame(av_ofmt_ctx, packet);
            }
            if (ret < 0) {
                console.error("保存编码器音频帧时出错: %d\n", ret);
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            console.error("[audio] AVERROR(EAGAIN)");
            break;
        }
        else if (ret == AVERROR_EOF) {
            console.error("[audio] EOF\n");
            break;
        }
        else if (ret < 0) {
            console.error("从编码器接收音频帧时出错: %d\n", ret);
            break;
        }
    } while (0);
    return ret;
}

int CameraRecoder::flush_video_encoder(size_t & frame_index)
{
    int ret;
    if (!(v_output_codec_->capabilities & AV_CODEC_CAP_DELAY)) {
        // Don't need delay
        return 0;
    }

    while (1) {
        ret = encode_video_frame(av_ofmt_ctx_, v_icodec_ctx_, v_ocodec_ctx_,
                                         v_in_stream_, v_out_stream_,
                                         NULL, frame_index);
        if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret < 0) {
            console.error("Failed to call flush_video_encoder(): %d ", ret);
            break;
        }
    };
    return ret;
}

int CameraRecoder::flush_audio_encoder(size_t & frame_index)
{
    int ret;
    if (!(a_output_codec_->capabilities & AV_CODEC_CAP_DELAY)) {
        // Don't need delay
        return 0;
    }

    while (1) {
        ret = encode_audio_frame(av_ofmt_ctx_, a_icodec_ctx_, a_ocodec_ctx_,
                                         a_in_stream_, a_out_stream_,
                                         NULL, frame_index);
        if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret < 0) {
            console.error("Failed to call flush_audio_encoder(): %d ", ret);
            break;
        }
    };
    return ret;
}

void CameraRecoder::video_enc_loop()
{
    // 读取输入视频，解码后，重新编码，写入文件
    int ret, ret2;
    StopWatch sw_global;
    int64_t v_cur_pts = 0;
    size_t v_frame_index = 0;
    AVRational time_base_q = { 1, AV_TIME_BASE };
    static const int64_t kRemainTime = 1300 + 200;
    bool is_exit = false;

    AVRational frame_rate = v_ocodec_ctx_->framerate;
    // = AV_TIME_BASE / (num / den) = AV_TIME_BASE * den / num;
    double frame_duration = (double)AV_TIME_BASE * frame_rate.den / frame_rate.num;
    int64_t frame_duration_us = (int64_t)frame_duration;

    v_enc_entered_.store(true);
    console.info("video_enc_loop() enter");

    int64_t start_time = av_gettime_relative();
    while (1) {
        //sw_global.start();
        {
            //std::unique_lock<std::mutex> lock(v_mutex_);
            if (v_stopflag_.load()) {
                break;
            }
        }
        // Video
        do {
            fSmartPtr<AVPacket> packet = av_packet_alloc();
            ret = av_read_frame(v_ifmt_ctx_, packet);
            if (ret < 0) {
                is_exit = true;
                break;
            }
            if (packet->stream_index == v_stream_index_) {
#if 0
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(packet, v_in_stream_->time_base, v_icodec_ctx_->time_base);
#else
                // 转换为标准time_base, 并减去起始时间
                av_packet_rescale_ts(packet, v_in_stream_->time_base, time_base_q);
                if (v_start_time_ != 0) {
                    packet->pts = packet->pts - v_start_time_;
                    packet->dts = packet->dts - v_start_time_;
                }
                else {
                    v_start_time_ = packet->pts;
                    packet->pts = 0;
                    packet->dts = packet->pts;
                }
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(packet, time_base_q, v_icodec_ctx_->time_base);
#endif
#if 1
                if (packet->pts == v_last_dec_pts_) {
                    int64_t elapsed_time = av_gettime_relative() - start_time;
                    int64_t frame_pts = (int64_t)(frame_duration * (v_frame_index + 1));
                    int64_t sleep_us = frame_pts - (elapsed_time + kRemainTime);
                    sleep_us = (sleep_us < (frame_duration_us - kRemainTime)) ? sleep_us : (frame_duration_us - kRemainTime);
                    if (sleep_us > 0) {
                        console.info("* Video sleep_us: %d", (int)(sleep_us / 2));
                        av_usleep((unsigned)sleep_us / 2);
                    }
                    else {
                        console.info("** Video sleep_us: %d", 100);
                        av_usleep(100);
                    }
                    break;
                }
                v_last_dec_pts_ = packet->pts;
#endif
                // 发送输入视频帧到解码器
                ret = avcodec_send_packet(v_icodec_ctx_, packet);
                if (ret < 0) {
                    console.error("Failed to send input video decoder send packet: %d", ret);
                    is_exit = true;
                    break;
                }

                while (1) {
                    fSmartPtr<AVFrame> frame = av_frame_alloc();
                    ret = av_frame_is_writable(frame);
                    // 解码器接受输入视频帧
                    ret = avcodec_receive_frame(v_icodec_ctx_, frame);
                    if (ret == 0) {
                        ret = av_frame_make_writable(frame);
                        // Get the stream of the input codec context

                        // 将解码后的视频帧转换为输出视频的格式, 并保存
                        ret2 = encode_video_frame(av_ofmt_ctx_, v_icodec_ctx_, v_ocodec_ctx_,
                                                          v_in_stream_, v_out_stream_,
                                                          frame, v_frame_index);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            is_exit = true;
                            break;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                v_frame_index++;
                                //console.debug("v_frame_index = %d", v_frame_index + 1);
                                //sw_global.stop();
                                //int64_t time_us = sw_global.elapsed_time_stamp();
                                //sw_global.print_elapsed_time_ms("Video processing");

                                int64_t elapsed_time = av_gettime_relative() - start_time;
                                int64_t frame_pts = (int64_t)(frame_duration * v_frame_index);
                                int64_t sleep_us = frame_pts - (elapsed_time + kRemainTime);
                                sleep_us = (sleep_us < (frame_duration_us - kRemainTime)) ? sleep_us : (frame_duration_us - kRemainTime);
                                if (sleep_us > 0) {
                                    console.info("Video sleep_us: %d", (int)sleep_us);
                                    av_usleep((unsigned)sleep_us);
                                }
                            }
                            else {
                                assert(ret2 == AVERROR(EAGAIN));
                                continue;
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        console.error("[video] avcodec_receive_frame(): AVERROR(EAGAIN)\n");
                        break;
                    }
                    else if (ret == AVERROR_EOF) {
                        console.error("[video] avcodec_receive_frame(): EOF\n");
                        is_exit = true;
                        break;
                    } else if (ret < 0) {
                        console.error("Failed to receive frame from input video decoder: %d", ret);
                        is_exit = true;
                        break;
                    }
                }
                if (is_exit)
                    break;
            }
        } while (0);

        if (is_exit)
            break;
    }

    ret = flush_video_encoder(v_frame_index);

    console.info("video_enc_loop() exit");
    v_enc_entered_.store(false);
}

void CameraRecoder::audio_enc_loop()
{
    // 读取输入音频，解码后，重新编码，写入文件
    int ret, ret2;
    StopWatch sw_global;
    int64_t a_cur_pts = 0;
    size_t a_frame_index = 0;
    AVRational time_base_q = { 1, AV_TIME_BASE };
    static const int64_t kRemainTime = 1300 + 200;
    bool is_exit = false;

    AVRational r_sample_rate = { a_ocodec_ctx_->sample_rate, 1 };
    // = AV_TIME_BASE / (num / den) = AV_TIME_BASE * den / num;
    double sample_time = (double)AV_TIME_BASE * r_sample_rate.den / r_sample_rate.num;
    double frame_duration = sample_time * a_ocodec_ctx_->frame_size;
    int64_t frame_duration_us = (int64_t)frame_duration;

    a_enc_entered_.store(true);
    console.info("audio_enc_loop() enter");

    int64_t start_time = av_gettime_relative();
    while (1) {
        {
            //std::unique_lock<std::mutex> lock(a_mutex_);
            if (a_stopflag_.load()) {
                break;
            }
        }
        // Audio
        do {
            fSmartPtr<AVPacket> packet = av_packet_alloc();
            sw_global.start();
            ret = av_read_frame(a_ifmt_ctx_, packet);
            sw_global.print_elapsed_time_ms("Audio processing");
            if (ret < 0) {
                is_exit = true;
                break;
            }
            //a_raw_frame_index++;
            //console.debug("a_raw_frame_index = %d", a_raw_frame_index);
            if (packet->stream_index == a_stream_index_) {
#if 0
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(packet, a_in_stream_->time_base, a_icodec_ctx_->time_base);
#else
                // 转换为标准time_base, 并减去起始时间
                av_packet_rescale_ts(packet, a_in_stream_->time_base, time_base_q);
                if (a_start_time_ != 0) {
                    packet->pts = packet->pts - a_start_time_;
                    packet->dts = packet->dts - a_start_time_;
                }
                else {
                    a_start_time_ = packet->pts;
                    packet->pts = 0;
                    packet->dts = packet->pts;
                }
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(packet, time_base_q, a_icodec_ctx_->time_base);
#endif
                // 发送输入音频帧到解码器
                int ret = avcodec_send_packet(a_icodec_ctx_, packet);
                if (ret < 0) {
                    console.error("Failed to send input audio decoder send packet: %d", ret);
                    is_exit = true;
                    break;
                }
                while (1) {
                    //console.debug("a_raw_frame_index = %d", a_raw_frame_index);
                    fSmartPtr<AVFrame> frame = av_frame_alloc();
                    ret = av_frame_is_writable(frame);
                    ret = avcodec_receive_frame(a_icodec_ctx_, frame);
                    if (ret == 0) {
                        ret = av_frame_make_writable(frame);
                        //sw_global.print_elapsed_time_ms("Audio processing");
                        // 将编码后的音频帧转换为输出音频的格式, 并保存
                        ret2 = encode_audio_frame(av_ofmt_ctx_, a_icodec_ctx_, a_ocodec_ctx_,
                                                          a_in_stream_, a_out_stream_,
                                                          frame, a_frame_index);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            is_exit = true;
                            break;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                a_frame_index++;
                                //console.debug("a_frame_index = %d", a_frame_index + 1);
                                //sw_global.stop();
                                //int64_t time_us = sw_global.elapsed_time_stamp();
                                //sw_global.print_elapsed_time_ms("Audio processing");
#if 1
                                int64_t elapsed_time = av_gettime_relative() - start_time;
                                int64_t frame_pts = (int64_t)(frame_duration * a_frame_index);
                                int64_t sleep_us = frame_pts - (elapsed_time + kRemainTime);
                                sleep_us = (sleep_us < (frame_duration_us - kRemainTime)) ? sleep_us : (frame_duration_us - kRemainTime);
                                if (sleep_us > 0) {
                                    console.info("Audio sleep_us: %d", (int)sleep_us);
                                    av_usleep((unsigned)sleep_us);
                                }
                                else {
                                    console.info("Audio sleep_us: %d", (int)sleep_us);
                                }
#endif
                            }
                            else {
                                assert(ret2 == AVERROR(EAGAIN));
                                continue;
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        console.error("[audio] avcodec_receive_frame(): AVERROR(EAGAIN)\n");
                        break;
                    }
                    else if (ret == AVERROR_EOF) {
                        console.error("[audio] avcodec_receive_frame(): EOF\n");
                        is_exit = true;
                        break;
                    }
                    else if (ret < 0) {
                        console.error("Failed to receive frame from input audio decoder: %d", ret);
                        is_exit = true;
                        break;
                    }
                }
                if (is_exit)
                    break;
            }
        } while (0);

        if (is_exit)
            break;
    }

    ret = flush_audio_encoder(a_frame_index);

    console.info("audio_enc_loop() exit");
    a_enc_entered_.store(false);
}

int CameraRecoder::start()
{
    int ret = AVERROR_UNKNOWN;
    if (!inited_) {
        return AVERROR(EINVAL);
    }

    if (v_out_stream_ == nullptr || a_out_stream_ == nullptr) {
        return AVERROR(EINVAL);
    }

	console.print("========== Output Information ==========\n");
	av_dump_format(av_ofmt_ctx_, 0, output_file_.c_str(), 1);
	console.print("========================================\n");

    // 打开输出文件
    if (!(av_ofmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&av_ofmt_ctx_->pb, output_file_.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            console.error("Failed to open the output file: %d", ret);
            return ret;
        }
    }

    // 写入文件头
    ret = avformat_write_header(av_ofmt_ctx_, NULL);
    if (ret < 0) {
        console.error("Failed to write the output file header: %d", ret);
        return ret;
    }

	console.print("========== Output Information ==========\n");
	av_dump_format(av_ofmt_ctx_, 0, output_file_.c_str(), 1);
	console.print("========================================\n");

    AVRational r_sample_rate = { a_ocodec_ctx_->sample_rate, 1 };
    vi_time_stamp_.init(v_ocodec_ctx_->framerate, v_ocodec_ctx_->time_base);
    if (a_ocodec_ctx_->frame_size != 0)
        ai_time_stamp_.init(r_sample_rate, a_ocodec_ctx_->time_base, a_ocodec_ctx_->frame_size);
    else
        ai_time_stamp_.init(r_sample_rate, a_ocodec_ctx_->time_base, 1024);

    // 读取输入视频、音频，解码后，重新编码，写入文件
#if 0
    // Duration between 2 frames (us)
    double v_duration = ((double)AV_TIME_BASE / av_q2d(v_in_stream_->r_frame_rate));
    double v_time_base = (av_q2d(v_in_stream_->time_base) * AV_TIME_BASE);
    int64_t v_duration_64 = (int64_t)(v_duration / v_time_base);

    // Duration between 2 frames (us)
    AVRational r_sample_rate = { a_in_stream_->codecpar->sample_rate, 1 };
    double a_duration = ((double)AV_TIME_BASE / av_q2d(r_sample_rate));
    double a_time_base = (av_q2d(a_in_stream_->time_base) * AV_TIME_BASE);
    int64_t a_duration_64 = (int64_t)(a_duration / a_time_base);
#endif

    timestamp_reset();

    if (!a_enc_entered_.load()) {
        a_stopflag_.store(false);
        a_enc_thread_ = std::thread(std::bind(&CameraRecoder::audio_enc_loop, this));
    }

    if (!v_enc_entered_.load()) {
        v_stopflag_.store(false);
        v_enc_thread_ = std::thread(std::bind(&CameraRecoder::video_enc_loop, this));
    }

    int ch;
    do {
        ch = _getch();
    } while (ch != (int)'q');

    console.info("Waiting the video thread exit.");

    // 通知视频线程退出
    {
        std::lock_guard<std::mutex> lock(v_mutex_);
        v_stopflag_.store(true);  // 设置退出标志
    }

    console.info("Waiting the video/audio thread(s) exit.");

    // 通知音频线程退出
    {
        std::lock_guard<std::mutex> lock(a_mutex_);
        a_stopflag_.store(true);  // 设置退出标志
    }

    if (a_enc_thread_.joinable()) {
        a_enc_thread_.join();
        console.info("The audio encoder thread exit.");
    }

    if (v_enc_thread_.joinable()) {
        v_enc_thread_.join();
        console.info("The video encoder thread exit.");
    }

    // 写入文件尾
    {
        std::unique_lock<std::mutex> lock(w_mutex_);
        ret = av_write_trailer(av_ofmt_ctx_);
    }
    return ret;
}

void CameraRecoder::timestamp_reset()
{
    v_start_time_ = 0;
    a_start_time_ = 0;

    v_last_dec_pts_ = -1;

    v_last_pts_ = -1;
    v_pts_addition_ = 0;
}

int CameraRecoder::stop()
{
    if (v_enc_entered_.load()) {
        {
            std::lock_guard<std::mutex> lock(v_mutex_);
            v_stopflag_.store(false);
        }
        if (v_enc_thread_.joinable())
            v_enc_thread_.join();
        v_enc_entered_ = false;
    }

    if (a_enc_entered_.load()) {
        {
            std::lock_guard<std::mutex> lock(a_mutex_);
            a_stopflag_.store(false);
        }
        if (a_enc_thread_.joinable())
            a_enc_thread_.join();
        a_enc_entered_ = false;
    }

    timestamp_reset();
    return 0;
}
