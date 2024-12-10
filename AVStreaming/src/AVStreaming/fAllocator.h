
#pragma once

#include "fSmartPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
}

/* AVFormatContext (0) */

template <>
struct fAllocator<AVFormatContext, 0>
{
    inline void _free(AVFormatContext * ptr) {
        if (ptr) {
            avformat_close_input(&ptr);
        }
    }
};

/* AVFormatContext: (1) */

template <>
struct fAllocator<AVFormatContext, 1>
{
    inline void _free(AVFormatContext * ptr) {
        if (ptr) {
            avformat_free_context(ptr);
        }
    }
};

/* AVCodecContext */

template <>
struct fAllocator<AVCodecContext>
{
    inline void _free(AVCodecContext * ptr) {
        if (ptr) {
            avcodec_free_context(&ptr);
        }
    }
};

/* AVFrame */

template <>
struct fAllocator<AVFrame>
{
    inline void _free(AVFrame * ptr) {
        if (ptr) {
            av_frame_free(&ptr);
        }
    }
};

/* AVFrame */

template <>
struct fAllocator<AVPacket>
{
    inline void _free(AVPacket * ptr) {
        if (ptr) {
            av_packet_unref(ptr);
        }
    }
};
