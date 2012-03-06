/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#include <map>
#include <Image.h>

class ImageLoaderImpl;
class Image;

class ImageLoader {
    friend class ImageLoaderFactory;

    ImageLoaderImpl* _loaderImpl;
public :
    ImageLoader();

public :
    Image* load(const char* fileName);
    Image* load(const char* fileName, const Image::ImageFormat targetFormat);

private :
    void setLoaderImpl(ImageLoaderImpl* loaderImpl);
};

inline void ImageLoader::setLoaderImpl(ImageLoaderImpl* loaderImpl)
{
    _loaderImpl= loaderImpl;
}

#endif //

