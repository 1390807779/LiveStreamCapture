#include <ImageSwsContext.h>
#include <utility>
#include <type_traits>

using namespace std;

ImageParam::ImageParam():
    width(0),
    height(0),
    format(static_cast<AVPixelFormat>(0)),
    swsFlags(0)
{
}

ImageParam::~ImageParam()
{
}

inline void ImageParam::operator=(ImageParam _imageParam)
{
    this->width = _imageParam.width;
    this->height = _imageParam.height;
    this->format = _imageParam.format;
    this->swsFlags = _imageParam.swsFlags;
}

ImageSwsContext::ImageSwsContext():
    imageSwsContext(nullptr)
{
}

ImageSwsContext::~ImageSwsContext()
{
    if (imageSwsContext)
    {
        sws_freeContext(imageSwsContext);
    }
}

void ImageSwsContext::setFormatImageSwsContextWithImageParam(int foramt, shared_ptr<ImageParam> param)
{
    *imageParam.get() = *param.get();
    imageSwsContext = sws_getCachedContext(imageSwsContext, param->width, param->height, static_cast<AVPixelFormat>(param->format), param->width, param->height, 
                                            static_cast<AVPixelFormat>(foramt), param->swsFlags, nullptr, nullptr, nullptr);
}

void ImageSwsContext::setFormatImageSwsContextWithTwoParam(std::shared_ptr<ImageParam> inParam, std::shared_ptr<ImageParam> outParam)
{
    *imageParam.get() = *inParam.get();
    imageSwsContext = sws_getCachedContext(imageSwsContext, inParam->width, inParam->height, static_cast<AVPixelFormat>(inParam->format), outParam->width, outParam->height, 
                                            static_cast<AVPixelFormat>(outParam->format), outParam->swsFlags, nullptr, nullptr, nullptr);
}

int ImageSwsContext::scaleImage(uint8_t **data, int lineSize[], uint8_t *pixels[], int pitch[])
{
    if (!imageSwsContext)
    {
        cout << "imageSwsContext is null" << endl;
        return -1;
    }
    sws_scale(imageSwsContext, add_const_t<const uint8_t *const *>(data), lineSize, 0, imageParam->height, pixels, pitch);
    return 0;
}

bool ImageSwsContext::isNull()
{
    if (!imageSwsContext)
    {
        return true;
    }
    return false;
}