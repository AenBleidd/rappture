/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <vr3d/vrTexture.h>

vrTexture::vrTexture() :
    _objectID(0), 
    _type(DT_FLOAT), 
    _colorFormat(CF_RGBA), 
    _internalColorFormat(CF_RGBA),
    _minFilter(TF_LINEAR), 
    _magFilter(TF_LINEAR),
    _compCount(4)
{
}

vrTexture::~vrTexture()
{
    glDeleteTextures(1, &_objectID);
}
