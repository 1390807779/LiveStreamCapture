#include<iostream>
#include<memory>

class Frame;
class AudioParam;
class ResampleData;
class AVFrame;

enum AudioSampleFormat
{
    SampleFormatS16 //暂时只支持一种格式
};

class AudioParamOutput
{
private:
    std::shared_ptr<AudioParam> audioParam;
public:
    AudioParamOutput(std::shared_ptr<AudioParam> _audioParam);
    ~AudioParamOutput();
    int getAudioChannels();
    int getAudioSampleRate();
    int getAudioFormat();
    int getAudioBytesPerSecond();
    void setChannels(int nbChannels);
    void setSampleRate(int freq);
    void setBufferSize(int size);
    void updateSampleFormat(AudioSampleFormat sampleForamt);
    bool isChannelsNativeOrder();
    void copyToParam(std::shared_ptr<AudioParam> dstParam);
};

class AudioDataOutput
{
private:
    uint8_t *datas[8];
    int nbChannels;
    int nbSamples;
    int format;
    std::shared_ptr<ResampleData> resampleData;
    int size;
public:
    AudioDataOutput(std::shared_ptr<Frame> _frame);
    ~AudioDataOutput();
    uint8_t* getData();
    int getSize();
    void setFrame(std::shared_ptr<Frame> _frame);
    void setResampleData(std::shared_ptr<ResampleData> _resampleData);
    bool isNull();
};

class VideoDataOutput
{
private:
    uint8_t *datas[8];
    int lineSize[8];
    int width;
    int height;
    int format;
    int sampleAspectRatioNum;
    int sampleAspectRatioDen;
public:
    VideoDataOutput(std::shared_ptr<Frame> _frame);
    ~VideoDataOutput();
    uint8_t* getDataWithIndex(int index);
    uint8_t** getData();
    int getLinesizeWithIndex(int index);
    int* getLineSize();
    int getDataFormat();
    int getWidth();
    int getHeight();
    void setFrame(std::shared_ptr<Frame> _frame);
    void resize(int *width, int *height);
    bool isFlip();
    bool isNull();
};