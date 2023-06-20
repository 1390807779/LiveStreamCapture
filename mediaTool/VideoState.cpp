#include <VideoState.h>
#include <Frame.h>
#include <Decoder.h>
#include <Clock.h>
#include <AudioSwrContext.h>
#include <utility>
#include <Encoder.h>
#include <thread> 
#include <AVMediaTool.h>
extern "C"
{
#include <libavutil/time.h>
#include <libavutil/log.h>
}
using namespace std;

// could open when debug
// ------------------------------

// shared_mutex fileMutex;

// void FFmpegLogFunc(void* ptr, int level, const char* fmt, va_list vl) {
//     unique_lock<shared_mutex> lock(fileMutex);
//     FILE *fp = fopen("my_log.txt","a+");     
//     if(fp){     
//         vfprintf(fp,fmt,vl);  
//         fflush(fp);  
//         fclose(fp);  
//     }     
// }

VideoState::VideoState(string url, int videoPacketMaxSize, int audioPacketMaxSize):
    mediaUrl(url),
    decoder(nullptr),
    audioClock(0),
    videoClock(0),
    skipTime(1000.0),
    minAudioFrame(audioPacketMaxSize),
    minVideoFrame(videoPacketMaxSize),
    isDecodeFinish({false}),
    isEncodeing({false}),
    isMute({false}),
    decodeFrameQueue(new MediaQueue<Frame>()),
    encodeFrameQueue(new MediaQueue<Frame>())
{
    // av_log_set_callback(FFmpegLogFunc);
}

VideoState::~VideoState()
{
    videoInfo.currentFrame = nullptr;
    videoInfo.videoClock = nullptr;
    audioInfo.audioParam = nullptr;
    audioInfo.currentFrame = nullptr;
    audioInfo.audioClock = nullptr;
    if (decodeFrameQueue)
    {
        delete decodeFrameQueue;
    }
    if (encodeFrameQueue)
    {
        delete encodeFrameQueue;
    }
    
}

int VideoState::initDecoderWithUrl()
{
    mediaOption option;
    option.decodeMediaTypes = {AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO};
    option.mediaUrl = mediaUrl;
    decoder = make_shared<Decoder>(option);
    int ret = decoder->initMedia();
    if (ret < 0)
    {
        cout << "VideoState Decoder init Failed" << endl;
        return ret;
    }
    decoder->outputPacket = bind(&VideoState::packetReceive, this, placeholders::_1, placeholders::_2);
    decoder->outputFrame = bind(&VideoState::frameReceive, this, placeholders::_1, placeholders::_2, placeholders::_3);
    videoInfo.maxFrameTimer = decoder->fmtContext->iformat->flags & AVFMT_TS_DISCONT ? 10.0 : 3600.0;
    ret = initAudioAndVideoInfo();
    if (ret < 0)
    {
        cout << "init media info fail " << endl;
        return ret;
    }
    return 0;
}

int VideoState::initAudioAndVideoInfo()
{
    audioInfo.audioParam = make_shared<AudioParam>();
    shared_ptr<streamContext> streamContext = decoder->getStreamContextWithType(AVMEDIA_TYPE_AUDIO);
    audioInfo.audioParam->format = AV_SAMPLE_FMT_S16;
    audioInfo.audioParam->sampleRate = streamContext->decContext->sample_rate;
    audioInfo.audioClock = make_shared<Clock>();
    int ret = 0;
    ret = av_channel_layout_copy(&audioInfo.audioParam->channelLayout, &streamContext->decContext->ch_layout);
    if (ret < 0)
    {
        cout << "init channnel layout fail" << endl;
        return ret;
    }
    audioInfo.audioParam->nChannel = streamContext->decContext->ch_layout.nb_channels;
    streamContext = decoder->getStreamContextWithType(AVMEDIA_TYPE_VIDEO);
    videoInfo.width = streamContext->decContext->width;
    videoInfo.height = streamContext->decContext->height;
    videoInfo.frameRate = streamContext->stream->avg_frame_rate;
    videoInfo.videoClock = make_shared<Clock>();
    return ret;
}

