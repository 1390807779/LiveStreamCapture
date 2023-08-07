#include <Encoder.h>
#include <memory>
#include <functional>
#include <iostream>
#include <Frame.h>
#include <ImageSwsContext.h>
#include <AudioSwrContext.h>
#include <Packet.h>
extern "C"
{
#include <libavformat/avformat.h>
}

using namespace std;

std::shared_ptr<Frame> Encoder::transformImageFrame(std::shared_ptr<Frame> frame)
{
    std::shared_ptr<OutputStream> outStream = getOutputStreamWithType(AVMEDIA_TYPE_VIDEO);
    AVCodecContext *encContext = outStream ? outStream->encContext : nullptr;
    if (!encContext || !frame->frame->data)
    {
        return nullptr;
    }
    int ret = 0;
    if (frame->frame->width == encContext->width && 
        frame->frame->height == encContext->height &&
        frame->frame->format == static_cast<int>(encContext->pix_fmt))
    {
        return frame;
    }

    AVFrame *newFrame = av_frame_alloc();
    newFrame->format = encContext->pix_fmt;
    newFrame->width = encContext->width;
    newFrame->height = encContext->height;
    av_frame_get_buffer(newFrame, 0);
    int test = av_frame_copy_props(newFrame, frame->frame);
    shared_ptr<ImageSwsContext> imageSwsContext = make_shared<ImageSwsContext>();
    shared_ptr<ImageParam> inImageParam = make_shared<ImageParam>();
    shared_ptr<ImageParam> outImageParam = make_shared<ImageParam>();
    inImageParam->format = frame->frame->format;
    inImageParam->width = frame->frame->width;
    inImageParam->height = frame->frame->height;
    outImageParam->format = encContext->pix_fmt;
    outImageParam->width = encContext->width;
    outImageParam->height = encContext->height;
    imageSwsContext->setFormatImageSwsContextWithTwoParam(inImageParam, outImageParam);
    shared_ptr<Frame> outFrame = make_shared<Frame>(newFrame);
    av_frame_free(&newFrame);
    ret = imageSwsContext->scaleImage(frame->frame->data, frame->frame->linesize, outFrame->frame->data, outFrame->frame->linesize);
    if (ret < 0)
    {
        cout << "scaleImageFailed" << endl;
        return nullptr;
    }
    outFrame->frame->width = encContext->width;
    outFrame->frame->height = encContext->height;
    outFrame->frame->format = static_cast<int>(encContext->pix_fmt);

    return outFrame;
}

std::shared_ptr<Frame> Encoder::transformAudioFrame(std::shared_ptr<Frame> frame)
{
    std::shared_ptr<OutputStream> outStream = getOutputStreamWithType(AVMEDIA_TYPE_AUDIO);
    AVCodecContext *encContext = outStream ? outStream->encContext : nullptr;
    if (!encContext || !frame->frame->data)
    {
        return nullptr;
    }
    int ret = 0;
    if (frame->frame->format == static_cast<int>(encContext->sample_fmt) &&
        !av_channel_layout_compare(&frame->frame->ch_layout, &encContext->ch_layout) &&
        frame->frame->sample_rate == encContext->sample_rate)
    {
        return frame;
    }

    return frame;
    // TODO
    // int nbSample = encContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE ? 10000 : encContext->frame_size;
    // AVFrame *newFrame = av_frame_alloc();
    // newFrame->sample_rate = encContext->sample_rate;
    // newFrame->format = encContext->sample_fmt;
    // newFrame->nb_samples = nbSample;
    // av_channel_layout_copy(&newFrame->ch_layout, &encContext->ch_layout);
    // av_dict_copy(&newFrame->metadata, frame->frame->metadata, 0);
    // av_frame_get_buffer(newFrame, 0);

    // shared_ptr<AudioSwrContext> swrContext = make_shared<AudioSwrContext>();
    // shared_ptr<AudioParam> inParam = make_shared<AudioParam>();
    // shared_ptr<AudioParam> outParam = make_shared<AudioParam>();
    // ret = av_channel_layout_copy(&inParam->channelLayout, &frame->frame->ch_layout);
    // if (ret < 0)
    // {
    //     cout << "error to copy channel layout while encode" << endl;
    //     return nullptr;
    // }
    // ret = av_channel_layout_copy(&outParam->channelLayout, &encContext->ch_layout);
    // if (ret < 0)
    // {
    //     cout << "error to copy channel layout while encode" << endl;
    //     return nullptr;
    // }
    // inParam->format = static_cast<AVSampleFormat>(frame->frame->format);
    // inParam->sampleRate = frame->frame->sample_rate;
    // outParam->format = encContext->sample_fmt;
    // outParam->sampleRate = encContext->sample_rate;
    // ret = swrContext->initWithAudioParam(inParam, outParam);
    // if (ret < 0)
    // {
    //     cout << "swrContext init failed while encode" << endl;
    //     return nullptr;
    // }
    // shared_ptr<Frame> outFrame = make_shared<Frame>(newFrame);
    // ret = swrContext->resampleCustomize(const_cast<const uint8_t **>(frame->frame->extended_data), outFrame->frame->data, nullptr, frame->frame->nb_samples);
    // if (ret < 0)
    // {
    //     return nullptr;
    // }
    
    // av_channel_layout_copy(&outFrame->frame->ch_layout, &encContext->ch_layout);
    // outFrame->frame->sample_rate = encContext->sample_rate;
    // outFrame->frame->format = static_cast<int>(encContext->sample_fmt);
    // outFrame->frame->nb_samples = nbSample;
    // return outFrame;
}

