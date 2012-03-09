/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef IMAGE_LOADER_IMPL_H
#define IMAGE_LOADER_IMPL_H

#include <Image.h>

class ImageLoaderImpl
{
public:
    friend class ImageLoader;

    ImageLoaderImpl();

    virtual ~ImageLoaderImpl();

protected:
    Image::ImageFormat _targetImageFormat;

public:
    virtual Image *load(const char *fileName) = 0;
};

#endif