DecodeStatus VideoState::startDecode()
{
    if (decodeFrameQueue->getQueueSizeWithType(QueueAudioType) < minAudioFrame || 
        decodeFrameQueue->getQueueSizeWithType(QueueVideoType) < minVideoFrame)
    {
        DecodeStateType ret = decoder->decodeOnce();
        if (ret == decodeFinish)
        {
            isDecodeFinish.store(true);
            return decodeStatusFinish;
        }
        if (ret < 0)
        {
            cout << "VideoState decode Failed " << endl;
            return decodeStatusError;
        }
    }
    else
    {
        cout <<  "audio and video queue is unwriteable" << endl;
        return decodeStatusBufferFull;
    }
    return decodeStatusSuccess;
}

int VideoState::startEncodeTo(std::string url)
{
    EncodeOption option;
    option.mediaUrl = url;
    option.format = url.substr(url.find_last_of('.') + 1);
    std::shared_ptr<Encoder> encoder = make_shared<Encoder>(option);
    
    int ret = 0;
    ret = encoder->init();
    if (eallback)
    {
        eallback("encoder init with ret url" + to_string(ret) + " " + url);
    }
    if (ret < 0)
    {
        cout << "encoder init failed" << endl;
        return -1;
    }
    if (isDecodeFinish.load())
    {
        return -1;
    }
    vector<AVMediaType> types = {AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO};
    for (auto &&type : types)
    {
        AVCodecContext *decContext = nullptr;
        shared_ptr<streamContext> streamContext = decoder->getStreamContextWithType(type);
        if (!streamContext)
        {
            cout << "get streamContext failed" << endl;
            return -1;
        }
        decContext = streamContext->decContext;
        ret = encoder->addStreamWithParam(type, decContext, streamContext->stream);
        if (eallback)
        {
            eallback("add stream with ret" + to_string(ret));
        }
        if (ret < 0)
        {
            cout << "encoder add stream failed" << endl;
            return -1;
        }
    }
    ret = encoder->openFileToEncode();
    if (ret < 0)
    {
        cout << "open file to write failed" << endl;
        return ret;
    }
    isEncodeing.store(true);
    double startTime  = -1.0;
    if (eallback)
    {
        eallback("encode ing ............ ");
    }
    do
    {
        if (encodeFrameQueue->getQueueSizeWithType(QueueAudioType) < 1 || encodeFrameQueue->getQueueSizeWithType(QueueVideoType) < 1)
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }
        shared_ptr<Frame> videoFrame = encodeFrameQueue->getTopWithType(QueueVideoType);
        shared_ptr<Frame> audioFrame = encodeFrameQueue->getTopWithType(QueueAudioType);
        AVMediaType encodeType = AVMEDIA_TYPE_UNKNOWN;
        shared_ptr<Frame> encodeFrame = nullptr;
        MediaQueueType queueType = QueueUnknowType;
        if (videoFrame->pts <= audioFrame->pts)
        {
            encodeType = AVMEDIA_TYPE_VIDEO;
            encodeFrame = videoFrame;
            queueType = QueueVideoType;
        }
        else
        {
            encodeType = AVMEDIA_TYPE_AUDIO;
            encodeFrame = audioFrame;
            queueType = QueueAudioType;
        }
        EncodeStateType type = encoder->encodeOnce(encodeType, encodeFrame);
        encodeFrameQueue->popWithType(queueType);
    } while (isEncodeing.load());
    while (encodeFrameQueue->getQueueSizeWithType(QueueAudioType) > 0)
    {
        encoder->encodeOnce(AVMEDIA_TYPE_AUDIO, encodeFrameQueue->popWithType(QueueAudioType));
    }
    while (encodeFrameQueue->getQueueSizeWithType(QueueVideoType) > 0)
    {
        encoder->encodeOnce(AVMEDIA_TYPE_VIDEO, encodeFrameQueue->popWithType(QueueVideoType));
    }
    ret = encoder->finishEncode();
    if (eallback)
    {
        eallback("encode finish ");
    }
    encodeFrameQueue->updateQueue(QueueAudioType, queue<shared_ptr<Frame>>());
    encodeFrameQueue->updateQueue(QueueVideoType, queue<shared_ptr<Frame>>());
    return 0;
}

