#include <future>
#include <functional>

const int nextNbChannels[] = {0, 0, 1, 6, 2, 6, 4, 6};
const int nextSampleRates[] = {0, 44100, 48000, 96000, 192000};

class VideoDataOutput;
class AudioDataOutput;
class AudioParamOutput;
class MathFunction;
class FormatBridge;
class VideoState;
struct SDL_Window;
struct SDL_Surface;
struct SDL_Renderer;
union SDL_Event;
struct SDL_AudioSpec;
struct SDL_Texture;
struct SDL_Rect;

using tipCallback = std::function<std::string(std::string)>;

class PlayWindow
{
private:
    SDL_Window *window;
    SDL_Surface *screenSurFace;
    SDL_Renderer *renderer;
    SDL_Event *event;
    uint32_t audioDeviceId;
    SDL_Texture *texture;
    int xTop;
    int yTop;
    int screenWidth;
    int screenHeight;
    bool isQuit; 
    std::string mediaUrl;
    std::shared_ptr<MathFunction> mathFunction;
    std::shared_ptr<VideoState> videoState;
    std::shared_ptr<FormatBridge> formatBridge;
    std::shared_ptr<AudioParamOutput> hardwareAudioParam;
    std::future<int> decodeThread;
    std::future<int> audioPushThread;
    std::future<void> encodeThread;

    int reallocTexture(uint32_t format, int width, int height, uint32_t blendMode, int isInit);
    int startDecodeWithParam(int milliSecondSleepTime, bool *quit);
    void handleSDLEvent();
    void delayForEvent(int milluSecond);
    void caculateDisplayRect(SDL_Rect *rect, std::shared_ptr<VideoDataOutput> videoData);
    int initAudio(std::shared_ptr<AudioParamOutput> audioParam);
    void startEncodeWithParam(std::string url);

public:

    tipCallback tcallback;

    PlayWindow(int width, int height);
    ~PlayWindow();
    int initWindowFrom(void *ptr);
    int initMedia(std::string url);
    void playWithSDL();
    int updateVideoWithSDL();
    int updateVideo(int *recallTime);
    int updateAudioWithSDL();
    int updateAudio(bool *quit);
    int updateTexture(std::shared_ptr<VideoDataOutput> videoData);
    void startDecode();
    void startEncode(std::string saveDir);
    void cancelEncode();
    void changeMuteOrVocal();
};