#pragma once

#include <stdint.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/rational.h>
}

struct av_time_stamp
{
    av_time_stamp() {
        duration_ = 0.0;
        tick_time_ = 0.0;
        duration_64_ = 0;
    }
    ~av_time_stamp() {
        //
    }

    double duration_;
    double tick_time_;
    int64_t duration_64_;

    void init(const AVRational & frame_rate, const AVRational & time_base) {
        static const double kDoubleEpsilon = 0.000001;

        duration_ = (double)AV_TIME_BASE / av_q2d(frame_rate) + kDoubleEpsilon;
        tick_time_ = (av_q2d(time_base) * AV_TIME_BASE);
        duration_64_ = (int64_t)(duration_ / tick_time_);
        if (duration_64_ <= 0)
            duration_64_ = 1;
    }

    int64_t pts(size_t frame_index) const {
        return (int64_t)((duration_ * frame_index) / tick_time_);
    }

    int64_t duration() const {
        return duration_64_;
    }
};