void VideoState::cancelEncode()
{
    isEncodeing .store(false);
}

void VideoState::changeAudioOutputWithIsMute()
{
    isMute.store(!isMute.load());
}

void VideoState::packetReceive(AVMediaType type, AVPacket *packet)
{
    switch (type)
    {
    case AVMEDIA_TYPE_AUDIO:
        audioPacketReceive(packet);
        break;
    case AVMEDIA_TYPE_VIDEO:
        videoPacketReceive(packet);
        break;
    default:
        break;
    }
    return;
}

shared_ptr<AudioParamOutput> VideoState::getAudioParamOutput()
{
    return make_shared<AudioParamOutput>(audioInfo.audioParam);
}

void VideoState::frameReceive(AVMediaType type, AVFrame *frame, AVRational timeBase)
{
    switch (type)
    {
    case AVMEDIA_TYPE_AUDIO:
        audioFrameReceive(frame, timeBase);
        break;
    case AVMEDIA_TYPE_VIDEO:
        videoFrameReceive(frame, timeBase);
        break;
    default:
        break;
    }
}

void VideoState::audioFrameReceive(AVFrame *frame, AVRational timeBase)
{
    unique_lock<shared_mutex> lock(mutex);
    shared_ptr<Frame> frameProxy = make_shared<Frame>(frame);
    frameProxy->pts = frame->pts * av_q2d(timeBase);
    decodeFrameQueue->pushWithType(QueueAudioType, frameProxy);
    
}

void VideoState::audioPacketReceive(AVPacket *packet)
{
}

void VideoState::videoFrameReceive(AVFrame *frame, AVRational timeBase)
{
    unique_lock<shared_mutex> lock(mutex);
    frame->pts = frame->best_effort_timestamp;
    shared_ptr<Frame> frameProxy = make_shared<Frame>(frame);
    frameProxy->sampleAspectRatio = av_guess_sample_aspect_ratio(decoder->fmtContext, decoder->getStreamContextWithType(AVMEDIA_TYPE_VIDEO)->stream, frame);
    frameProxy->pts = frame->pts * av_q2d(timeBase);
    decodeFrameQueue->pushWithType(QueueVideoType, frameProxy);
}

void VideoState::videoPacketReceive(AVPacket *packet)
{
}

std::shared_ptr<VideoDataOutput> VideoState::getVideoData(double *recallTime)
{
    double currentTime = av_gettime_relative() / 1000000.0;
    std::shared_ptr<VideoDataOutput> videoOutput = nullptr;
    if (!videoInfo.currentFrame)
    {
        videoInfo.currentFrame = decodeFrameQueue->popWithType(QueueVideoType);
        if (videoInfo.currentFrame)
        {
            // init frame timer
            videoInfo.frameTimer = currentTime;
            videoOutput = make_shared<VideoDataOutput>(videoInfo.currentFrame);
        }
        return videoOutput;
    }
    shared_ptr<Frame> nextFrame = decodeFrameQueue->getTopWithType(QueueVideoType);
    while (nextFrame)
    {
        VideoFrameCheckStatus checkStatus = checkVideoFramePTS(currentTime, videoInfo.currentFrame, nextFrame, recallTime);
        // before encode, for data copy to VideoDataPutout
        videoOutput = make_shared<VideoDataOutput>(nextFrame);
        if (checkStatus == VideoFrameShow)
        {
            videoInfo.currentFrame = decodeFrameQueue->popWithType(QueueVideoType);// equal to currentFrame = nextFrame
            if (isEncodeing.load() && nextFrame)
            {
                encodeFrameQueue->pushWithType(QueueVideoType, nextFrame);
            }
        }
        if (checkStatus != VideoFrameDiscard)
        {
            break;
        }
        decodeFrameQueue->popWithType(QueueVideoType);
        if (isEncodeing.load() && nextFrame)
        {
            encodeFrameQueue->pushWithType(QueueVideoType, nextFrame);
        }
        // set the value after push old frame to encode queue
        nextFrame = decodeFrameQueue->getTopWithType(QueueVideoType);
    }
    if (!nextFrame)
    {
        return nullptr;
    }
    // if (videoInfo.currentFrame)
    // {
    //     videoOutput = make_shared<VideoDataOutput>(videoInfo.currentFrame);
    // }
    return videoOutput;
}

