#include <iostream>
#include <map>

enum DecodeMediaType
{
    decodeUnknowType = -1,
    decodeVideoType,
    decodeAudioType
};

class FormatBridge
{
private:
    std::map<int, uint32_t> imageFormatBridge;
    std::map<int, int> audioFormatBridge;
    std::map<int, int> decodeTypeFormatBridge;
public:
    FormatBridge();
    ~FormatBridge();
    uint32_t getImageSDLFormatWithAVForamt(int format);
    uint32_t getSDLBlendmodeWithAVForamt(int format);
    uint32_t getAudioSDLFormatWithAVForamt(int format);
    int getAVMeidaTypeWithDecodeType(int decodeType);
};
