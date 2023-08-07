extern "C"
{
#include <SDL2/SDL.h>
}
#include <PlayWindow.h>
#include <VideoState.h>
#include <iostream>
#include <FormatBridge.h>
#include <ImageSwsContext.h>
#include <MathFunction.h>
#include <thread>
#include <functional>
#include <ctime>
#include <filesystem>

using namespace std;

const int SDLAudioMinBufferSize = 512;
const int SDLAudioMaxCallbackPerSec = 30;
const int videoBufferSize = 5;
const int audioBufferSize = 15;
const int audioHardwareBufferSize = 40960;

int PlayWindow::reallocTexture(uint32_t format, int width, int height, uint32_t blendMode, int isInit)
{
    uint32_t oldFormat;
    int access, oldWidth, oldHeight;
    if (!texture || SDL_QueryTexture(texture, &oldFormat, &access, &oldWidth, &oldHeight) < 0 
                                    || width != oldWidth || height != oldHeight || oldFormat != format)
    {
        void *pixels;
        int pitch;
        if (texture)
        {
            SDL_DestroyTexture(texture);
        }
        if (!(texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height)))
        {
            return -1;
        }
        if (SDL_SetTextureBlendMode(texture, static_cast<SDL_BlendMode>(blendMode)) < 0)
        {
            return -1;
        }
        if (isInit)
        {
            if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) < 0)
            {
                return -1;
            }
            memset(pixels, 0, pitch * height);
            SDL_UnlockTexture(texture);
        }
    }
    
    return 0;
}

int PlayWindow::startDecodeWithParam(int milliSecondSleepTime, bool *quit)
{
    DecodeStatus decodeStatus = decodeStatusError;
    do
    {
        decodeStatus = videoState->startDecode();
        if (decodeStatus == decodeStatusBufferFull)
        {
            this_thread::sleep_for(chrono::milliseconds(milliSecondSleepTime));
        }
        
    } while (!*quit && (decodeStatus == decodeStatusSuccess || decodeStatus == decodeStatusBufferFull));
    

    return static_cast<int>(decodeStatus);
}

void PlayWindow::handleSDLEvent()
{
    if (SDL_PollEvent(event) <= 0)
    {
        return;
    }
    if (!event)
    {
        return;
    }
    switch (event->type)
    {
    case SDL_QUIT:
        isQuit = true;
        break;
    case SDL_WINDOWEVENT:
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_CLOSE:
            isQuit = true;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void PlayWindow::delayForEvent(int milliSecond)
{
    chrono::milliseconds time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()) + chrono::milliseconds(milliSecond);
    while (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()) < time)
    {
        handleSDLEvent();
    }
}

void PlayWindow::caculateDisplayRect(SDL_Rect * rect, std::shared_ptr<VideoDataOutput> videoData)
{
    int width = screenWidth;
    int height = screenHeight;
    videoData->resize(&width, &height);
    rect->x = xTop + (screenWidth - width) / 2;
    rect->y = yTop + (screenHeight - height) / 2;
    rect->w = max(width, 1);
    rect->h = max(height, 1);
}

PlayWindow::PlayWindow(int width, int height) : 
    xTop(0),
    yTop(0),
    screenWidth(width),
    screenHeight(height),
    window(nullptr),
    screenSurFace(nullptr),
    renderer(nullptr),
    event(nullptr),
    mathFunction(make_shared<MathFunction>()),
    videoState(nullptr),
    formatBridge(make_shared<FormatBridge>()),
    isQuit(false),
    texture(nullptr),
    mediaUrl("")
{
}

PlayWindow::~PlayWindow()
{
    isQuit = true;
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    if (screenSurFace)
    {
        SDL_FreeSurface(screenSurFace);
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }
}

int PlayWindow::initWindowFrom(void *ptr)
{
    int ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (ret < 0)
    {
        cout << "init SDL video and audio failed" << endl;
        return ret;
    }
    if (ptr)
    {
        window = SDL_CreateWindowFrom(ptr);
    }
    else
    {
        window = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
    }

    if (!window)
    {
        cout << "init window failed" << endl;
        return -1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer)
    {
        cout << "init renderer failed" << endl;
        return -1;
    }
    return 0;
}

int PlayWindow::initMedia(string url)
{
    mediaUrl = url;
    videoState = make_shared<VideoState>(url, videoBufferSize, audioBufferSize);
    int ret = videoState->initDecoderWithUrl();
    if (ret < 0)
    {
        return ret;
    }
    initAudio(videoState->getAudioParamOutput());
    videoState->updateHardWareParam(hardwareAudioParam);
    hardwareAudioParam = videoState->getAudioParamOutput();
    videoState->eallback = tcallback;
    return 0;
}