std::shared_ptr<AudioDataOutput> VideoState::getAudioData(uint32_t queueAudioSize)
{
    shared_ptr<Frame> nextFrame = decodeFrameQueue->getTopWithType(QueueAudioType);
    /*caclulate current frame with next frame*/
    shared_ptr<Frame> currentFrame = decodeFrameQueue->popWithType(QueueAudioType);
    audioInfo.currentFrame = nextFrame ? currentFrame : nullptr;
    /*update audio clock*/
    std::shared_ptr<AudioDataOutput> audioOutput = audioInfo.currentFrame ? make_shared<AudioDataOutput>(audioInfo.currentFrame) : nullptr;
    if (audioOutput && currentFrame)
    {
        // use Frame before encode, for data to use
        shared_ptr<Frame> resampleFrame = currentFrame;
        if (isMute.load())
        {
            shared_ptr<AVMediaTool> mediaTool = make_shared<AVMediaTool>();
            shared_ptr<Frame> muteFrame = mediaTool->getPlaceholderFrameWithTypeAndFrame(AVMEDIA_TYPE_AUDIO, currentFrame);
            resampleFrame = muteFrame;
        }

        int ret = getAudioSampleData(audioOutput, resampleFrame);
        audioClock = currentFrame->pts + static_cast<double>(currentFrame->frame->nb_samples / currentFrame->frame->sample_rate);
        
        updateAudioClock(queueAudioSize);
        if (isEncodeing.load())
        {
            encodeFrameQueue->pushWithType(QueueAudioType, currentFrame);
        }
        if (ret < 0)
        {
            cout << "resample audio failed" << endl;
            return nullptr;
        }
    }

    return audioOutput;
}

int VideoState::getAudioSampleData(shared_ptr<AudioDataOutput> audioData, shared_ptr<Frame> currentFrame)
{
    shared_ptr<AudioParam> frameParam = make_shared<AudioParam>();
    av_channel_layout_copy(&frameParam->channelLayout, &currentFrame->frame->ch_layout);
    frameParam->nChannel = currentFrame->frame->ch_layout.nb_channels;
    frameParam->sampleRate = currentFrame->frame->sample_rate;
    frameParam->format = static_cast<AVSampleFormat>(currentFrame->frame->format);
    if (audioInfo.audioParam->format != frameParam->format ||
        audioInfo.audioParam->nChannel != frameParam->nChannel ||
        av_channel_layout_compare(&audioInfo.audioParam->channelLayout, &frameParam->channelLayout) ||
        audioInfo.audioParam->sampleRate != frameParam->sampleRate)
    {
        shared_ptr<AudioSwrContext> swrContext = make_shared<AudioSwrContext>();
        int ret = swrContext->initWithAudioParam(frameParam, audioInfo.audioParam);
        if (ret < 0)
        {
            cout << "swrContext init Failed" << endl;
            return ret;
        }
        const uint8_t **frameData = const_cast<const uint8_t **>(currentFrame->frame->extended_data);
        shared_ptr<ResampleData> resampleData = make_shared<ResampleData>();
        ret = resampleData->initDataWithParam(frameParam, currentFrame->frame->nb_samples);
        if (ret < 0)
        {
            cout << "init resample data failed" << endl;
            return ret;
        }
        ret = swrContext->resample(frameData, resampleData);
        if (ret < 0)
        {
            cout << "swrContext resample Failed" << endl;
            return ret;
        }
        audioData->setResampleData(resampleData);
    }
    return 0;
}

