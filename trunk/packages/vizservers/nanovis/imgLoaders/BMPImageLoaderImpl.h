/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __BMP_IMAGE_LOADER_IMPL_H__
#define __BMP_IMAGE_LOADER_IMPL_H__

#include <ImageLoaderImpl.h>

class BMPImageLoaderImpl : public ImageLoaderImpl {
public : 
    BMPImageLoaderImpl();
    ~BMPImageLoaderImpl();

public :
    virtual Image* load(const char* fileName);

};
#endif /* __BMP_IMAGE_LOADER_IMPL_H__ */
