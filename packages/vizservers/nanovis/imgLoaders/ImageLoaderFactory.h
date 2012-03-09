/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef IMAGE_LOADER_FACTORY_H
#define IMAGE_LOADER_FACTORY_H

#include <map>
#include <string>

class ImageLoaderImpl;
class Image;
class ImageLoader;

class ImageLoaderFactory
{
public:
    void addLoaderImpl(const std::string& ext, ImageLoaderImpl *loaderImpl);
    ImageLoader *createLoader(const std::string& ext);

    static ImageLoaderFactory *getInstance();

protected :
    ImageLoaderFactory();
    ~ImageLoaderFactory();

private:
    std::map<std::string, ImageLoaderImpl *> _loaderImpls;
    static ImageLoaderFactory *_instance;
};

#endif
