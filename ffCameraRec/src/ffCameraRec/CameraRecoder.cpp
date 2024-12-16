
#include "CameraRecoder.h"

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

#include "global.h"
#include "string_utils.h"
#include "fSmartPtr.h"
#include "DShowDevice.h"

CameraRecoder::CameraRecoder()
{
    v_input_fmt_ = nullptr;
    a_input_fmt_ = nullptr;
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
    if (v_input_fmt_) {
        v_input_fmt_ = nullptr;
    }
}

int CameraRecoder::init(const std::string & output_file)
{
    int ret;

    // 设置设备枚举选项
    fSmartPtr<AVDictionary> options = nullptr;
    ret = av_dict_set(&options, "list_devices", "true", 0);

    // 打开视频设备
    AVInputFormat * v_input_fmt = av_find_input_format("dshow");
    if (v_input_fmt == NULL) {
        console.error("Could not find input video format");
        return -1;
    }
    v_input_fmt_ = v_input_fmt;

    std::string videoDeviceName, audioDeviceName;
    char video_url[256];
    char audio_url[256];

    // 使用 dshow 枚举所有视频和音频设备
    {
        DShowDevice dshowDevice;
        dshowDevice.init();

        if (dshowDevice.videoDeviceList_.size() > 0)
            videoDeviceName = string_utils::ansi_to_utf8(dshowDevice.videoDeviceList_[0]);

        if (dshowDevice.audioDeviceList_.size() > 0)
            audioDeviceName = string_utils::ansi_to_utf8(dshowDevice.audioDeviceList_[0]);
    }

    // 选择视频设备
    if (!videoDeviceName.empty())
        snprintf(video_url, sizeof(video_url), "video=%s", videoDeviceName.c_str());
    else
        snprintf(video_url, sizeof(video_url), "video=dummy");

    // 选择音频设备
    if (!audioDeviceName.empty())
        snprintf(audio_url, sizeof(audio_url), "audio=%s", audioDeviceName.c_str());
    else
        snprintf(audio_url, sizeof(audio_url), "audio=dummy");

    AVFormatContext * v_ifmt_ctx = nullptr;

#if 0
    // 打开一个虚拟的输入流以枚举设备 (从 FFmpeg 4.2.10 已失效, 仅用于查询设备列表, 不能返回)
    ret = avformat_open_input(&v_ifmt_ctx, "video=dummy", v_input_fmt, &options);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, sizeof(error_buf));
        console.error("Could not open input video device: %s", error_buf);
        return ret;
    }
#endif

    ret = avformat_open_input(&v_ifmt_ctx, video_url, v_input_fmt, NULL);
    if (ret < 0) {
        console.error("Could not open input video device: %d", ret);
        return ret;
    }
    console.info("Selected video device: %s", videoDeviceName.c_str());

    if (v_ifmt_ctx) {
        std::cout << "========Device Info=============" << std::endl;
        if (v_ifmt_ctx->nb_streams > 0) {
            for (unsigned int i = 0; i < v_ifmt_ctx->nb_streams; ++i) {
                AVStream * stream = v_ifmt_ctx->streams[i];
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    std::cout << "Video Device: " << stream->codecpar->codec_tag << " " << stream->codecpar->extradata << std::endl;
                } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    std::cout << "Audio Device: " << stream->codecpar->codec_tag << " " << stream->codecpar->extradata << std::endl;
                }
            }
        }
        std::cout << "=================================" << std::endl;
    }

#if 0
    // 列出视频设备 (方法已失效)
    AVDeviceInfoList * videoDeviceList = NULL;
    ret = avdevice_list_input_sources(v_input_fmt, NULL, options, &videoDeviceList);
    if (ret < 0) {
        console.error("Could not list video devices: %d", ret);
        return ret;
    }
#endif

    // 列出音频设备
    AVInputFormat * a_input_fmt = av_find_input_format("dshow");
    if (a_input_fmt == NULL) {
        console.error("Could not find input audio format\n");
        return -1;
    }
    a_input_fmt_ = a_input_fmt;

    av_dict_free(&options);

    avformat_close_input(&v_ifmt_ctx);
    return 0;
}

int CameraRecoder::stop()
{
    return 0;
}
