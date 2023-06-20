extern "C"
{
#include <libavformat/avformat.h>
}
#include <memory>
#include <iostream>

class Frame;

class AVMediaTool
{
private:
    
public:
    AVMediaTool();
    ~AVMediaTool();
    AVDictionary *filterCodecOption(AVDictionary *opts, AVCodecID codecId, AVFormatContext *fmtContext, AVStream *stream, const AVCodec *codec);
    std::shared_ptr<Frame> getPlaceholderFrameWithTypeAndFrame(AVMediaType type, std::shared_ptr<Frame> frame);
    
};