std::shared_ptr<Frame> Encoder::transformFrame(AVMediaType type, std::shared_ptr<Frame> frame)
{
    if (!frame)
    {
        return frame;
    }
    shared_ptr<Frame> outFrame = nullptr;
    switch (type)
    {
    case AVMEDIA_TYPE_AUDIO:
        outFrame = transformAudioFrame(frame);
        break;
    case AVMEDIA_TYPE_VIDEO:
        outFrame = transformImageFrame(frame);
    default:
        break;
    }
    return outFrame;
}

int Encoder::pushPacketToBuffer(AVMediaType type, AVPacket *packet)
{
    int ret = 0;
    MediaQueueType writeQueueType = QueueUnknowType;
    MediaQueueType readQueueType = QueueUnknowType;
    switch (type)
    {
    case AVMEDIA_TYPE_AUDIO:
        writeQueueType = QueueAudioType;
        break;
    case AVMEDIA_TYPE_VIDEO:
        writeQueueType = QueueVideoType;
        break;
    default:
        break;
    }
    if (!standardPkt)
    {
        standardPkt = make_shared<Packet>(packet);
        standardPkt->isOutput = true;
        standardType = type;
    }
    else
    {
        shared_ptr<Packet> packetProxy = make_shared<Packet>(packet);

        //pcaket pts dts is 0
        if (standardPkt->packet->dts == packetProxy->packet->dts)
        {
            ret = av_interleaved_write_frame(fmtContext, standardPkt->packet);
            standardPkt = packetProxy;
            if (ret < 0)
            {
                return ret;
            }
            return ret;
        }
        packetProxy->isOutput = true;
        mediaPacketQueue->pushWithType(writeQueueType, packetProxy);
    }
    switch (standardType)
    {
    case AVMEDIA_TYPE_AUDIO:
        readQueueType = QueueVideoType;
        break;
    case AVMEDIA_TYPE_VIDEO:
        readQueueType = QueueAudioType;
        break;
    default:
        break;
    }
    ret = writePacketWithQueue(readQueueType);
    return ret;
}

int Encoder::writePacketWithQueue(int type)
{
    MediaQueueType queueType = static_cast<MediaQueueType>(type);
    int ret = 0;
    while (mediaPacketQueue->getQueueSizeWithType(queueType) > 0)
    {
        shared_ptr<Packet> writeablePacket = mediaPacketQueue->popWithType(queueType);
        if (standardPkt->packet->dts > writeablePacket->packet->dts)
        {
            ret = av_interleaved_write_frame(fmtContext, writeablePacket->packet);
        }
        else
        {
            ret = av_interleaved_write_frame(fmtContext, standardPkt->packet);
            standardPkt = writeablePacket;
            switch (static_cast<MediaQueueType>(type))
            {
            case QueueVideoType:
                standardType = AVMEDIA_TYPE_VIDEO;
                break;
            case QueueAudioType:
                standardType = AVMEDIA_TYPE_AUDIO;
                break;
            default:
                break;
            }
            break;
        }
    }
    return ret;
}

