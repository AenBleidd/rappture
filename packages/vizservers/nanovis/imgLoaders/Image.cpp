/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <memory.h>
#include <stdlib.h>

#include "Image.h"

Image::Image(const unsigned int width, const unsigned int height, 
	     const ImageFormat format, const Image::DataType type, void *data) :
    _width(width),
    _height(height),
    _format(format),
    _dataType(type)
{
    switch (type) {
    case IMG_UNSIGNED_BYTE:
        _dataTypeByteSize = 1;
        break;
    case IMG_FLOAT:
        _dataTypeByteSize = 4;
        break;
    }

    //_dataBuffer = aligned_malloc(width * height * comp * _dataTypeByteSize, 16);
    _dataBuffer = malloc(width * height * format * _dataTypeByteSize);

    if (data != NULL) {
        memcpy(_dataBuffer, data, width *height * format * _dataTypeByteSize);
    } else {
        memset(_dataBuffer, 0, width * height * format * _dataTypeByteSize);
    }
}

Image::~Image()
{
    //aligend_free(_dataBuffer);
    free(_dataBuffer);
}   
