#include <vector>
#include <queue>
#include <memory>
#include <map>
#include <shared_mutex>
#include <functional>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoState;

enum DecodeStateType
{
    decodeError = -1,
    decodeSuccess,
    decodeFinish,
};

struct mediaOption
{
    std::string mediaUrl = "";
    std::vector<AVMediaType> decodeMediaTypes = {};
};

class streamContext
{
public:
    AVStream *stream;
    AVCodecContext *decContext;
    AVFrame *frame;
    int streamIndex;
    AVMediaType type;
    streamContext();
    ~streamContext();
    int init();
};

using packetCallback = std::function<void(AVMediaType,AVPacket*)>;
using frameCallback = std::function<void(AVMediaType, AVFrame*,AVRational)>;

class Decoder
{
private:
    const mediaOption option;
    std::map<std::string, std::shared_ptr<streamContext>> streamContexts;
    std::map<int, int> streamIndexTypeMap;
    int streamCount;
    AVDictionary **streamOpts;
    AVPacket *packet;
    int openDecContext(std::shared_ptr<streamContext> streamContext, AVMediaType type);
    int decodePacket(std::shared_ptr<streamContext> streamContext);
    int finishDecode();
    std::shared_ptr<streamContext> getCurrentStreamContext();
public:
    AVFormatContext *fmtContext;
    packetCallback outputPacket;
    frameCallback outputFrame;
    Decoder(const mediaOption opt);
    ~Decoder();
    int initMedia();
    DecodeStateType decodeOnce();
    std::shared_ptr<streamContext> getStreamContextWithType(AVMediaType);
};