int Encoder::clearPacketBuffer()
{
    int ret = 0;
    MediaQueueType queueType = QueueUnknowType;
    do
    {
        switch (standardType)
        {
        case AVMEDIA_TYPE_AUDIO:
            queueType = QueueVideoType;
            break;
        case AVMEDIA_TYPE_VIDEO:
            queueType = QueueAudioType;
        default:
            break;
        }
        if (queueType == QueueUnknowType)
        {
            break;
        }
        if (mediaPacketQueue->getQueueSizeWithType(queueType) > 0)
        {
            writePacketWithQueue(queueType);
        }
    } while (mediaPacketQueue->getQueueSizeWithType(QueueAudioType) > 0 && mediaPacketQueue->getQueueSizeWithType(QueueVideoType) > 0);
    
    while (mediaPacketQueue->getQueueSizeWithType(QueueAudioType) > 0)
    {
        ret = av_interleaved_write_frame(fmtContext, standardPkt->packet);
        standardPkt = mediaPacketQueue->popWithType(QueueAudioType);
    }
    
    while (mediaPacketQueue->getQueueSizeWithType(QueueVideoType) > 0)
    {
        ret = av_interleaved_write_frame(fmtContext, standardPkt->packet);
        standardPkt = mediaPacketQueue->popWithType(QueueVideoType);
    }
    ret = av_interleaved_write_frame(fmtContext, standardPkt->packet);

    return ret;
}

Encoder::Encoder(EncodeOption _option):
    option(_option),
    fmtContext(nullptr),
    outStreamMap({}),
    isWritenHeader(false),
    standardPkt(nullptr),
    standardType(AVMEDIA_TYPE_UNKNOWN),
    mediaPacketQueue(new MediaQueue<Packet>()),
    startTime(0.0)
{
}

Encoder::~Encoder()
{
    if (fmtContext)
    {
        avformat_free_context(fmtContext);
    }
    if (mediaPacketQueue)
    {
        delete mediaPacketQueue;
    }
    
}

int Encoder::init()
{
    int ret = 0;
    if (!option.format.empty())
    {
        ret = avformat_alloc_output_context2(&fmtContext, nullptr, option.format.c_str(), option.mediaUrl.c_str());
    }
    else if (!option.mediaUrl.empty())
    {
        ret = avformat_alloc_output_context2(&fmtContext, nullptr, nullptr, option.mediaUrl.c_str());
    }
    else
    {
        ret = avformat_alloc_output_context2(&fmtContext, nullptr, "flv", option.mediaUrl.c_str());
    }
    if (ret < 0)
    {
        cout << "init encoder format context failed" << endl;
        return ret;
    }

    return 0;
}

int Encoder::addStreamWithParam(AVMediaType type, AVCodecContext *decContext, AVStream *stream)
{
    int ret = 0;
    AVCodecID codecId = AV_CODEC_ID_NONE;
    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:
        codecId = decContext ? decContext->codec_id : fmtContext->oformat->video_codec;
        break;
    case AVMEDIA_TYPE_AUDIO:
        codecId = decContext ? decContext->codec_id : fmtContext->oformat->audio_codec;
        break;
    default:
        break;
    }
    if (codecId != AV_CODEC_ID_NONE)
    {
        shared_ptr<OutputStream> outStream = make_shared<OutputStream>();
        ret = outStream->initWithFormatContext(codecId, fmtContext);
        if (ret < 0)
        {
            cout << "outstream init with format context failed while encoding" << endl;
            return ret;
        }
        
        if (decContext)
        {
            ret = outStream->initEncContextWithDecContext(fmtContext, decContext, stream);
        }
        else
        {
            ret = outStream->initEncContext(fmtContext);
        }
        if (ret < 0)
        {
            cout << "init EncContext failed while encoding" << endl;
            return ret;
        }
        AVDictionary* param = nullptr;
        av_dict_set(&param, "preset", "veryfast", 0);
        av_dict_set(&param, "tune", "film", 0);
        ret = outStream->openEncCodec(param);
        if (ret < 0)
        {
            cout << "open EncCodc failed" << endl;
            return ret;
        }
        outStreamMap[static_cast<int>(type)] = outStream;
    }
    return ret;
}

