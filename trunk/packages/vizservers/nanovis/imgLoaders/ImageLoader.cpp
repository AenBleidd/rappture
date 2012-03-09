/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>

#include <map>
#include <string>

#include "ImageLoader.h"
#include "ImageLoaderImpl.h"

ImageLoader::ImageLoader() :
    _loaderImpl(NULL)
{
}

Image *ImageLoader::load(const char* fileName, const Image::ImageFormat format)
{
    if (_loaderImpl) {
        _loaderImpl->_targetImageFormat = format;
        return _loaderImpl->load(fileName);
    }

    return 0;
}
