/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "ImageLoader.h"
#include "ImageLoaderImpl.h"
#include "ImageLoaderFactory.h"
#include <map>
#include <string>
#include <stdio.h>

ImageLoaderFactory* ImageLoaderFactory::_instance = 0;

ImageLoaderFactory::ImageLoaderFactory()
{
}

ImageLoaderFactory* ImageLoaderFactory::getInstance()
{
    if (_instance == 0)
    {
        _instance = new ImageLoaderFactory();
    }

    return _instance;
}

void ImageLoaderFactory::addLoaderImpl(const std::string& ext, ImageLoaderImpl* loaderImpl)
{
    std::map< std::string, ImageLoaderImpl*>::iterator iter;
    iter = _loaderImpls.find(ext);
    if (iter == _loaderImpls.end())
    {
        _loaderImpls[ext] = loaderImpl;
    }
    else
    {
        printf("conflick data loader for .%s files\n", ext.c_str());
        return;
    }
}

ImageLoader* ImageLoaderFactory::createLoader(const std::string& ext)
{
    std::map< std::string, ImageLoaderImpl*>::iterator iter;
    iter = _loaderImpls.find(ext);
    if (iter != _loaderImpls.end())
    {
        ImageLoader* imageLoader = new ImageLoader();
        imageLoader->setLoaderImpl((*iter).second);
        return imageLoader;
    }
    else
    {
        printf("%s file not supported\n", ext.c_str());
    }
    return 0;
}


