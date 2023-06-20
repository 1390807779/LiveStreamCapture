#include <iostream>
#include <memory>
extern "C"
{
#include <libswscale/swscale.h>
}
class ImageParam
{
private:
public:
    int width;
    int height;
    int format;
    int swsFlags;
    int *lineSize;
    ImageParam();
    ~ImageParam();
    void operator=(ImageParam _imageParam);
};

class ImageSwsContext
{
private:
    SwsContext *imageSwsContext;
    std::shared_ptr<ImageParam> imageParam;
public:
    ImageSwsContext();
    ~ImageSwsContext();
    void setFormatImageSwsContextWithImageParam(int foramt, std::shared_ptr<ImageParam> param);
    void setFormatImageSwsContextWithTwoParam(std::shared_ptr<ImageParam> inParam, std::shared_ptr<ImageParam> outParam);
    int scaleImage(uint8_t **data, int lineSize[], uint8_t *pixels[], int pitch[]);
    bool isNull();
};

