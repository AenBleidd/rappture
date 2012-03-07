/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <string.h>
#include <math.h>
#include <valarray>

#include <vr3d/vrTexture2D.h>

#ifdef _WIN32
inline unsigned int log2(unsigned int x) 
{
    unsigned int i = 0;
    while ( ( x = ( x >> 1 ) ) != 0 ) i++;
	return i;     
}
#endif

vrTexture2D::vrTexture2D(bool depth) :
    _width(0),
    _depthTexture(depth)
{
    _target = TT_TEXTURE_2D;
    _wrapT = TW_CLAMP_TO_EDGE;
    _wrapS =TW_CLAMP_TO_EDGE;
}

vrTexture2D::~vrTexture2D()
{
}

void vrTexture2D::setPixels(COLORFORMAT colorFormat, DATATYPE type,
                            int width, int height, void *data)

{
    TEXTARGET target;
#ifndef OPENGLES
    if ((pow(2.0f, (float)log2(width)) != width) && 
        (pow(2.0f, (float)log2(height)) != height)) {
        if (type == DT_FLOAT) {
            target = TT_TEXTURE_RECTANGLE;
        } else {
            target = TT_TEXTURE_2D;
        }
    } else {
        target = TT_TEXTURE_2D;
    }
#else
    target = TT_TEXTURE_2D;
#endif

    setPixels(target, colorFormat, colorFormat, type, width, height, data);
}

void vrTexture2D::setPixels(TEXTARGET target,
                            COLORFORMAT internalColorFormat,
                            COLORFORMAT colorFormat,
                            DATATYPE type, int width, int height, void* data)
{
    _target = target;
    _width = width;
    _height = height;
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
    glTexImage2D(_target, 0, _internalColorFormat, _width, _height, 0, _colorFormat, _type, data);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
	
    // TBD...
    if (_depthTexture) {
        //glTexParameteri(_target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
        glTexParameteri(_target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

        glTexParameteri(_target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE); 

        //glDrawBuffer(GL_NONE);
        //glReadBuffer(GL_NONE);
    }
	
    //TBD..
    glTexParameterf(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameterf(_target, GL_TEXTURE_WRAP_T, _wrapT);
}

void vrTexture2D::updatePixels(void *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(_target, _objectID);
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrapT);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _magFilter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexImage2D(_target, 0, _internalColorFormat, _width, _height, 0, _colorFormat, _type, data);
}

