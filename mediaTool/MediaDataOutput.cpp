#include <MediaDataOutput.h>
#include <Frame.h>
#include <AudioSwrContext.h>

using namespace std;

AudioDataOutput::AudioDataOutput(shared_ptr<Frame> _frame):
    resampleData(nullptr),
    size(0)
{
    for (auto &&data : datas)
    {
        data = nullptr;
    }
    
    setFrame(_frame);
}

AudioDataOutput::~AudioDataOutput()
{
    for (auto &&data : datas)
    {
        if (!datas)
        {
            continue;
        }
        delete[] data;
    }
    
}

uint8_t *AudioDataOutput::getData()
{
    if (resampleData)
    {
        return resampleData->getData();
    }
    if (isNull())
    {
        return nullptr;
    }
    return datas[0];
}

int AudioDataOutput::getSize()
{
    if (resampleData)
    {
        return resampleData->getSize();
    }
    if (!isNull())
    {
        return av_samples_get_buffer_size(nullptr, nbChannels, nbSamples, static_cast<AVSampleFormat>(format), 1);
    }
    return size;
}

void AudioDataOutput::setFrame(std::shared_ptr<Frame> _frame)
{
    for (size_t i = 0; i < 8; i++)
    {
        if (!_frame->frame->data[i])
        {
            continue;
        }

        size_t len = av_get_bytes_per_sample(static_cast<AVSampleFormat>(_frame->frame->format)) * _frame->frame->nb_samples;
        
        uint8_t *data = new uint8_t[len];
        datas[i] = data;
        memcpy(datas[i], _frame->frame->data[i], len);
    }
    
    nbSamples = _frame->frame->nb_samples;
    nbChannels = _frame->frame->ch_layout.nb_channels;
    format = _frame->frame->format;
}

void AudioDataOutput::setResampleData(shared_ptr<ResampleData> _resampleData)
{
    resampleData = _resampleData;
}

bool AudioDataOutput::isNull()
{
    if (!datas[0])
    {
        return true;
    }
    return false;
}

VideoDataOutput::VideoDataOutput(std::shared_ptr<Frame> _frame)
{

    for (size_t i = 0; i < 8; i++)
    {
        datas[i] = nullptr;
    }
    for (size_t i = 0; i < 8; i++)
    {
        lineSize[i] = 0;
    }
    
    setFrame(_frame);
}

VideoDataOutput::~VideoDataOutput()
{
    for (auto &&data : datas)
    {
        if (!datas)
        {
            continue;
        }
        delete[] data;
    }
}

uint8_t *VideoDataOutput::getDataWithIndex(int index)
{
    if (isNull())
    {
        return nullptr;
    }
    return (index < 8 ? datas[index] : nullptr);
}

uint8_t ** VideoDataOutput::getData()
{
    if (isNull())
    {
        return nullptr;
    }
    return datas;
}

int VideoDataOutput::getLinesizeWithIndex(int index)
{
    if (isNull())
    {
        return 0;
    }
    
    return index < 8 ? lineSize[index] : -1;
}

int *VideoDataOutput::getLineSize()
{
    if (isNull())
    {
        return 0;
    }
    return lineSize;
}

int VideoDataOutput::getDataFormat()
{
    if (isNull())
    {
        return -1;
    }
    
    return format;
}

int VideoDataOutput::getWidth()
{
    if (isNull())
    {
        return 0;
    }
    
    return width;
}

int VideoDataOutput::getHeight()
{
    if (isNull())
    {
        return 0;
    }
    
    return height;
}

void VideoDataOutput::setFrame(std::shared_ptr<Frame> _frame)
{
    for (size_t i = 0; i < 8; i++)
    {
        if (!_frame->frame->data[i] || _frame->frame->linesize[i] < 1 || _frame->frame->linesize[0] < 1)
        {
            continue;
        }
        
        size_t len = _frame->frame->linesize[i] * _frame->frame->height / _frame->frame->linesize[0] * _frame->frame->linesize[i];
        
        uint8_t *data = new uint8_t[len];
        datas[i] = data;
        memcpy(datas[i], _frame->frame->data[i], len);
    }
    for (size_t i = 0; i < 8; i++)
    {
        lineSize[i] = _frame->frame->linesize[i];
    }
    width = _frame->frame->width;
    height = _frame->frame->height;
    format = _frame->frame->format;
    sampleAspectRatioDen = _frame->frame->sample_aspect_ratio.den;
    sampleAspectRatioNum = _frame->frame->sample_aspect_ratio.num;
}

void VideoDataOutput::resize(int *dstWidth, int *dstHeight)
{
    int srcWidth = *dstWidth;
    int srcHeight = *dstHeight;
    AVRational sampleAspectRatio = {sampleAspectRatioNum, sampleAspectRatioDen};
    if (av_cmp_q(sampleAspectRatio, av_make_q(0, 1)) <= 0)
    {
        sampleAspectRatio = av_make_q(1,1);
    }
    AVRational aspectRatio = av_mul_q(sampleAspectRatio, av_make_q(width, height));
    *dstWidth = av_rescale(srcHeight, aspectRatio.num, aspectRatio.den);
    if (*dstWidth > srcWidth)
    {
        *dstWidth = srcWidth;
        *dstHeight = av_rescale(srcWidth,aspectRatio.den, aspectRatio.num);
    }
}

bool VideoDataOutput::isFlip()
{
    return lineSize[0] < 0;
}

bool VideoDataOutput::isNull()
{
    if (!datas[0])
    {
        return true;
    }
    return false;
}

AudioParamOutput::AudioParamOutput(shared_ptr<AudioParam> _audioParam):
    audioParam(make_shared<AudioParam>())
{
    *audioParam.get() = *_audioParam.get();
}

AudioParamOutput::~AudioParamOutput()
{
}

int AudioParamOutput::getAudioChannels()
{
    return audioParam->channelLayout.nb_channels;
}

int AudioParamOutput::getAudioSampleRate()
{
    return audioParam->sampleRate;
}

int AudioParamOutput::getAudioFormat()
{
    return static_cast<int>(audioParam->format);
}

int AudioParamOutput::getAudioBytesPerSecond()
{
    return audioParam->audioBytesPerSecond;
}

void AudioParamOutput::setChannels(int nbChannels)
{
    av_channel_layout_uninit(&audioParam->channelLayout);
    av_channel_layout_default(&audioParam->channelLayout, nbChannels);
}

void AudioParamOutput::setSampleRate(int freq)
{
    audioParam->sampleRate = freq;
}

void AudioParamOutput::setBufferSize(int size)
{
    audioParam->audioHardWareBufferSize = size;
}

void AudioParamOutput::updateSampleFormat(AudioSampleFormat sampleForamt)
{
    switch (sampleForamt)
    {
    case SampleFormatS16:
        audioParam->format = AV_SAMPLE_FMT_S16;
        break;
    default:
        break;
    }
}

bool AudioParamOutput::isChannelsNativeOrder()
{
    return audioParam->channelLayout.order == AV_CHANNEL_ORDER_NATIVE;
}

void AudioParamOutput::copyToParam(std::shared_ptr<AudioParam> dstParam)
{
    if (!dstParam)
    {
        return;
    }
    *dstParam.get() = *audioParam.get();
}
