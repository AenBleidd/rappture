/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "ImageLoaderImpl.h"
#include <stdio.h>

ImageLoaderImpl::ImageLoaderImpl()
{
}

ImageLoaderImpl::~ImageLoaderImpl()
{
}

Image* ImageLoaderImpl::load(const char* filename)
{
    Image* image = 0;
    printf("abstrac function is called ImageLoaderImpl::load\n");
    fflush(stdout);
    return image;
}
