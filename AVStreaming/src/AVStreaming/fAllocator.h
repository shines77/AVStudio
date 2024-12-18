
#pragma once

#include "fSmartPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
}

enum SmartPtr_Type {
    SMT_Input,
    SMT_Output,
    SMT_Decoder,
    SMT_Encoder,
    SMT_Muxer,
    SMT_Demuxer,
    SMT_Filter,
    SMT_BSFS,
    SMT_Video,
    SMT_Audio,
    SMT_Last
};

/* AVFormatContext (0) */

template <>
struct fAllocator<AVFormatContext, SMT_Input>
{
    inline void _free(AVFormatContext * ptr) {
        if (ptr) {
            avformat_close_input(&ptr);
        }
    }
};

/* AVFormatContext: (1) */

template <>
struct fAllocator<AVFormatContext, SMT_Output>
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

/* AVDictionary */

template <>
struct fAllocator<AVDictionary>
{
    inline void _free(AVDictionary * ptr) {
        if (ptr) {
            av_dict_free(&ptr);
        }
    }
};