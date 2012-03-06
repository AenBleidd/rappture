/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vr3d/vr3d.h>

#ifndef PI
#   define PI 3.14159265358979323846f
#endif

enum COLORFORMAT {
    CF_LUMINANCE = GL_LUMINANCE, /*< GL_LIMINANCE */

#ifndef OPENGLES
    CF_RGB8 = GL_RGB8, /*< GL_RGB8 */
    CF_FLOAT_RGBA32 = GL_FLOAT_RGBA32_NV,
    CF_DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
#endif
    CF_RGB = GL_RGB,            /*< GL_UNSIGNED_BYTE */
    CF_RGBA = GL_RGBA,          /*< GL_FLOAT */
};

enum DATATYPE {
    DT_UBYTE = GL_UNSIGNED_BYTE,    /*< GL_UNSIGNED_BYTE */
    DT_FLOAT = GL_FLOAT,            /*< GL_FLOAT */
#ifndef OPENGLES
    DT_UINT = GL_UNSIGNED_INT,  /*< GL_UNSIGNED_INT */
    DT_INT = GL_INT, /*< GL_INT */
	DT_DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
	DT_DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F_NV,
#endif
};


enum TEXFILTER {
    TF_NEAREST = GL_NEAREST,        /*< GL_NEAREST */
    TF_LINEAR = GL_LINEAR,          /*< GL_LINEAR */
};

enum TEXTARGET {
#ifndef OPENGLES
    TT_TEXTURE_1D = GL_TEXTURE_1D,  /*< R2_TEXTURE_1D */
#endif
    TT_TEXTURE_2D = GL_TEXTURE_2D,  /*< GL_TEXTURE_2D */
#ifndef OPENGLES
    TT_TEXTURE_RECTANGLE = GL_TEXTURE_RECTANGLE_NV, /*< GL_TEXTURE_RECTANGLE_NV */
    TT_TEXTURE_3D = GL_TEXTURE_3D,  /*< GL_TEXTURE_3D */
#endif

};

enum TEXWRAP {
#ifndef OPENGLES
    TW_CLAMP = GL_CLAMP,                    /*< GL_CLAMP */
#endif
    TW_CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,    /*< GL_CLAMP_TO_EDGE */
    TW_REPEAT = GL_REPEAT,                  /*< GL_REPEAT */
#ifndef OPENGLES
    TW_MIRROR = GL_MIRRORED_REPEAT,                  /*< GL_MIRROR */
#endif
};


