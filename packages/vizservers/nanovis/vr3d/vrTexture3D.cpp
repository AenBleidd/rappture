/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <string.h>
#include <math.h>
#include <valarray>

#include <vr3d/vrTexture3D.h>

#ifdef _WIN32
inline unsigned int log2(unsigned int x) 
{
    unsigned int i = 0;
    while ( ( x = ( x >> 1 ) ) != 0 ) i++;
	return i;     
}
#endif

vrTexture3D::vrTexture3D() :
    _width(0),
    _height(0),
    _depth(0)
{
    _target = TT_TEXTURE_3D;
    _wrapT = TW_CLAMP_TO_EDGE;
    _wrapS = TW_CLAMP_TO_EDGE;
}

vrTexture3D::~vrTexture3D()
{
}

void vrTexture3D::setPixels(COLORFORMAT colorFormat, DATATYPE type,
                            int width, int height, int depth, void *data)
{
    setPixels(TT_TEXTURE_3D, colorFormat, colorFormat, type, width, height, depth, data);
}

void vrTexture3D::setPixels(TEXTARGET target,
                            COLORFORMAT internalColorFormat,
                            COLORFORMAT colorFormat,
                            DATATYPE type,
                            int width, int height, int depth, void *data)
{
    _target = target;
    _width = width;
    _height = height;
    _depth = depth;
    _type = type;
    _internalColorFormat = internalColorFormat;
    _colorFormat = colorFormat;
    _compCount = GetNumComponent(_colorFormat);

    if (_objectID) {
        glDeleteTextures(1, &_objectID);
    }

    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &_objectID);
    glBindTexture(_target, _objectID);
    glTexImage3D(_target, 0, _internalColorFormat, _width, _height, _depth, 0, _colorFormat, _type, data) ;
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
	
    //TBD..
    glTexParameterf(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameterf(_target, GL_TEXTURE_WRAP_T, _wrapT);
}

void vrTexture3D::updatePixels(void* data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(_target, _objectID);
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, _wrapT); 
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexImage3D(_target, 0, _internalColorFormat, _width, _height, _depth, 0, _colorFormat, _type, data);
}