int PlayWindow::initAudio(shared_ptr<AudioParamOutput> audioParam)
{
    SDL_AudioSpec inputSpec, outputSpec;
    int inputChannels = audioParam->getAudioChannels();
    const char *env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env)
    {
        inputChannels = atoi(env);
        audioParam->setChannels(inputChannels);
    }
    if (!audioParam->isChannelsNativeOrder())
    {
        audioParam->setChannels(inputChannels);
    }
    inputChannels = audioParam->getAudioChannels();
    inputSpec.channels = inputChannels;
    inputSpec.freq = audioParam->getAudioSampleRate();
    if (inputSpec.freq <= 0 || inputSpec.channels <= 0)
    {
        cout << "freq or channels error" << endl;
        return -1;
    }
    int nextSampleRateIndex = FF_ARRAY_ELEMS(nextSampleRates) - 1;
    while (nextSampleRateIndex && nextSampleRates[nextSampleRateIndex] >= inputSpec.freq)
    {
        nextSampleRateIndex--;
    }
    inputSpec.format = formatBridge->getAudioSDLFormatWithAVForamt(audioParam->getAudioFormat());
    inputSpec.silence = 0;
    inputSpec.samples = max(SDLAudioMinBufferSize, 2 << mathFunction->AVLog(inputSpec.freq / SDLAudioMaxCallbackPerSec));
    inputSpec.callback = nullptr;
    inputSpec.userdata = nullptr;
    while (!(audioDeviceId = static_cast<Uint32>(SDL_OpenAudioDevice(nullptr, 0, &inputSpec, &outputSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))))
    {
        inputSpec.channels = nextNbChannels[FFMIN(7, inputSpec.channels)];
        if (!inputSpec.channels)
        {
            inputSpec.freq = nextSampleRates[nextSampleRateIndex--];
            inputSpec.channels = inputChannels;
            if (!inputSpec.freq)
            {
                cout << "audio open failed" << endl;
                return -1;
            }
        }
        audioParam->setChannels(inputSpec.channels);
    }
    if (outputSpec.format != AUDIO_S16SYS)
    {
        cout << "SDL audio is not supported" << endl;
        return -1;
    }
    if (outputSpec.channels != inputSpec.channels)
    {
        audioParam->setChannels(outputSpec.channels);
        if (!audioParam->isChannelsNativeOrder())
        {
            cout << "SDL channel is not supported" << endl;
            return -1;
        }
    }
    audioParam->updateSampleFormat(SampleFormatS16);
    audioParam->setSampleRate(outputSpec.freq);
    audioParam->setBufferSize(outputSpec.size);
    hardwareAudioParam = audioParam;
    return 0;
}

int PlayWindow::updateVideoWithSDL()
{
    int time = 10;
    delayForEvent(time);
    while (!isQuit)
    {
        int ret = updateVideo(&time);
        if (ret < 0)
        {
            cout << "update video failed" << endl;
        }

        delayForEvent(time);
        if (isQuit)
        {
            break;
        }
    }
    return 0;
}

int PlayWindow::updateVideo(int * recallTime)
{
    double time = static_cast<double>(*recallTime / 1000.0);
    shared_ptr<VideoDataOutput> videoData = videoState->getVideoData(&time);
    if (!videoData || videoData->isNull())
    {
        return -1;
    }
    *recallTime = static_cast<int>(time * 1000.0);
    int ret = updateTexture(videoData);
    if (ret < 0)
    {
        cout << "updateTextrue failed" << endl;
    }
    return ret;
}

