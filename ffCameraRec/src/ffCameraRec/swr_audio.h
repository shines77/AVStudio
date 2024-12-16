#pragma once

struct SwrContext;
struct AVFrame;

enum AVSampleFormat;

class swr_audio
{
public:
    swr_audio();
    ~swr_audio();

    int init(AVFrame * src_frame, AVSampleFormat src_format,
             AVFrame * dest_frame, AVSampleFormat dest_format);
    int convert(AVFrame * src_frame, AVSampleFormat src_format,
                AVFrame * dest_frame, AVSampleFormat dest_format);
    void cleanup();

private:
    SwrContext * swr_ctx_;
};
