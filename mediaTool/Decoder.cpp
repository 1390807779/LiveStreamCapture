#include<iostream>
extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavutil/samplefmt.h>
}
#include <VideoState.h>
#include <Decoder.h>
#include <AVMediaTool.h>

using namespace std;

streamContext::streamContext():
    stream(nullptr),
    decContext(nullptr),
    streamIndex(0),
    type(AVMEDIA_TYPE_UNKNOWN)
{
}

streamContext::~streamContext()
{
    if (decContext)
    {
        avcodec_free_context(&decContext);
    }
    if (frame)
    {
        av_frame_free(&frame);
    }
}

int streamContext::init()
{
    frame = av_frame_alloc();
    if (!frame)
    {
        cout << "could not allocate frame" << endl;
        return AVERROR(ENOMEM);
    }
    return 0;
}

Decoder::Decoder(const mediaOption opt):
    option(opt),
    fmtContext(nullptr),
    streamContexts({}),
    streamIndexTypeMap({}),
    streamCount(0),
    outputFrame(nullptr),
    outputPacket(nullptr),
    streamOpts(nullptr),
    packet(nullptr)
{
}

Decoder::~Decoder()
{
    if (packet)
    {
        av_packet_free(&packet);
    }
    if (fmtContext)
    {
        avformat_free_context(fmtContext);
    }
    if (streamOpts)
    {
        av_dict_free(streamOpts);
    }
    
}

