/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef IMAGE_H
#define IMAGE_H

class Image
{
public:
    enum DataType {
        IMG_UNSIGNED_BYTE,
        IMG_FLOAT
    };

    enum ImageFormat {
        IMG_RGB = 3,
        IMG_RGBA = 4
    };

    Image(const unsigned int width, const unsigned int height,
          const ImageFormat format, const DataType dataType, void *data = NULL);

    ~Image();

    unsigned int getWidth() const
    {
        return _width;
    }

    unsigned int getHeight() const
    {
        return _height;
    }

    unsigned int getComponentCount() const
    {
        return (unsigned int)_format;
    }

    unsigned int getDataTypeByteSize() const
    {
        return _dataTypeByteSize;
    }

    DataType getDataType() const
    {
        return _dataType;
    }

    void *getImageBuffer()
    {
        return _dataBuffer;
    }

private:
    const unsigned int _width;
    const unsigned int _height;
    ImageFormat _format;
    DataType _dataType;
    unsigned int _dataTypeByteSize;
    void *_dataBuffer;
};

#endif

