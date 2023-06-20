extern "C"
{
#include<libswresample/swresample.h>
}
#include<memory>

class AudioParam
{
public:
    int nChannel = -1;
    int sampleRate = -1;
    AVChannelLayout channelLayout;
    AVSampleFormat format;
    int audioHardWareBufferSize = 0;
    int audioBytesPerSecond = 0;
    int frameSize = 0;
    AudioParam();
    ~AudioParam();
    void operator()(AudioParam *param);
    void operator = (AudioParam param);
};

class ResampleData
{
private:
    uint8_t *data[8];
    bool isInit;
    int nbSamples;
    int minSize;
    int size;
public:
    ResampleData();
    ~ResampleData();
    int initDataWithParam(std::shared_ptr<AudioParam> param, int nbSamples);
    uint8_t **getDataPtr();
    uint8_t *getData();
    int getNbSample();
    int getSize();
    int getMinSize();
    void setSize(int _size);
};

class AudioSwrContext
{
private:
    SwrContext *swrContext;
    std::shared_ptr<AudioParam> outAudioParam;
    std::shared_ptr<AudioParam> inAudioParam;
public:
    AudioSwrContext();
    ~AudioSwrContext();
    int initWithAudioParam(std::shared_ptr<AudioParam> inParam, std::shared_ptr<AudioParam> outParam);
    int resample(const uint8_t **in, std::shared_ptr<ResampleData> resampleData);
    int resampleCustomize(const uint8_t **in, uint8_t **out, int *size, int nbSamples);
};