std::shared_ptr<OutputStream> Encoder::getOutputStreamWithType(AVMediaType type)
{
    if (outStreamMap.find(static_cast<int>(type)) == outStreamMap.end())
    {
        return nullptr;
    }
    return outStreamMap[static_cast<int>(type)];
}

EncodeStateType Encoder::encodeOnce(AVMediaType type, std::shared_ptr<Frame> frame)
{
    int ret = 0;
    if (!isWritenHeader)
    {
        ret = avformat_write_header(fmtContext, nullptr);
        startTime = frame->frame->pts;
    }
    if (ret < 0)
    {
        cout << "failed to write the header" << endl;
        return encodeError;
    }
    isWritenHeader = true; 
    shared_ptr<Frame> outFrame = transformFrame(type, frame);
    if (!outFrame && frame)
    {
        cout << "frame transform failed " << endl;
        return encodeError;
    }
    std::shared_ptr<OutputStream> outputStream = getOutputStreamWithType(type);
    if (!outputStream)
    {
        cout << "error type to encode" << endl;
        return encodeError;
    }
    if (!outFrame)
    {
        ret = avcodec_send_frame(outputStream->encContext, nullptr);
    }
    else
    {
        outFrame->isInput = true;
        outFrame->frame->pts = outFrame->frame->pts - startTime;
        outFrame->frame->pts = av_rescale_q(outFrame->frame->pts, outputStream->dataTimeBase, outputStream->encContext->time_base);
        ret = avcodec_send_frame(outputStream->encContext, outFrame->frame);
        av_frame_unref(outFrame->frame);
    }
    if (ret < 0)
    {
        cout << "error sending frame to endcoder" << endl;
        return encodeError;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(outputStream->encContext, outputStream->packet);
        
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            cout << "error to encoding frame" << endl;
            av_packet_unref(outputStream->packet);
            return encodeError;
        }
        av_packet_rescale_ts(outputStream->packet, outputStream->encContext->time_base, outputStream->stream->time_base);
        outputStream->packet->stream_index = outputStream->stream->index;
        ret = pushPacketToBuffer(type, outputStream->packet);
        av_packet_unref(outputStream->packet);
        if (ret < 0)
        {
            cout << "Error while writing output packet" << endl;
            return encodeError;
        }
    }
    av_packet_unref(outputStream->packet);
    return ret == AVERROR_EOF ? encodeFinish : encodeSuccess;
}

int Encoder::openFileToEncode()
{
    int ret = 0;
    if (!(fmtContext->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&fmtContext->pb, option.mediaUrl.c_str(), AVIO_FLAG_WRITE);
    }
    return ret;
}

int Encoder::finishEncode()
{
    encodeOnce(AVMEDIA_TYPE_AUDIO, nullptr);
    encodeOnce(AVMEDIA_TYPE_VIDEO, nullptr);
    clearPacketBuffer();
    int ret = av_write_trailer(fmtContext);
    avio_closep(&fmtContext->pb);
    return ret;
}

OutputStream::OutputStream():
    codecID(AV_CODEC_ID_NONE),
    packet(nullptr),
    stream(nullptr),
    encContext(nullptr),
    codec(nullptr),
    count(0.0)
{
}

OutputStream::~OutputStream()
{
    if (encContext)
    {
        avcodec_free_context(&encContext);
    }
    if (packet)
    {
        av_packet_free(&packet);
    }
    
}

int OutputStream::initWithFormatContext(AVCodecID codecId, AVFormatContext *fmtContext)
{
    codec = avcodec_find_encoder(codecId);
    if (!codec)
    {
        cout << "find encoder failed" << endl;
        return -1;
    }
    packet = av_packet_alloc();
    if (!packet)
    {
        cout << "alloc packet failed" << endl;
        return -1;
    }
    stream = avformat_new_stream(fmtContext, nullptr);
    if (!stream)
    {
        cout << "alloc stream failed" << endl;
        return -1;
    }
    stream->id = fmtContext->nb_streams - 1;
    encContext = avcodec_alloc_context3(codec);
    if (!encContext)
    {
        cout << "alloc encContext failed" << endl;
        return -1;
    }
    encContext->codec_id = codecId;
    return 0;
}

