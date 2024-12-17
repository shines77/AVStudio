#pragma once

#include <stdint.h>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "av_time_stamp.h"

struct AVInputFormat;
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVCodec;
struct AVFrame;

enum AVPixelFormat;
enum AVSampleFormat;

class sws_video;
class swr_audio;

class CameraRecoder
{
public:
    CameraRecoder();
    ~CameraRecoder();

    void release();
    void cleanup();

    int init(const std::string & output_file);
    int start();
    int stop();

protected:
    int avcodec_encode_video_frame(AVFormatContext * av_ofmt_ctx,
                                   AVCodecContext * v_icodec_ctx,
                                   AVCodecContext * v_ocodec_ctx,
                                   AVStream * v_in_stream,
                                   AVStream * v_out_stream,
                                   AVFrame * src_frame, size_t & frame_index);
    int avcodec_encode_audio_frame(AVFormatContext * av_ofmt_ctx,
                                   AVCodecContext * a_icodec_ctx,
                                   AVCodecContext * a_ocodec_ctx,
                                   AVStream * a_in_stream,
                                   AVStream * a_out_stream,
                                   AVFrame * src_frame, size_t & frame_index);

    void video_enc_loop();
    void audio_enc_loop();

    int create_encoders();

protected:
    std::atomic<bool> v_stopflag_;

    // Input
    AVInputFormat * av_input_fmt_;

    AVFormatContext * v_ifmt_ctx_;
    AVFormatContext * a_ifmt_ctx_;

    AVStream * v_in_stream_;
    AVStream * a_in_stream_;

    AVCodec * v_input_codec_;
    AVCodec * a_input_codec_;

    AVCodecContext * v_icodec_ctx_;
    AVCodecContext * a_icodec_ctx_;

    // Output
    AVFormatContext * v_ofmt_ctx_;
    AVFormatContext * a_ofmt_ctx_;

    AVStream * v_out_stream_;
    AVStream * a_out_stream_;

    AVCodec * v_output_codec_;
    AVCodec * a_output_codec_;

    AVCodecContext * v_ocodec_ctx_;
    AVCodecContext * a_ocodec_ctx_;

    std::mutex  v_mutex_;

    // Output file
    AVFormatContext * av_ofmt_ctx_;

    sws_video * sws_video_;
    swr_audio * swr_audio_;

    bool inited_;

    int v_stream_index_;
    int a_stream_index_;

    std::mutex w_mutex_;

    av_time_stamp vi_time_stamp_;
    av_time_stamp ai_time_stamp_;

    int64_t v_start_time_;
    int64_t a_start_time_;

    std::string output_file_;

    std::atomic<bool> v_enc_entered_;
    std::atomic<bool> a_enc_entered_;

    std::atomic<bool> a_stopflag_;

    std::condition_variable v_cond_var_;
    std::condition_variable a_cond_var_;

    std::thread v_enc_thread_;
    std::thread a_enc_thread_;

    // From DShowDevice
    std::string videoDeviceName_;
    std::string audioDeviceName_;

    std::mutex  a_mutex_;
};
