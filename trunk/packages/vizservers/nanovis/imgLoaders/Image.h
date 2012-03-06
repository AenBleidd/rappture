/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __IMAGE_H__
#define __IMAGE_H__

class Image {
public :
    enum DataType {
        IMG_UNSIGNED_BYTE,
        IMG_FLOAT
    };

    enum ImageFormat {
        IMG_RGB = 3,
        IMG_RGBA = 4
    };

private :
    const unsigned int _width;
    const unsigned int _height;
    ImageFormat _format;
    DataType _dataType;
    unsigned int _dataTypeByteSize;
    void* _dataBuffer;
public :

    Image(const unsigned int width, const unsigned int height, const ImageFormat format, const DataType dataType, void* data = 0);
    ~Image();

    unsigned int getWidth() const;
    unsigned int getHeight() const;
    unsigned int getComponentCount() const;
    unsigned int getDataTypeByteSize() const;
    DataType getDataType() const;
    void* getImageBuffer();
};

inline  void* Image::getImageBuffer()
{
    return _dataBuffer;
}

inline unsigned int Image::getWidth() const
{
    return _width;
}

inline unsigned int Image::getHeight() const
{
    return _height;
}

inline unsigned int Image::getComponentCount() const
{
    return (unsigned int) _format;
}

inline Image::DataType Image::getDataType() const
{
    return _dataType;
}

inline unsigned int Image::getDataTypeByteSize() const
{
    return _dataTypeByteSize;
}

#endif // 