int OutputStream::setVideoOption(VideoOption option)
{
    videoOption = option;
    return 0;
}

int OutputStream::setAudioOption(AudioOption option)
{
    audioOption = option;
    return 0;
}

int OutputStream::initEncContext(AVFormatContext *fmtContext)
{
    if (!encContext)
    {
        cout << "no encContext to init" << endl;
        return -1;
    }
    switch (encContext->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
        encContext->sample_fmt = audioOption.sampleFormat;
        encContext->bit_rate = audioOption.bitRate;
        encContext->sample_rate = audioOption.sampleRate;
        encContext->thread_count = 2;
        stream->time_base = audioOption.timeBase;
        av_channel_layout_copy(&encContext->ch_layout, &audioOption.channelLayout);
        dataTimeBase = audioOption.dataTimeBase;
        break;
    case AVMEDIA_TYPE_VIDEO:
        encContext->bit_rate = videoOption.bitRate > 4000000 ? 4000000 : videoOption.bitRate;
        encContext->width = videoOption.width > 1280 && videoOption.width > videoOption.height ? 1280 : videoOption.width;
        encContext->height = videoOption.height > 720 && videoOption.width > videoOption.height ? 720 : videoOption.height;
        encContext->gop_size = videoOption.gopSize < 60 ? 60 :videoOption.gopSize;
        encContext->pix_fmt = videoOption.pixelFormat;
        encContext->framerate = av_inv_q(videoOption.timeBase);
        encContext->qmin = 10;
        encContext->qmax = 30;
        encContext->thread_count = 8;
        stream->time_base = videoOption.timeBase;
        encContext->time_base = videoOption.timeBase;
        dataTimeBase = videoOption.dataTimeBase;
        if (encContext->codec_id == AV_CODEC_ID_MPEG2VIDEO)
        {
            encContext->max_b_frames = 2;
        }
        if (encContext->codec_id == AV_CODEC_ID_MPEG1VIDEO)
        {
            encContext->mb_decision = 2;
        }
        break;
    default:
        break;
    }
    if (fmtContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
        encContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    return 0;
}

int OutputStream::initEncContextWithDecContext(AVFormatContext *fmtContext, AVCodecContext *decContext, AVStream *stream)
{
    if (decContext)
    {
        switch (decContext->codec_type)
        {
        case AVMEDIA_TYPE_AUDIO:
            audioOption.bitRate = decContext->bit_rate;
            av_channel_layout_copy(&audioOption.channelLayout, &decContext->ch_layout);
            audioOption.sampleFormat = decContext->sample_fmt;
            audioOption.sampleRate = decContext->sample_rate;
            audioOption.timeBase = av_make_q(1, decContext->sample_rate);
            audioOption.dataTimeBase = stream ? stream->time_base : av_make_q(1, decContext->sample_rate);
            break;
        case AVMEDIA_TYPE_VIDEO:
            videoOption.bitRate = decContext->bit_rate;
            videoOption.pixelFormat = decContext->pix_fmt;
            videoOption.timeBase = av_inv_q(decContext->framerate);
            videoOption.width = decContext->width;
            videoOption.height = decContext->height;
            videoOption.gopSize = decContext->gop_size;
            videoOption.dataTimeBase = stream ? stream->time_base : av_inv_q(decContext->framerate);
            break;
        default:
            break;
        }
    }
    
    initEncContext(fmtContext);
    return 0;
}

int OutputStream::openEncCodec(AVDictionary *opt)
{
    int ret = avcodec_open2(encContext, codec, &opt);
    if (ret < 0)
    {
        cout << "could not open Codec With Type: " << av_get_media_type_string(encContext->codec_type) << endl;
        return ret;
    }
    ret = avcodec_parameters_from_context(stream->codecpar, encContext);
    if (ret < 0)
    {
        cout << "could not copy the stream parameters" << endl;
        return ret;
    }
    return 0;
}
