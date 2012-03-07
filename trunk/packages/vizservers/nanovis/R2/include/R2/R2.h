/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef R2_H
#define R2_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <string.h>


typedef bool            R2bool;

typedef float           R2float;
typedef double R2double;
typedef char            R2int8;
typedef char            R2char;
typedef unsigned char   R2uint8;
typedef short           R2int16;
typedef unsigned short  R2uint16;
typedef int             R2int32;
typedef unsigned int    R2uint32;
typedef void            R2void;

#ifndef PI
#   define PI 3.14159265358979323846f
#endif

enum R2COLORFORMAT {
    R2_LUMINANCE = GL_LUMINANCE, /*< GL_LIMINANCE */
    R2_RGB8 = GL_RGB8, /*< GL_RGB8 */
    R2_RGB = GL_RGB,            /*< GL_UNSIGNED_BYTE */
    R2_RGBA = GL_RGBA,          /*< GL_FLOAT */
    R2_FLOAT_RGBA32 = GL_FLOAT_RGBA32_NV
};

enum R2DATATYPE {
    R2_UBYTE = GL_UNSIGNED_BYTE,    /*< GL_UNSIGNED_BYTE */
    R2_FLOAT = GL_FLOAT,            /*< GL_FLOAT */
    R2_UINT = GL_UNSIGNED_INT,  /*< GL_UNSIGNED_INT */
    R2_INT = GL_INT /*< GL_INT */
};

enum R2TEXFILTER {
    R2_NEAREST = GL_NEAREST,        /*< GL_NEAREST */
    R2_LINEAR = GL_LINEAR           /*< GL_LINEAR */
};

enum R2TEXTARGET {
    R2_TEXTURE_1D = GL_TEXTURE_1D,  /*< R2_TEXTURE_1D */
    R2_TEXTURE_2D = GL_TEXTURE_2D,  /*< GL_TEXTURE_2D */
    R2_TEXTURE_RECTANGLE = GL_TEXTURE_RECTANGLE_NV, /*< GL_TEXTURE_RECTANGLE_NV */
    R2_TEXTURE_3D = GL_TEXTURE_3D  /*< GL_TEXTURE_3D */

};
enum R2TEXWRAP {
    R2_CLAMP = GL_CLAMP,                    /*< GL_CLAMP */
    R2_CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,    /*< GL_CLAMP_TO_EDGE */
    R2_REPEAT = GL_REPEAT                   /*< GL_REPEAT */
};

#define R2Assert(s) 

extern void R2Init();
extern void R2Exit();

#endif