int Decoder::initMedia()
{
    avformat_network_init();
    packet = av_packet_alloc();
    if (!packet)
    {
        cout << "Could not allocate packet" << endl;
        return -1;
    }
    fmtContext = avformat_alloc_context();
    if (!fmtContext)
    {
        cout << "alloc formatcontext failed" << endl;
        return -1;
    }
    AVDictionary *formatOpts = nullptr;
    if (!av_dict_get(formatOpts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE)) {
        av_dict_set(&formatOpts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    }
    int ret = avformat_open_input(&fmtContext, option.mediaUrl.c_str(), nullptr, nullptr);
    av_dict_set(&formatOpts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
    if (ret < 0)
    {
        cout << "Could not init FormatContext" << endl;
        return -1;
    }
    av_format_inject_global_side_data(fmtContext);
    ret = avformat_find_stream_info(fmtContext, streamOpts);
    if (ret < 0)
    {
        cout << "Could not find stream information" << endl;
        return -1;
    }
    for (auto &&mediaType : option.decodeMediaTypes)
    {
        string typeString = av_get_media_type_string(mediaType);
        shared_ptr<streamContext> typeStreamContext = make_shared<streamContext>();
        ret = typeStreamContext->init();
        if (ret < 0)
        {
            cout << "Could not init streamContext with Type: " << typeString << endl;
            return ret;
        }
        
        ret = openDecContext(typeStreamContext, mediaType);
        if (ret < 0)
        {
            cout << "Failed to open codec context with type: " << typeString << endl;
            return -1;
        }
        if (typeStreamContext->stream)
        {
            streamCount++;
        }
        streamContexts[typeString] = typeStreamContext;
        streamIndexTypeMap[typeStreamContext->streamIndex] = static_cast<AVMediaType>(mediaType);
    }
    if (streamCount <= 0)
    {
        cout << "Could not find any stream in the input " << endl;
        return -1;
    }
    return 0;
}
int Decoder::openDecContext(shared_ptr<streamContext> streamContext, AVMediaType type)
{
    string typeString = string(av_get_media_type_string(type));
    int ret = av_find_best_stream(fmtContext, type, -1, -1, nullptr, 0);
    if (ret < 0)
    {
        cout << "Could not find stream with type: " << typeString << endl;
        return -1;
    }
    streamContext->type = type;
    streamContext->streamIndex = ret;
    streamContext->stream = fmtContext->streams[ret];
    streamContext->stream->discard = AVDISCARD_DEFAULT;

    auto streamDecoder = avcodec_find_decoder(streamContext->stream->codecpar->codec_id);
    if (!streamDecoder)
    {
        cout << "Failed to find codec with type: " << typeString << endl;
        return AVERROR(EINVAL);
    }
    streamContext->decContext = avcodec_alloc_context3(streamDecoder);
    if (!streamContext->decContext)
    {
        cout << "Failed to allocate the codec context with type: " << typeString << endl;
        return AVERROR(ENOMEM);
    }
    ret = avcodec_parameters_to_context(streamContext->decContext, streamContext->stream->codecpar);
    if (ret <0)
    {
        cout << "Failed to copy codec context parameters to decoder context with type: " << typeString << endl;
        return ret;
    }
    shared_ptr<AVMediaTool> meidaTool = make_shared<AVMediaTool>();
    AVDictionary *opts = nullptr;
    meidaTool->filterCodecOption(opts, streamContext->decContext->codec_id, fmtContext, streamContext->stream, streamDecoder);
    if (opts)
    {
        if (!av_dict_get(opts, "threads", nullptr, 0))
        {
            av_dict_set(&opts, "threads", "auto", 0);
        }
        av_dict_set(&opts, "lowres", add_const_t<const char *>(&streamDecoder->max_lowres), 0);
        
    }
    ret = avcodec_open2(streamContext->decContext, streamDecoder, &opts);
    if (ret < 0)
    {
        cout << "Failed to open codec with type: " << typeString << endl;
        return ret;
    }
    av_dict_free(&opts);
    return 0;
}

DecodeStateType Decoder::decodeOnce()
{
    int ret = av_read_frame(fmtContext, packet);
    if (ret < 0)
    {
        cout << "Decoder read frame Finish" << endl;
        finishDecode();
        return decodeFinish;
    }
    shared_ptr<streamContext> typeStreamContext = getCurrentStreamContext();
    if (!typeStreamContext)
    {
        cout << "Decoder decode error without current type" << endl;
        return decodeError;
    }
    if (outputPacket)
    {
        outputPacket(typeStreamContext->type, packet);
    }
    decodePacket(typeStreamContext);
    return decodeSuccess;
}

int Decoder::decodePacket(shared_ptr<streamContext> streamContext)
{   
    //解码
    int ret = avcodec_send_packet(streamContext->decContext, packet);
    if (ret != AVERROR(EAGAIN) && packet)
    {
        av_packet_unref(packet);
    }
    ret = avcodec_receive_frame(streamContext->decContext, streamContext->frame);
    if (ret < 0)
    {
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        {
            return -1;
        }
        return ret;
    };
    
    if (outputFrame)
    {
        outputFrame(streamContext->type, streamContext->frame, streamContext->stream->time_base);
    }
    av_frame_unref(streamContext->frame);
    return 0;
}

int Decoder::finishDecode()
{
    shared_ptr<streamContext> streamContext = nullptr;
    for (auto &&streamContextMap : streamContexts)
    {
        streamContext = streamContextMap.second;
        packet = nullptr;
        int ret = decodePacket(streamContext);
    }
    return 0;
}

std::shared_ptr<streamContext> Decoder::getCurrentStreamContext()
{
    if (streamIndexTypeMap.empty())
    {
        cout << "without open StreamContext" << endl;
    }
    AVMediaType type = streamIndexTypeMap.find(packet->stream_index) != streamIndexTypeMap.end() ? static_cast<AVMediaType>(streamIndexTypeMap[packet->stream_index]) : AVMEDIA_TYPE_VIDEO;
    shared_ptr<streamContext> typeStreamContext = getStreamContextWithType(type);
    return typeStreamContext;
}

shared_ptr<streamContext> Decoder::getStreamContextWithType(AVMediaType type)
{
    return streamContexts.find(av_get_media_type_string(type)) != streamContexts.end() ? streamContexts[av_get_media_type_string(type)] : nullptr;
}