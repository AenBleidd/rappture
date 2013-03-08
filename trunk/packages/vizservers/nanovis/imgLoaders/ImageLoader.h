/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <map>

#include <Image.h>

class ImageLoaderImpl;

class ImageLoader
{
public:
    friend class ImageLoaderFactory;

    ImageLoader();

    Image *load(const char *fileName, const Image::ImageFormat targetFormat = Image::IMG_RGB);

private:
    void setLoaderImpl(ImageLoaderImpl *loaderImpl)
    {
        _loaderImpl = loaderImpl;
    }

    ImageLoaderImpl * _loaderImpl;
};

#endif

