
#pragma once

#include <string>

struct AVOutputFormat;
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

enum AVPixelFormat;
enum AVSampleFormat;

struct MuxerSettings
{
    // Video
    int codec_id_v;
    int bit_rate_v;
	int width;
	int height;
	int out_width;
	int out_height;
    int frame_rate;
	int qb;
	AVPixelFormat pix_fmt;   

    // Audio
    int codec_id_a;
    int bit_rate_a;
	int nb_channel;
	int sample_rate;
	AVSampleFormat sample_fmt;

    MuxerSettings() {
        codec_id_v = 0;
        bit_rate_v = 0;
        width = 0;
        height = 0;
        out_width = 0;
        out_height = 0;
        frame_rate = 0;
        qb = 0;
        pix_fmt = (AVPixelFormat)0;

        codec_id_a = 0;
        bit_rate_a = 0;
        nb_channel = 0;
        sample_rate = 0;
        sample_fmt = (AVSampleFormat)0;
    }
};

class ffMuxer
{
public:
    ffMuxer();
    ~ffMuxer();

    int init(const std::string & input_video_file, const std::string & input_audio_file,
             const std::string & output_file, MuxerSettings * output_settings);

    void stop();
    void cleanup();
    void release();

private:
    AVFormatContext *   v_ifmt_ctx_;
    AVFormatContext *   a_ifmt_ctx_;

    AVFormatContext *   av_ofmt_ctx_;
    AVOutputFormat *    av_ofmt_;

    AVFormatContext *   v_ofmt_ctx_;
    AVFormatContext *   a_ofmt_ctx_;

    int in_video_index_;
    int in_audio_index_;

    int out_video_index_;
    int out_audio_index_;
};
