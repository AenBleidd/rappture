/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef BMP_IMAGE_LOADER_IMPL_H
#define BMP_IMAGE_LOADER_IMPL_H

#include <ImageLoaderImpl.h>

class BMPImageLoaderImpl : public ImageLoaderImpl
{
public:
    BMPImageLoaderImpl();

    virtual ~BMPImageLoaderImpl();

    virtual Image *load(const char *fileName);
};
#endif