VideoFrameCheckStatus VideoState::checkVideoFramePTS(double currentTime, std::shared_ptr<Frame> frame, std::shared_ptr<Frame> nextFrame, double *recalltime)
{
    double lastDuration = 1.0 / ceil(getVideoFrameRate());
    if (nextFrame)
    {
        double tempDuration = nextFrame->pts - frame->pts;
        if (tempDuration > 0 && tempDuration < videoInfo.maxFrameTimer)
        {
            lastDuration = tempDuration;
        }
    }
    if (lastDuration > skipTime && currentTime > videoInfo.frameTimer + lastDuration)
    {
        // 当前时间超出下一帧播放时间，舍弃下一帧
        cout << "video Frame should discard" << endl;
        return VideoFrameDiscard;
    }
    //音频线程与视频线程真实时间差距 
    double clockDiff = videoInfo.videoClock->getClock() - audioInfo.audioClock->getClock();
    double syscDelay = FFMAX(syncThresholdMin, FFMIN(lastDuration, syncThresholdMax));
    //同步音频需delay时间
    double delay = lastDuration;
    if (fabs(clockDiff) < videoInfo.maxFrameTimer)
    {
        if (clockDiff <= -syscDelay)
        {
            delay = FFMAX(0, delay + clockDiff);
        }
        else if (clockDiff >= syscDelay && lastDuration > syncFrameDurationThreshold)
        {
            delay  = delay + clockDiff;
        }
        else if (clockDiff >= syscDelay)
        {
            delay = 2 * delay;
        }
    }
    //更新刷新时间
    if (currentTime < videoInfo.frameTimer + delay)
    {
        *recalltime = FFMIN(videoInfo.frameTimer + delay - currentTime, *recalltime);
        return VideoFrameDelay;
    }
    //更新Video时间
    videoInfo.frameTimer += delay;
    updateVideoClock(nextFrame->pts);
    return VideoFrameShow;
}


double VideoState::getVideoFrameRate()
{
    return av_q2d(videoInfo.frameRate);
}

int VideoState::updateHardWareParam(shared_ptr<AudioParamOutput> hardWareParam)
{
    hardWareParam->copyToParam(audioInfo.audioParam);
    audioInfo.audioParam->audioBytesPerSecond = av_samples_get_buffer_size(nullptr, audioInfo.audioParam->channelLayout.nb_channels, audioInfo.audioParam->sampleRate, audioInfo.audioParam->format, 1);
    audioInfo.audioParam->frameSize = av_samples_get_buffer_size(nullptr, audioInfo.audioParam->channelLayout.nb_channels, 1, audioInfo.audioParam->format, 1);
    if (audioInfo.audioParam->audioBytesPerSecond <= 0 || audioInfo.audioParam->frameSize <= 0)
    {
        cout << "av_samples_get_buffer_size failed" << endl;
        return -1;
    }
    return 0;
}

void VideoState::updateAudioClock(int audioWrittenSize)
{
    double pts = audioClock - static_cast<double>(audioWrittenSize) / audioInfo.audioParam->audioBytesPerSecond;
    int64_t time = av_gettime_relative();
    double updatedTime = time / 1000000.0;
    audioInfo.audioClock->updateClock(pts, updatedTime, pts - updatedTime);
    cout << "update audio pts: " << pts << endl;
}

void VideoState::updateVideoClock(double pts)
{
    int64_t time = av_gettime_relative();
    double updatedTime = time / 1000000.0;
    videoInfo.videoClock->updateClock(pts, updatedTime, pts - updatedTime);
    cout << "update video pts: " << pts << endl;
}