#pragma once

extern "C" {
#include <libavutil/rational.h>
}

#include <string>

void str_to_lower(std::string & str);
void str_to_upper(std::string & str);

bool av_q2d_eq(AVRational a, AVRational b);
bool av_q2d_lt(AVRational a, AVRational b);
bool av_q2d_gt(AVRational a, AVRational b);

bool av_q2d_lte(AVRational a, AVRational b);
bool av_q2d_gte(AVRational a, AVRational b);

AVRational av_rescale_time_base(const AVRational & time_base);
AVRational av_fixed_time_base(const AVRational & time_base);
