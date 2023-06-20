extern "C"
{
#include<libavformat/avformat.h>
}

class Frame
{
private:

public:
    AVFrame *frame = nullptr;
    double pts;
    AVRational sampleAspectRatio;
    bool isInput;
    Frame(AVFrame *frame);
    ~Frame();
};
