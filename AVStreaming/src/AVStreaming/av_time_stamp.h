#pragma once

#include <stdint.h>
#include <assert.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/rational.h>
}

struct av_time_stamp
{
    av_time_stamp() {
        duration_f = 0.0;
        scale_factor = 0.0;
        duration_64 = 0;
    }
    ~av_time_stamp() {
        //
    }

    double duration_f;
    double scale_factor;
    int64_t duration_64;

    void init(const AVRational & frame_rate, const AVRational & time_base, size_t sample_size = 1) {
        static const double kDoubleEpsilon = 0.000001;

        duration_f = ((double)AV_TIME_BASE / av_q2d(frame_rate) + kDoubleEpsilon) * sample_size;
        scale_factor = (av_q2d(time_base) * AV_TIME_BASE);
        duration_64 = (int64_t)(duration_f / scale_factor);
        if (duration_64 <= 0)
            duration_64 = 1;
    }

    inline int64_t pts(size_t frame_index) const {
        return (int64_t)((duration_f * frame_index + 0.5) / scale_factor);
    }

    inline int64_t duration() const {
        return duration_64;
    }

    void rescale_ts(AVPacket * packet, size_t frame_index) {
        assert(packet != nullptr);
        packet->pts = pts(frame_index);
        packet->dts = packet->pts;
        packet->duration = duration();
        //++frame_index;
    }
};
