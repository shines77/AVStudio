
#include "ffMuxer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>             // For av_gettime_relative()
#include <libavutil/log.h>
}

#include <stdint.h>
#include <string>

#include "AVConsole.h"
#include "error_code.h"
#include "fAllocator.h"

//
// See: https://blog.csdn.net/leixiaohua1020/article/details/39802913
//
ffMuxer::ffMuxer()
{
    v_ifmt_ctx_ = nullptr;
    a_ifmt_ctx_ = nullptr;

    av_ofmt_ctx_ = nullptr;
    av_ofmt_ = nullptr;

    v_ofmt_ctx_ = nullptr;
    a_ofmt_ctx_ = nullptr;

    v_in_stream_ = nullptr;
    a_in_stream_ = nullptr;

    v_out_stream_ = nullptr;
    a_out_stream_ = nullptr;

    start_time_ = 0;

    in_video_index_ = -1;
    in_audio_index_ = -1;

    out_video_index_ = -1;
    out_audio_index_ = -1;
}

ffMuxer::~ffMuxer()
{
    release();
}

void ffMuxer::release()
{
    stop();
    cleanup();
}

void ffMuxer::stop()
{
    //
}

void ffMuxer::cleanup()
{
    if (v_ifmt_ctx_) {
        avformat_close_input(&v_ifmt_ctx_);
        v_ifmt_ctx_ = nullptr;
    }

    if (a_ifmt_ctx_) {
        avformat_close_input(&a_ifmt_ctx_);
        a_ifmt_ctx_ = nullptr;
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
}

const char * get_stream_type(bool is_video)
{
    return (is_video ? "video" : "audio");
}

int ffMuxer::create_from_in_stream(AVFormatContext * av_ofmt_ctx, AVStream ** pp_out_stream,
                                   AVStream * in_stream, bool is_video /* = true */)
{
    int ret;
    if (pp_out_stream == nullptr || in_stream == nullptr) {
        console.error("Invalid parameter for the input %s decoder.", get_stream_type(is_video));
        return AVERROR(EINVAL);
    }

    do {
        AVCodec * in_dec_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
        if (!in_dec_codec) {
            console.error("Failed to find the input %s decoder.", get_stream_type(is_video));
            ret = AVERROR_UNKNOWN;
            break;
        }
        fSmartPtr<AVCodecContext> in_codec_ctx = avcodec_alloc_context3(in_dec_codec);
        ret = avcodec_parameters_to_context(in_codec_ctx, in_stream->codecpar);
        if (av_ofmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
            in_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        ret = avcodec_open2(in_codec_ctx, in_dec_codec, NULL);
        if (ret < 0) {
            console.error("Failed to open the input %s decoder context.", get_stream_type(is_video));
            break;
        }
        AVStream * out_stream = avformat_new_stream(av_ofmt_ctx, in_dec_codec);
        if (!out_stream) {
            console.error("Failed allocating output %s stream", get_stream_type(is_video));
            ret = AVERROR_UNKNOWN;
            break;
        }     

        // Copy the settings of AVCodecContext
        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        // 下面用法已被废弃
        // ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            console.error("Failed to copy context from input to output %s stream codec context",
                          get_stream_type(is_video));
            break;
        }
        out_stream->codecpar->codec_tag = 0;

        // Write back
        *pp_out_stream = out_stream;
    } while (0);

    return ret;
}

int ffMuxer::init(const std::string & input_video_file, const std::string & input_audio_file,
                  const std::string & output_file, MuxerSettings * output_settings)
{
    int ret = ErrCode(E_ERROR);
    release();

    do {
        // 打开视频文件
        ret = avformat_open_input(&v_ifmt_ctx_, input_video_file.c_str(), NULL, NULL);
        if (ret < 0) {
            console.error("Could not open video file: %d", ret);
            break;
        }
        ret = avformat_find_stream_info(v_ifmt_ctx_, NULL);
        if (ret < 0) {
            console.error("Failed to retrieve input video stream info: %d", ret);
            break;
        }

        // 打开音频文件
        ret = avformat_open_input(&a_ifmt_ctx_, input_audio_file.c_str(), NULL, NULL);
        if (ret < 0) {
            console.error("Could not open audio file: %d", ret);
            break;
        }
        ret = avformat_find_stream_info(a_ifmt_ctx_, NULL);
        if (ret < 0) {
            console.error("Failed to retrieve input audio stream info: %d", ret);
            break;
        }

        console.print("=========== Input Information ==========\n");
        av_dump_format(v_ifmt_ctx_, 0, input_video_file.c_str(), 0);
        av_dump_format(a_ifmt_ctx_, 0, input_audio_file.c_str(), 0);
        console.print("========================================\n");

        // 创建输出文件
        ret = avformat_alloc_output_context2(&av_ofmt_ctx_, NULL, NULL, output_file.c_str());
        if (ret < 0) {
            console.error("Could not create output context: %d", ret);
            break;
        }
        av_ofmt_ = av_ofmt_ctx_->oformat;

        // 从输入视频文件中找到第一个视频流
        for (unsigned int i = 0; i < v_ifmt_ctx_->nb_streams; i++) {
            // Create output AVStream according to input AVStream
            if (v_ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                in_video_index_ = i;

                AVStream * in_stream = v_ifmt_ctx_->streams[i];
                AVStream * out_stream = nullptr;
                ret = create_from_in_stream(av_ofmt_ctx_, &out_stream, in_stream, true);
                if (ret < 0) {
                    console.error("Failed to create new stream from input video stream: %d", ret);
                    goto cleanup_and_exit;
                }
                if (out_stream != nullptr) {
                    out_stream->r_frame_rate = in_stream->r_frame_rate;
                    out_video_index_ = out_stream->index;

                    v_in_stream_ = in_stream;
                    v_out_stream_ = out_stream;
                }
                break;
            }
        }

        // 从输入音频文件中找到第一个音频流
        for (unsigned int i = 0; i < a_ifmt_ctx_->nb_streams; i++) {
            // Create output AVStream according to input AVStream
            if (a_ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                in_audio_index_ = i;

                AVStream * in_stream = a_ifmt_ctx_->streams[i];
                AVStream * out_stream = nullptr;
                ret = create_from_in_stream(av_ofmt_ctx_, &out_stream, in_stream, true);
                if (ret < 0) {
                    console.error("Failed to create new stream from input audio stream: %d", ret);
                    goto cleanup_and_exit;
                }
                if (out_stream != nullptr) {
                    out_audio_index_ = out_stream->index;

                    a_in_stream_ = in_stream;
                    a_out_stream_ = out_stream;
                }
                break;
            }
        }

	    console.print("========== Output Information ==========\n");
	    av_dump_format(av_ofmt_ctx_, 0, output_file.c_str(), 1);
	    console.print("========================================\n");

        output_file_ = output_file;
    } while (0);

cleanup_and_exit:
    return ret;
}

int ffMuxer::start()
{
    int ret;
    start_time_ = av_gettime_relative();

    do {
        // 创建输出文件
        ret = avformat_alloc_output_context2(&av_ofmt_ctx_, NULL, NULL, output_file_.c_str());
        if (ret < 0) {
            console.error("Could not create output context: %d", ret);
            break;
        }

        // 写入文件头
        ret = avformat_write_header(av_ofmt_ctx_, NULL);
        if (ret < 0) {
            console.error("Error occurred when writing header to output file: %d", ret);
            break;
        }
    } while (0);

    return ret;
}
