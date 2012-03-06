/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __IMAGE_LOADER_FACTORY_H__
#define __IMAGE_LOADER_FACTORY_H__

#include <map>
#include <string>

class ImageLoaderImpl;
class Image;
class ImageLoader;

class ImageLoaderFactory {
    std::map< std::string, ImageLoaderImpl*> _loaderImpls;

    static ImageLoaderFactory* _instance;
protected :
    ImageLoaderFactory();
    ~ImageLoaderFactory();

public :
    static ImageLoaderFactory* getInstance();

public :
    void addLoaderImpl(const std::string& ext, ImageLoaderImpl* loaderImpl);
    ImageLoader* createLoader(const std::string& ext);
};

#endif //
