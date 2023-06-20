#include <FormatBridge.h>
extern "C"
{
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
}

FormatBridge::FormatBridge()
{
    imageFormatBridge = {
        { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
        { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
        { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
        { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
        { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
        { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
        { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
        { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
        { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
        { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
        { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
        { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
        { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
        { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
        { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
        { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
        { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
        { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
        { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
        { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
    };

    audioFormatBridge = {
        {AV_SAMPLE_FMT_S16,          AUDIO_S16SYS},
    };

    decodeTypeFormatBridge = {
        {decodeUnknowType,           AVMEDIA_TYPE_UNKNOWN},
        {decodeVideoType ,           AVMEDIA_TYPE_VIDEO},
        {decodeAudioType,            AVMEDIA_TYPE_AUDIO},
    };
}

FormatBridge::~FormatBridge()
{
    
}

uint32_t FormatBridge::getImageSDLFormatWithAVForamt(int format)
{
    if (imageFormatBridge.find(format) == imageFormatBridge.end())
    {
        return SDL_PIXELFORMAT_UNKNOWN;
    }
    return imageFormatBridge[format];
}

uint32_t FormatBridge::getSDLBlendmodeWithAVForamt(int format)
{
    if (format == AV_PIX_FMT_RGB32 ||
        format == AV_PIX_FMT_RGB32_1 ||
        format == AV_PIX_FMT_BGR32 ||
        format == AV_PIX_FMT_BGR32_1)
    {
        return SDL_BLENDMODE_BLEND;
    }
    
    return SDL_BLENDMODE_NONE;
}

uint32_t FormatBridge::getAudioSDLFormatWithAVForamt(int format)
{
    if (audioFormatBridge.find(format) == audioFormatBridge.end())
    {
        return AUDIO_S16SYS;
    }
    
    return audioFormatBridge[format];
}

int FormatBridge::getAVMeidaTypeWithDecodeType(int decodeType)
{
    if (decodeTypeFormatBridge.find(decodeType) == decodeTypeFormatBridge.end())
    {
        return static_cast<int>(AVMEDIA_TYPE_UNKNOWN);
    }
    return decodeTypeFormatBridge[decodeType];
}
