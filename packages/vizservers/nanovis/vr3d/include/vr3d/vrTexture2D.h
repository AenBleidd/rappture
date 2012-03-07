/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRTEXTURE2D_H
#define VRTEXTURE2D_H

#include <vr3d/vrEnums.h>
#include <vr3d/vrTexture.h>

class vrTexture2D : public vrTexture
{
    /**
     * @brief Two-dimensional texture width.
     * @brief Default value is 0
     */
    int _width;

    /**
     * @brief Two-dimensional texture height.
     * @brief Default value is 0
     */
    int _height;

    /**
     * @brief Wrap mode.
     * @brief Default value is R2_CLAMP
     */
    TEXWRAP _wrapS;

    /**
     * @brief Wrap mode.
     * @brief Default value is R2_CLAMP
     */
    TEXWRAP _wrapT;

    bool _depthTexture;
public:
    /**
     * @brief constructuor
     */
    vrTexture2D(bool depth = false);

    /** 
     * @brief destructor
     */
    ~vrTexture2D();

    /**
     * @brief get texture size
     * @return texture length
     */
    int getWidth() const;

    /**
     * @brief get texture height
     * @return texture height
     */
    int getHeight() const;

    /**
     * @brief set wrap mode
     */
    void setWrapS(TEXWRAP wrap);

    /*
     * @brief set wrap mode
     */
    void setWrapT(TEXWRAP wrap);

    /**
     * @brief set wrap mode
     */
    TEXWRAP getWrapS() const;

    /*
     * @brief set wrap mode
     */
    TEXWRAP getWrapT() const;

    /**
     * @brief set texture data
     * @param format texture data format 
     * @param type texture data type 
     * @param width texture width
     * @param data texture data
     */
    void setPixels(COLORFORMAT format, DATATYPE type,
                   int width, int height, void* data);

    /**
     * @brief set texture data
     * @param internalFormat texture internal data format 
     * @param format texture data format 
     * @param type texture data type 
     * @param width texture width
     * @param data texture data
     */
    void setPixels(TEXTARGET target, COLORFORMAT internalFormat,
                   COLORFORMAT format, DATATYPE type, int width, int height, void *data);

    /**
     * @brief set texture data (NV)
     * @param internalFormat internal colorformat
     * @param format texture data format 
     * @param type texture data type 
     * @param width texture width
     * @param data texture data
     */
    void setPixels(COLORFORMAT internalFormat, COLORFORMAT format,
                   DATATYPE type, int width, int height, void *data);

    /**
     * @brief update pixel data
     * @param data pixel data
     */
    virtual void updatePixels(void *data);
};

inline int vrTexture2D::getWidth() const
{
    return _width;
}

inline int vrTexture2D::getHeight() const
{
    return _height;
}

inline void vrTexture2D::setWrapS(TEXWRAP wrap)
{
    _wrapS = wrap;
}

inline void vrTexture2D::setWrapT(TEXWRAP wrap)
{
    _wrapT = wrap;
}

inline TEXWRAP vrTexture2D::getWrapS() const
{
    return _wrapS;
}

inline TEXWRAP vrTexture2D::getWrapT() const
{
    return _wrapT;
}

#endif
