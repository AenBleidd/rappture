/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef IMAGE_LOADER_IMPL_H
#define IMAGE_LOADER_IMPL_H

#include <Image.h>

class ImageLoaderImpl
{
public:
    ImageLoaderImpl();

    virtual ~ImageLoaderImpl();

    virtual Image *load(const char *fileName) = 0;

    friend class ImageLoader;

protected:
    Image::ImageFormat _targetImageFormat;
};

#endif
