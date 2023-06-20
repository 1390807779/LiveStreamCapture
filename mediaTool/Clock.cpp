#include <Clock.h>
#include <limits>
extern "C"
{
#include <libavutil/time.h>
}

Clock::Clock():
    pts(0),
    updateTime(0),
    ptsDrift(-std::numeric_limits<double>::max())
{
}

Clock::~Clock()
{
}

void Clock::updateClock(double _pts, double _updateTime, double _ptsDrift)
{
    pts = _pts;
    updateTime = _updateTime;
    ptsDrift = _ptsDrift;
}

double Clock::getClock()
{
    return av_gettime_relative() / 1000000.0 + ptsDrift;
}