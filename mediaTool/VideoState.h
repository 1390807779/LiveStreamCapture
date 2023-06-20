#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <MediaQueue.h>
#include <MediaDataOutput.h>
#include <atomic>
#include <functional>
extern "C"
{
#include<libavformat/avformat.h>
}

const double syncThresholdMin = 0.04;
const double syncThresholdMax = 0.1;
const double syncFrameDurationThreshold = 0.1;

// class VideoDataOutput;
// class AudioDataOutput;
class Frame;
class Clock;
class AudioParam;
class Decoder;
struct AVRational;
class AudioParamOutput;


enum DecodeStatus
{
    decodeStatusError = -1,
    decodeStatusSuccess,
    decodeStatusFinish,
    decodeStatusBufferFull,
};

enum VideoFrameCheckStatus
{
    VideoFrameShow = 0,
    VideoFrameDiscard,
    VideoFrameDelay,
};

struct AudioInfo
{
    int64_t nextPts;
    AVRational nextTimeBase;
    std::shared_ptr<AudioParam> audioParam = nullptr;
    std::shared_ptr<Frame> currentFrame = nullptr;
    std::shared_ptr<Clock> audioClock = nullptr;
};
struct VideoInfo
{
    int width;
    int height;
    double frameTimer;
    double maxFrameTimer;
    AVRational frameRate;
    std::shared_ptr<Frame> currentFrame = nullptr;
    std::shared_ptr<Clock> videoClock = nullptr;
};

using errorCallback = std::function<std::string(std::string)>;

class VideoState: public std::enable_shared_from_this<VideoState>
{
private:
    std::string mediaUrl;
    std::shared_ptr<Decoder> decoder;
    MediaQueue<Frame> *decodeFrameQueue;
    MediaQueue<Frame> *encodeFrameQueue;
    double audioClock;
    double videoClock;
    int minAudioFrame;
    int minVideoFrame;
    double skipTime;
    AudioInfo audioInfo;
    VideoInfo videoInfo;
    std::shared_mutex mutex;
    std::shared_mutex encodeMutex;
    std::atomic<bool> isDecodeFinish;
    volatile std::atomic<bool> isEncodeing;
    std::atomic<bool> isMute;
    
    void audioFrameReceive(AVFrame *frame, AVRational timeBase);
    void audioPacketReceive(AVPacket *packet);
    void videoFrameReceive(AVFrame *frame, AVRational timeBase);
    void videoPacketReceive(AVPacket *packet);
    void frameReceive(AVMediaType type, AVFrame *frame, AVRational timeBase);
    void packetReceive(AVMediaType type, AVPacket *packet);
    int initAudioAndVideoInfo();
    int getAudioSampleData(std::shared_ptr<AudioDataOutput> audioData, std::shared_ptr<Frame> currentFrame);
    VideoFrameCheckStatus checkVideoFramePTS(double currentTime, std::shared_ptr<Frame> frame, std::shared_ptr<Frame> nextFrame, double *recalltime);
public:
    errorCallback eallback;

    VideoState(std::string url, int videoPacketBufferMinSize, int audioPacketBufferMinSize);
    ~VideoState();
    int initDecoderWithUrl();
    DecodeStatus startDecode();
    int startEncodeTo(std::string url);
    void cancelEncode();
    void changeAudioOutputWithIsMute();
    std::shared_ptr<AudioParamOutput> getAudioParamOutput();
    std::shared_ptr<VideoDataOutput> getVideoData(double *recallTime);
    std::shared_ptr<AudioDataOutput> getAudioData(uint32_t queueAudioSize);
    double getVideoFrameRate();
    int updateHardWareParam(std::shared_ptr<AudioParamOutput> hardWareParam);
    void updateAudioClock(int audioWrittenSize);
    void updateVideoClock(double pts);
};