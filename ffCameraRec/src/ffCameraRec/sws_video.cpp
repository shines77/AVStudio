
#include "sws_video.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
}

#include <stdint.h>
#include <assert.h>
#include "global.h"

sws_video::sws_video()
{
    sws_ctx_ = nullptr;
}

sws_video::~sws_video()
{
    cleanup();
}

int sws_video::init(AVFrame * src_frame, AVPixelFormat src_format,
                    AVFrame * dest_frame, AVPixelFormat dest_format)
{
    int ret = 0;
    cleanup();

    assert(sws_ctx_ == nullptr);
    // 创建 SwsContext 用于格式转换
    SwsContext * sws_ctx = sws_getContext(
        src_frame->width, src_frame->height, src_format,
        dest_frame->width, dest_frame->height, dest_format,
        SWS_BILINEAR, NULL, NULL, NULL);
    if (sws_ctx == nullptr) {
        console.error("Could not initialize the conversion context");
        return AVERROR(ENOMEM);
    }

    sws_ctx_ = sws_ctx;
    return ret;
}

int sws_video::convert(AVFrame * src_frame, AVPixelFormat src_format,
                       AVFrame * dest_frame, AVPixelFormat dest_format)
{
    int ret = AVERROR(EINVAL);
    // 执行格式转换
    if (sws_ctx_) {
        int64_t start_time = av_gettime_relative();
        ret = sws_scale(sws_ctx_,
                        (const uint8_t * const *)src_frame->data, src_frame->linesize,
                        0, src_frame->height,
                        dest_frame->data, dest_frame->linesize);
        if (ret < 0) {
            console.error("Error converting YUVY422 to YUV420P: %d", ret);
            return ret;
        }
        int64_t end_time = av_gettime_relative();
        //console.info("sws_video::convert elapsed time: %d", (int)(end_time - start_time));
    }
    return ret;
}

void sws_video::cleanup()
{
    // 释放 SwsContext
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);
        sws_ctx_ = nullptr;
    }
}