int PlayWindow::updateAudio(bool *quit)
{
    while (!*quit)
    {
        uint32_t queueAudioSize = SDL_GetQueuedAudioSize(static_cast<SDL_AudioDeviceID>(audioDeviceId));
        if (queueAudioSize < audioHardwareBufferSize)
        {
            shared_ptr<AudioDataOutput> audioData = videoState->getAudioData(queueAudioSize);
            if (!audioData)
            {
                cout << "audio data is null" << endl;
            }
            else
            {
                SDL_QueueAudio(static_cast<SDL_AudioDeviceID>(audioDeviceId), audioData->getData(), audioData->getSize());
                continue;
            }
        }
        videoState->updateAudioClock(queueAudioSize);
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    return 0;
}

int PlayWindow::updateTexture(shared_ptr<VideoDataOutput> videoData)
{
    int format = videoData->getDataFormat(); 
    int width = videoData->getWidth();
    int height = videoData->getHeight();
    int *lineSize = videoData->getLineSize();
    uint8_t **data = videoData->getData();
    uint8_t *data0 = videoData->getDataWithIndex(0);
    uint8_t *data1 = videoData->getDataWithIndex(1);
    uint8_t *data2 = videoData->getDataWithIndex(2);
    int lineSize0 = videoData->getLinesizeWithIndex(0);
    int lineSize1 = videoData->getLinesizeWithIndex(1);
    int lineSize2 = videoData->getLinesizeWithIndex(2);
    if (!data)
    {
        return -1;
    }
    
    SDL_PixelFormatEnum sdlPixelFormat = static_cast<SDL_PixelFormatEnum>(formatBridge->getImageSDLFormatWithAVForamt(format));
    SDL_BlendMode sdlBlendMode = static_cast<SDL_BlendMode>(formatBridge->getSDLBlendmodeWithAVForamt(format));
    if (reallocTexture(sdlPixelFormat == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdlPixelFormat, width, height, sdlBlendMode, 0) < 0)
    {
        return -1;
    }
    shared_ptr<ImageSwsContext> imageSwsContext = make_shared<ImageSwsContext>();
    shared_ptr<ImageParam> imageParam = make_shared<ImageParam>();
    imageParam->format = format;
    imageParam->height = height;
    imageParam->width = width;
    imageParam->swsFlags = SWS_BICUBIC;
    imageParam->lineSize = lineSize;
    int ret = 0;
    switch (sdlPixelFormat)
    {
    case SDL_PIXELFORMAT_UNKNOWN:
        imageSwsContext->setFormatImageSwsContextWithImageParam(format, imageParam);
        if (imageSwsContext->isNull())
        {
            ret = -1;
        }
        else
        {
            uint8_t **pixels;
            int *pitch;
            if (!SDL_LockTexture(texture, nullptr, (void **)pixels, pitch))
            {
                ret = imageSwsContext->scaleImage(data, lineSize, pixels, pitch);
                SDL_UnlockTexture(texture);
            }
        }
        break;
    case SDL_PIXELFORMAT_IYUV:
        if (lineSize0 > 0 && lineSize1 > 0 && lineSize2 > 0)
        {
            ret = SDL_UpdateYUVTexture(texture, nullptr, data0, lineSize0,
                                            data1, lineSize1,
                                            data2, lineSize2);
        }
        else if (lineSize0 < 0 && lineSize1 < 0 && lineSize2 < 0)
        {
            ret = SDL_UpdateYUVTexture(texture, nullptr, data0 + lineSize0 * (height - 1), lineSize0,
                                                        data1 + lineSize1 * (mathFunction->AVCeilRightShift(height, 1) - 1), lineSize1,
                                                        data2 + lineSize2 * (mathFunction->AVCeilRightShift(height, 1) - 1), lineSize2);
        }
        else
        {
            return -1;
        }
        break;
    default:
        if (*lineSize < 0)
        {
            ret = SDL_UpdateTexture(texture, nullptr, data0 + lineSize0 * (height - 1), -lineSize0);   
        }
        else
        {
            ret = SDL_UpdateTexture(texture, nullptr, data0, lineSize0);
        }
        break;
    }
    SDL_Rect rect;
    caculateDisplayRect(&rect, videoData);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);
    return ret;
}

int PlayWindow::updateAudioWithSDL()
{
    auto audioUpdateFunction = bind(&updateAudio, this, placeholders::_1);
    audioPushThread = async(audioUpdateFunction, &isQuit);
    SDL_PauseAudioDevice(static_cast<SDL_AudioDeviceID>(audioDeviceId), 0);
    return 0;
}

void PlayWindow::startDecode()
{
    int decodeSleepTime = static_cast<int>(videoState->getVideoFrameRate() * 1000);
    auto decodeFuction = bind(&startDecodeWithParam, this, placeholders::_1, placeholders::_2);
    int audioBytesPerSecond = hardwareAudioParam->getAudioBytesPerSecond();
    if (audioBytesPerSecond > 0)
    {
        // 取缓存的十分之一时间
        decodeSleepTime = min(decodeSleepTime, static_cast<int>((static_cast<double>(audioHardwareBufferSize) / audioBytesPerSecond * 1000.0) / 10.0));
    }
    decodeThread = async(launch::async, decodeFuction, decodeSleepTime, &isQuit);
}

void PlayWindow::startEncode(std::string saveDir)
{
    string suffix = mediaUrl.substr(mediaUrl.find_last_of('.') + 1);
    suffix = suffix.substr(0, suffix.find_first_of('?')); // for http url
    time_t now = time(0);
    tm *time = localtime(&now);
    string filename = to_string(time->tm_year) + "-" + to_string(time->tm_mon) + "-" 
                    + to_string(time->tm_mday) + "-" + to_string(time->tm_hour) + "-"
                    + to_string(time->tm_min) + "-" + to_string(time->tm_sec) + "." + suffix;
    filename = saveDir + "/" + filename;
    auto encodeFunction = bind(&startEncodeWithParam, this, placeholders::_1);
    encodeThread = async(launch::async, encodeFunction, filename);
}

void PlayWindow::startEncodeWithParam(std::string url)
{
    videoState->startEncodeTo(url);
}

void PlayWindow::cancelEncode()
{
    videoState->cancelEncode();
}

void PlayWindow::changeMuteOrVocal()
{
    videoState->changeAudioOutputWithIsMute();
}

void PlayWindow::playWithSDL()
{
    updateAudioWithSDL();
    updateVideoWithSDL();
}
