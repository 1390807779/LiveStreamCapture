#include<iostream>
#include<AudioSwrContext.h>
using namespace std;

void AudioParam::operator()(AudioParam *param)
{
    av_channel_layout_copy(&this->channelLayout, &param->channelLayout);
    this->format = param->format;
    this->nChannel = param->nChannel;
    this->sampleRate = param->sampleRate;
}

void AudioParam::operator=(AudioParam param)
{
    av_channel_layout_copy(&this->channelLayout, &param.channelLayout);
    this->format = param.format;
    this->nChannel = param.nChannel;
    this->sampleRate = param.sampleRate;
    this->audioBytesPerSecond = param.audioBytesPerSecond;
    this->audioHardWareBufferSize = param.audioHardWareBufferSize;
    this->frameSize = param.frameSize;
}

AudioParam::AudioParam():
    format(AV_SAMPLE_FMT_S16),
    nChannel(-1),
    sampleRate(-1)
{
}

AudioParam::~AudioParam()
{
    av_channel_layout_uninit(&channelLayout);
}

AudioSwrContext::AudioSwrContext():
    swrContext(nullptr),
    inAudioParam(make_shared<AudioParam>()),
    outAudioParam(make_shared<AudioParam>())
{
}

AudioSwrContext::~AudioSwrContext()
{
    if (swrContext)
    {
        swr_free(&swrContext);
    }
}

int AudioSwrContext::initWithAudioParam(shared_ptr<AudioParam> inParam, shared_ptr<AudioParam> outParam)
{
    swr_alloc_set_opts2(&swrContext, 
        &outParam->channelLayout, outParam->format, outParam->sampleRate, 
        &inParam->channelLayout, inParam->format, inParam->sampleRate, 
        0, nullptr);
    int ret = 0;
    if (!swrContext || swr_init(swrContext) < 0)
    {
        ret = -1;
        cout << "can not creat sample rate converter" << endl;
    }
    *outAudioParam.get() = *outParam.get();
    *inAudioParam.get() = *inParam.get();
    return ret;
}

int AudioSwrContext::resample(const uint8_t **in, shared_ptr<ResampleData> resampleData)
{
    if (!resampleData->getDataPtr())
    {
        cout << "no audio buffer " << endl;
        return -1;
    }
    int len = swr_convert(swrContext, resampleData->getDataPtr(), resampleData->getMinSize(), in, resampleData->getNbSample());
    if (len < 0)
    {
        cout << "swr_convert failed" << endl;
        return -1;
    }
    if (len == resampleData->getMinSize())
    {
        cout << "audio buffer is probably too small" << endl;
        return -1;
    }
    int size = len * outAudioParam->channelLayout.nb_channels * av_get_bytes_per_sample(outAudioParam->format);
    resampleData->setSize(size);
    return 0;
}

int AudioSwrContext::resampleCustomize(const uint8_t **in, uint8_t **out, int *size, int nbSamples)
{
    if (!out)
    {
        cout << "no audio buffer " << endl;
        return -1;
    }
    int outCount = static_cast<int64_t>(nbSamples) * inAudioParam->sampleRate / outAudioParam->sampleRate + 256;
    int minSize = av_samples_get_buffer_size(NULL, outAudioParam->channelLayout.nb_channels, outCount, outAudioParam->format, 0);
    int len = swr_convert(swrContext, out, minSize, in, nbSamples);
    if (len < 0)
    {
        cout << "swr_convert failed" << endl;
        return -1;
    }
    if (len == minSize)
    {
        cout << "audio buffer is probably too small" << endl;
        return -1;
    }
    int reSize = len * outAudioParam->channelLayout.nb_channels * av_get_bytes_per_sample(outAudioParam->format);
    if (size)
    {
        *size = reSize;
    }
    return 0;
}

ResampleData::ResampleData():
    isInit(false),
    nbSamples(0),
    minSize(0),
    size(0)
{
    for (auto &&d : data)
    {
        d = nullptr;
    }
}

ResampleData::~ResampleData()
{
    if (isInit)
    {
        av_freep(data);
    }
}

int ResampleData::initDataWithParam(std::shared_ptr<AudioParam> param, int _nbSamples)
{
    nbSamples = _nbSamples;
    int outCount = static_cast<int64_t>(nbSamples) * param->sampleRate / param->sampleRate + 256;
    minSize = av_samples_get_buffer_size(NULL, param->channelLayout.nb_channels, outCount, param->format, 0);
    if (minSize < 0)
    {
        cout << "init resample data failed" << endl;
        return -1;
    }
    unsigned int audioBufferSize = 0;
    av_fast_malloc(data, &audioBufferSize, minSize);
    isInit = true;
    return 0;
}

uint8_t **ResampleData::getDataPtr()
{
    return data;
}

uint8_t *ResampleData::getData()
{
    return data[0];
}

int ResampleData::getNbSample()
{
    return nbSamples;
}

int ResampleData::getSize()
{
    return size;
}

int ResampleData::getMinSize()
{
    return minSize;
}

void ResampleData::setSize(int _size)
{
    size = _size;
}
