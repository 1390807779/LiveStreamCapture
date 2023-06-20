extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}
#include <MathFunction.h>

MathFunction::MathFunction()
{
}

MathFunction::~MathFunction()
{
}

int MathFunction::AVLog(unsigned int num)
{
    return av_log2(num);
}

int MathFunction::AVCeilRightShift(int ceil, int step)
{
    return AV_CEIL_RSHIFT(ceil, step);
}
