#include<Frame.h>

Frame::Frame(AVFrame *frame):
    frame(av_frame_clone(frame)),
    pts(0.0),
    isInput(false)
{
}

Frame::~Frame()
{
    if (isInput)
    {
        isInput = true;
    }
    
    if (frame)
    {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
    
}
