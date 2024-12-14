
#include "utils.h"

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
