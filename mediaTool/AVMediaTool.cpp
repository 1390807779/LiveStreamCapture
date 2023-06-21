#include <AVMediaTool.h>
#include <Frame.h>
extern "C"
{
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

using namespace std;

AVMediaTool::AVMediaTool()
{
}

AVMediaTool::~AVMediaTool()
{
}

AVDictionary *AVMediaTool::filterCodecOption(AVDictionary *opts, AVCodecID codecId, AVFormatContext *fmtContext, AVStream *stream, const AVCodec *codec)
{
    AVDictionary *ret = nullptr;
    const AVDictionaryEntry *dictEntry = nullptr;
    int flags = fmtContext->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
    char prefix = 0;
    const AVClass *avClass = avcodec_get_class();

    if (!codec)
    {
        codec = fmtContext->oformat ? avcodec_find_encoder(codecId) : avcodec_find_decoder(codecId);
    }
    switch (stream->codecpar->codec_type)
    {
    case AVMEDIA_TYPE_VIDEO:
        prefix  = 'v';
        flags  |= AV_OPT_FLAG_VIDEO_PARAM;
        break;
    case AVMEDIA_TYPE_AUDIO:
        prefix  = 'a';
        flags  |= AV_OPT_FLAG_AUDIO_PARAM;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        prefix  = 's';
        flags  |= AV_OPT_FLAG_SUBTITLE_PARAM;
        break;
    default:
        break;
    }
    while (dictEntry = av_dict_iterate(opts, dictEntry)) {
        const AVClass *priv_class;
        char *str = strchr(dictEntry->key, ':');
        if (str)
            switch (avformat_match_stream_specifier(fmtContext, stream, str + 1)) 
            {
            case  1: 
                *str = 0; 
                break;
            case  0:
                continue;
            default:
                return nullptr;
            }

        if (av_opt_find(&avClass, dictEntry->key, nullptr, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec || 
            ((priv_class = codec->priv_class) && av_opt_find(&priv_class, dictEntry->key, nullptr, flags, AV_OPT_SEARCH_FAKE_OBJ)))
        {
            av_dict_set(&ret, dictEntry->key, dictEntry->value, 0);
        }
        else if (dictEntry->key[0] == prefix && av_opt_find(&avClass, dictEntry->key + 1, nullptr, flags, AV_OPT_SEARCH_FAKE_OBJ))
        {
            av_dict_set(&ret, dictEntry->key + 1, dictEntry->value, 0);
        }
        if (str)
        {
            *str = ':';
        }
    }
    return ret;
}

std::shared_ptr<Frame> AVMediaTool::getPlaceholderFrameWithTypeAndFrame(AVMediaType type, std::shared_ptr<Frame> frame)
{
    AVFrame *initFrame = av_frame_alloc();
    if (!initFrame)
    {
        return nullptr;
    }    
    switch (type)
    {
    case AVMEDIA_TYPE_AUDIO:
        initFrame->sample_rate = frame->frame->sample_rate;
        initFrame->format = frame->frame->format;
        initFrame->nb_samples = frame->frame->nb_samples;
        av_channel_layout_copy(&initFrame->ch_layout, &frame->frame->ch_layout);
        break;
    case AVMEDIA_TYPE_VIDEO:
        initFrame->width = frame->frame->width;
        initFrame->height = frame->frame->height;
        initFrame->format = frame->frame->format;
        break;
    default:
        break;
    }
    av_frame_get_buffer(initFrame, 0);
    if (type == AVMEDIA_TYPE_AUDIO)
    {
        av_samples_set_silence(initFrame->data, 0, initFrame->nb_samples, initFrame->ch_layout.nb_channels, static_cast<AVSampleFormat>(initFrame->format));
    }
    shared_ptr<Frame> out = make_shared<Frame>(initFrame);
    av_frame_free(&initFrame);
    return out;
}
