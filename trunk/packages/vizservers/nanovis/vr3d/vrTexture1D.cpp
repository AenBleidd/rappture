/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <vr3d/vrTexture1D.h>

vrTexture1D::vrTexture1D(bool depth):
    _width(0)
{
    _target = TT_TEXTURE_1D;
    _wrapS = TW_CLAMP_TO_EDGE;
}

vrTexture1D::~vrTexture1D()
{
}

void vrTexture1D::setPixels(COLORFORMAT colorFormat, DATATYPE type,
                            int width, void *data)
{
    setPixels(TT_TEXTURE_1D, colorFormat,  colorFormat, type, width, data);
}

void vrTexture1D::setPixels(TEXTARGET target,  COLORFORMAT internalColorFormat,
                            COLORFORMAT colorFormat, DATATYPE type,
                            int width, void* data)
{
    _target = target;
    _width = width;
    _type = type;
    _internalColorFormat = internalColorFormat;
    _colorFormat = colorFormat;
    _compCount = GetNumComponent(_colorFormat);

    if (_objectID) {
        glDeleteTextures(1, &_objectID);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &_objectID);
    glBindTexture(_target, _objectID);

    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexImage1D(_target, 0, _internalColorFormat, _width, 0, _colorFormat, _type, data);
}

void vrTexture1D::updatePixels(void *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(_target, _objectID);
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexImage1D(_target, 0, _internalColorFormat, _width, 0, _colorFormat, _type, data);
}
