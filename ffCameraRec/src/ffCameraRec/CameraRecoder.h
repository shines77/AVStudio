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
    int encode_video_frame(AVFormatContext * av_ofmt_ctx,
                           AVCodecContext * v_icodec_ctx,
                           AVCodecContext * v_ocodec_ctx,
                           AVStream * v_in_stream,
                           AVStream * v_out_stream,
                           AVFrame * src_frame, size_t & frame_index);
    int encode_audio_frame(AVFormatContext * av_ofmt_ctx,
                           AVCodecContext * a_icodec_ctx,
                           AVCodecContext * a_ocodec_ctx,
                           AVStream * a_in_stream,
                           AVStream * a_out_stream,
                           AVFrame * src_frame, size_t & frame_index);

    int flush_video_encoder(size_t & frame_index);
    int flush_audio_encoder(size_t & frame_index);

    void video_enc_loop();
    void audio_enc_loop();

    int create_encoders();
    void timestamp_reset();

    static const size_t kCacheLineSize = 64;

    static const size_t kPointPaddingSize = kCacheLineSize - sizeof(void *);
    static const size_t kMutexPaddingSize = kCacheLineSize - (sizeof(std::mutex) % kCacheLineSize);
    static const size_t kAtomicPaddingSize = kCacheLineSize - (sizeof(std::atomic<bool>) % kCacheLineSize);
    static const size_t kCondVarPaddingSize = kCacheLineSize - (sizeof(std::condition_variable) % kCacheLineSize);

protected:
    bool inited_;

    // Video Input
    AVInputFormat *     av_input_fmt_;

    AVFormatContext *   v_ifmt_ctx_;
    AVStream *          v_in_stream_;
    AVCodec *           v_input_codec_;
    AVCodecContext *    v_icodec_ctx_;

    // Video Output
    AVFormatContext *   v_ofmt_ctx_;
    AVStream *          v_out_stream_;
    AVCodec *           v_output_codec_;
    AVCodecContext *    v_ocodec_ctx_;

    sws_video *         sws_video_;
    int                 v_stream_index_;

    av_time_stamp       vi_time_stamp_;

    int64_t             v_start_time_;
    int64_t             v_last_dec_pts_;

    int64_t             v_last_pts_;
    int64_t             v_pts_addition_;

    // Audio Input
    AVFormatContext *   a_ifmt_ctx_;
    AVStream *          a_in_stream_;  
    AVCodec *           a_input_codec_;
    AVCodecContext *    a_icodec_ctx_;

    // Audio Output
    AVFormatContext *   a_ofmt_ctx_;
    AVStream *          a_out_stream_;    
    AVCodec *           a_output_codec_;
    AVCodecContext *    a_ocodec_ctx_;

    swr_audio *         swr_audio_;
    int                 a_stream_index_;

    av_time_stamp       ai_time_stamp_;
    int64_t             a_start_time_;

    char padding_sep[kCacheLineSize];

    // Output file
    AVFormatContext *   av_ofmt_ctx_;
    char padding_av_ofmt_ctx[kPointPaddingSize];

    // Thread
    std::mutex w_mutex_;
    char padding_w_mutex[kMutexPaddingSize];

    std::mutex  v_mutex_;
    char padding_v_mutex[kMutexPaddingSize];

    std::mutex  a_mutex_;
    char padding_a_mutex[kMutexPaddingSize];

    std::atomic<bool> v_stopflag_;
    char padding_v_stopflag[kAtomicPaddingSize];

    std::atomic<bool> a_stopflag_;
    char padding_a_stopflag[kAtomicPaddingSize];    

    std::string output_file_;

    std::atomic<bool> v_enc_entered_;
    std::atomic<bool> a_enc_entered_;

    std::condition_variable v_cond_var_;
    char padding_v_cond_var[kCondVarPaddingSize];

    std::condition_variable a_cond_var_;
    char padding_a_cond_var[kCondVarPaddingSize];

    std::thread v_enc_thread_;
    std::thread a_enc_thread_;

    // From DShowDevice
    std::string videoDeviceName_;
    std::string audioDeviceName_;
};
