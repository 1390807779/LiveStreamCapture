#include <iostream>
#include <map>
#include <memory>
#include <MediaQueue.h>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class Frame;
class PacketQueue;
class Packet;

enum EncodeStateType
{
    encodeError = -1,
    encodeSuccess,
    encodeFinish,
    encodeTest,
};

struct EncodeOption
{
    std::string mediaUrl = "";
    std::string format = "";
};

struct VideoOption
{
    int width = 640;
    int height = 480;
    int bitRate = 400000;
    int gopSize = 10;
    AVRational timeBase = {1, 30};
    AVCodecID codecID = AV_CODEC_ID_H264;
    AVPixelFormat pixelFormat = AV_PIX_FMT_YUV420P16;
    AVRational dataTimeBase = {1, 30}; // input frame time base;
};

struct AudioOption
{
    AVSampleFormat sampleFormat = AV_SAMPLE_FMT_S16;
    int bitRate = 64000;
    int sampleRate = 44100;
    AVRational timeBase = {1, 44100};
    AVChannelLayout channelLayout;
    AVRational dataTimeBase = {1, 44100}; // input frame time base;
};

class OutputStream
{
public:
    AVCodecID codecID;
    AVPacket *packet;
    AVStream *stream;
    AVCodecContext *encContext;
    const AVCodec *codec;
    VideoOption videoOption;
    AudioOption audioOption;
    double count;
    AVRational dataTimeBase;

    OutputStream();
    ~OutputStream();
    int initWithFormatContext(AVCodecID codecId ,AVFormatContext *fmtContext);
    int setVideoOption(VideoOption option);
    int setAudioOption(AudioOption option);
    int initEncContext(AVFormatContext *fmtContext);
    int initEncContextWithDecContext(AVFormatContext *fmtContext, AVCodecContext *decContext, AVStream *stream);
    int openEncCodec(AVDictionary *opt);
};

class Encoder
{
private:
    EncodeOption option;
    AVFormatContext *fmtContext;
    std::map<int, std::shared_ptr<OutputStream>> outStreamMap;
    bool isWritenHeader;
    MediaQueue<Packet> *mediaPacketQueue;
    std::shared_ptr<Packet> standardPkt;          //standard packet while writing
    AVMediaType standardType;               //standard type while writing
    uint32_t startTime;

    std::shared_ptr<Frame> transformImageFrame(std::shared_ptr<Frame> frame);
    std::shared_ptr<Frame> transformAudioFrame(std::shared_ptr<Frame> frame);
    std::shared_ptr<Frame> transformFrame(AVMediaType type, std::shared_ptr<Frame> frame);
    int pushPacketToBuffer(AVMediaType type, AVPacket *packet);
    int writePacketWithQueue(int type);
    int clearPacketBuffer();
public:
    Encoder(EncodeOption _option);
    ~Encoder();
    int init();
    int addStreamWithParam(AVMediaType type, AVCodecContext *decContext, AVStream *stream);
    std::shared_ptr<OutputStream> getOutputStreamWithType(AVMediaType type);
    EncodeStateType encodeOnce(AVMediaType type, std::shared_ptr<Frame> frame);
    int openFileToEncode();
    int finishEncode();
};
