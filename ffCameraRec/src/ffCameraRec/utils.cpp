
#include "utils.h"

#include <assert.h>
#include <algorithm>
#include <cctype>

void str_to_lower(std::string & str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

void str_to_upper(std::string & str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
}

bool av_q2d_eq(AVRational a, AVRational b)
{
    return ((a.num == b.num) && (a.den == b.den));
}

bool av_q2d_lt(AVRational a, AVRational b)
{
    return (av_q2d(a) < av_q2d(b));
}

bool av_q2d_lte(AVRational a, AVRational b)
{
    return (av_q2d(a) <= av_q2d(b));
}

bool av_q2d_gt(AVRational a, AVRational b)
{
    return (av_q2d(a) > av_q2d(b));
}

bool av_q2d_gte(AVRational a, AVRational b)
{
    return (av_q2d(a) >= av_q2d(b));
}

AVRational av_rescale_time_base(const AVRational & time_base)
{
    static const size_t kMaxTimeBaseValue = 10000000;
    AVRational new_time_base = time_base;
    if (time_base.den > time_base.num) {
        if (time_base.den <= kMaxTimeBaseValue) {
            double scale_factor = (double)kMaxTimeBaseValue / time_base.den;
            int num = (int)((double)time_base.num * scale_factor + 0.5);
            new_time_base.num = num;
            new_time_base.den = kMaxTimeBaseValue;
        }
    }
    else if (time_base.den < time_base.num) {
        if (time_base.num <= kMaxTimeBaseValue) {
            double scale_factor = (double)kMaxTimeBaseValue / time_base.num;
            int den = (int)((double)time_base.den * scale_factor + 0.5);
            new_time_base.num = kMaxTimeBaseValue;
            new_time_base.den = den;
        }
    }
    return new_time_base;
}

void av_fixed_time_base_impl(int & num, int & den)
{
    assert(den > num);
    assert(num != 0);
    static const double kDoubleEpsilon = 0.000001;
    static const double kDoubleErrorThreshold = 0.001;

    double q2d = den / num;
    int64_t q2d_n = (int64_t)q2d;
    double q2d_r = (double)(q2d_n + 1);
    double positive_error = q2d - (double)q2d_n;
    double negative_error = q2d_r - q2d;
    if (positive_error >= 0.0 && positive_error < kDoubleErrorThreshold) {
        num = 1;
        den = (int)q2d_n;
    }
    else if (negative_error >= 0 && negative_error < kDoubleErrorThreshold) {
        num = 1;
        den = (int)(q2d_n + 1);
    }
    else {
        // No change
    }
}

AVRational av_fixed_time_base(const AVRational & time_base)
{
    AVRational new_time_base = time_base;
    if (time_base.den > time_base.num && time_base.num != 0) {
        av_fixed_time_base_impl(new_time_base.num, new_time_base.den);
    }
    else if (time_base.den < time_base.num && time_base.den != 0) {
        av_fixed_time_base_impl(new_time_base.den, new_time_base.num);
    }

    return new_time_base;
}
