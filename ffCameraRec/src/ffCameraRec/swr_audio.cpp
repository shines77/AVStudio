
#include "swr_audio.h"

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

#include <assert.h>
#include "global.h"

swr_audio::swr_audio()
{
    swr_ctx_ = nullptr;
}

swr_audio::~swr_audio()
{
    cleanup();
}

int swr_audio::init(AVFrame * src_frame, AVSampleFormat src_format,
                    AVFrame * dest_frame, AVSampleFormat dest_format)
{
    int ret = 0;
    cleanup();

    assert(swr_ctx_ == nullptr);
    // 创建 SwrContext 用于格式转换
    SwrContext * swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        console.error("Could not allocate SwrContext");
        return AVERROR(ENOMEM);
    }

    // 配置 SwrContext
    ret = av_opt_set_int(swr_ctx, "in_channel_count", src_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_count", dest_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "in_channel_layout", src_frame->channel_layout, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_layout", dest_frame->channel_layout, 0);
    ret = av_opt_set_int(swr_ctx, "in_sample_rate", src_frame->sample_rate, 0);
    ret = av_opt_set_int(swr_ctx, "out_sample_rate", dest_frame->sample_rate, 0);
    ret = av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_format,  0);
    ret = av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dest_format, 0);

    swr_ctx_ = swr_ctx;
    return ret;
}

int swr_audio::convert(AVFrame * src_frame, AVSampleFormat src_format,
                       AVFrame * dest_frame, AVSampleFormat dest_format)
{
    int ret = AVERROR(EINVAL);
    // 执行格式转换
    if (swr_ctx_) {
        int64_t start_time = av_gettime_relative();
        ret = swr_convert_frame(swr_ctx_, dest_frame, src_frame);
        if (ret < 0) {
            console.error("Error converting PCM_S16LE to FLTP: %d", ret);
            return ret;
        }
        int64_t end_time = av_gettime_relative();
        //console.info("swr_audio::convert() elapsed time: %d", (int)(end_time - start_time));
    }
    return ret;
}

void swr_audio::cleanup()
{
    // 释放 SwrContext
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
}
