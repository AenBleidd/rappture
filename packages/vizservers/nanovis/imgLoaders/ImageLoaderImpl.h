/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __IMAGE_LOADER_IMPL_H__
#define __IMAGE_LOADER_IMPL_H__

#include <Image.h>

class ImageLoaderImpl {
    friend class ImageLoader;

protected :
    Image::ImageFormat _targetImageFormat;

public :
    ImageLoaderImpl();
    virtual ~ImageLoaderImpl();

public :
    virtual Image* load(const char* fileName);
};

#endif
