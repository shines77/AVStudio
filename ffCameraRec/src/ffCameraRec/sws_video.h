#pragma once

struct SwsContext;
struct AVFrame;

enum AVPixelFormat;

class sws_video
{
public:
    sws_video();
    ~sws_video();

    int init(AVFrame * src_frame, AVPixelFormat src_format,
             AVFrame * dest_frame, AVPixelFormat dest_format);
    int convert(AVFrame * src_frame, AVPixelFormat src_format,
                AVFrame * dest_frame, AVPixelFormat dest_format);
    void cleanup();

private:
    SwsContext * sws_ctx_;
};
