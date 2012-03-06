/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "ImageLoader.h"
#include "ImageLoaderImpl.h"
#include <map>
#include <string>
#include <stdio.h>

ImageLoader::ImageLoader()
    : _loaderImpl(0)
{
}

Image* ImageLoader::load(const char* fileName)
{
    if (_loaderImpl)
    {
        _loaderImpl->_targetImageFormat = Image::IMG_RGB;
        return _loaderImpl->load(fileName);
    }
    
    return 0;
}

Image* ImageLoader::load(const char* fileName, const Image::ImageFormat format)
{
    if (_loaderImpl)
    {
        _loaderImpl->_targetImageFormat = format;
        return _loaderImpl->load(fileName);
    }
    
    return 0;
}


