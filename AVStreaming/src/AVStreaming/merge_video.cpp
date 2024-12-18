
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/log.h>
}

int merge_video()
{
    AVFormatContext *video_ctx = NULL, *audio_ctx = NULL, *output_ctx = NULL;
    AVStream *video_stream = NULL, *audio_stream = NULL;
    const char *video_filename = "input_video.h264";
    const char *audio_filename = "input_audio.aac";
    const char *output_filename = "output_h264.mp4";
    int ret, cnt = 0;
    int frameIndex;

    // 初始化 FFmpeg 库
    av_register_all();

    // 打开视频文件
    if ((ret = avformat_open_input(&video_ctx, video_filename, NULL, NULL)) < 0) {
        debug_print("Could not open video file: %d\n", ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(video_ctx, NULL)) < 0) {
        debug_print("Could not find video stream info: %d\n", ret);
        return -1;
    }

    // 打开音频文件
    if ((ret = avformat_open_input(&audio_ctx, audio_filename, NULL, NULL)) < 0) {
        debug_print("Could not open audio file: %d\n", ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(audio_ctx, NULL)) < 0) {
        debug_print("Could not find audio stream info: %d\n", ret);
        return -1;
    }

    // 创建输出文件
    if ((ret = avformat_alloc_output_context2(&output_ctx, NULL, NULL, output_filename)) < 0) {
        debug_print("Could not create output context: %d\n", ret);
        return -1;
    }

    // 添加视频流
    video_stream = avformat_new_stream(output_ctx, NULL);
    if (!video_stream) {
        debug_print("Failed to create video stream\n");
        return -1;
    }
    avcodec_parameters_copy(video_stream->codecpar, video_ctx->streams[0]->codecpar);
    video_stream->codecpar->codec_tag = 0;

    // 添加音频流
    audio_stream = avformat_new_stream(output_ctx, NULL);
    if (!audio_stream) {
        debug_print("Failed to create audio stream\n");
        return -1;
    }
    avcodec_parameters_copy(audio_stream->codecpar, audio_ctx->streams[0]->codecpar);
    audio_stream->codecpar->codec_tag = 0;

    // 打开输出文件
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&output_ctx->pb, output_filename, AVIO_FLAG_WRITE)) < 0) {
            debug_print("Could not open output file: %d\n", ret);
            return -1;
        }
    }

    // 写入文件头
    if ((ret = avformat_write_header(output_ctx, NULL)) < 0) {
        debug_print("Error occurred when writing header to output file: %d\n", ret);
        return -1;
    }

    AVStream * i_video_stream = video_ctx->streams[0];
    video_stream->r_frame_rate = i_video_stream->r_frame_rate;

    AVStream * i_audio_stream = audio_ctx->streams[0];

    // 读取并写入视频帧
    cnt = 0;
    frameIndex = 0;

    // Parameters
    double av_time_base = (av_q2d(video_stream->time_base) * AV_TIME_BASE);
    // Duration between 2 frames (us)
    double duration = ((double)AV_TIME_BASE / av_q2d(video_stream->r_frame_rate));
    int64_t duration_64 = (int64_t)(duration / av_time_base);

    while (1) {
        AVPacket pkt;
        ret = av_read_frame(video_ctx, &pkt);
        if (ret == 0) {
            if (pkt.pts == AV_NOPTS_VALUE) {
                // Write PTS
                pkt.pts = (int64_t)((duration * frameIndex) / av_time_base);
                pkt.dts = pkt.pts;
                pkt.duration = duration_64;
            }
            frameIndex++;
            pkt.stream_index = video_stream->index;
            ret = av_interleaved_write_frame(output_ctx, &pkt);
            if (ret < 0) {
                debug_print("Error occurred when writing video packet to output file: %d\n", ret);
                break;
            }
            av_packet_unref(&pkt);
        }
        else if (ret == AVERROR(EAGAIN)) {
            cnt++;
            if (cnt > 100)
                break;
            continue;;
        }
        else if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            debug_print("Error occurred when writing video to output file: %d\n", ret);
            break;
        }
    }

    // 读取并写入音频帧
    cnt = 0;
    while (1) {
        AVPacket pkt;
        ret = av_read_frame(audio_ctx, &pkt);
        if (ret == 0) {
            pkt.stream_index = audio_stream->index;
            av_packet_rescale_ts(&pkt, i_audio_stream->time_base, audio_stream->time_base);
            ret = av_interleaved_write_frame(output_ctx, &pkt);
            if (ret < 0) {
                debug_print("Error occurred when writing audio packet to output file: %d\n", ret);
            }
            av_packet_unref(&pkt);
        }
        else if (ret == AVERROR(EAGAIN)) {
            cnt++;
            if (cnt > 100)
                break;
            continue;;
        }
        else if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            debug_print("Error occurred when writing audio to output file: %d\n", ret);
            break;
        }
    }

    // 写入文件尾
    av_write_trailer(output_ctx);

    // 释放资源
    avformat_close_input(&video_ctx);
    avformat_close_input(&audio_ctx);
    if (output_ctx && !(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_ctx->pb);
    }
    avformat_free_context(output_ctx);

    return 0;
}

int main(int argc, char const * argv[])
{
#if LIBAVFORMAT_BUILD < AV_VERSION_INT(58, 9, 100)
    // 该函数从 avformat 库的 58.9.100 版本开始被废弃
    // (2018-02-06, 大约是 FFmepg 4.0 版本)
    av_register_all();
#endif

    int ret = merge_video();

    return 0;
}
