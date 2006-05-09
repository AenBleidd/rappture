/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


//The NanoVis system wide defines
//Here we try to hide OpenGL native definitions as much as possible


#ifndef _DEFINE_H_
#define _DEFINE_H_

#include <GL/glew.h>
#include <Cg/cgGL.h>

#define NVIS_FLOAT GL_FLOAT
#define NVIS_UNSIGNED_INT GL_UNSIGNED_INT
#define NVIS_UNSIGNED_BYTE GL_UNSIGNED_BYTE

#define NVIS_LINEAR_INTERP GL_LINEAR
#define NVIS_NEAREST_INTERP GL_NEAREST

typedef GLuint NVISdatatype;		//OpenGL datatype: unsigned int
typedef GLuint NVISinterptype;		//OpenGL interpolation type: unsigned int
typedef GLuint NVISid;			//OpenGL identifier: unsigned int


#define CHECK_FRAMEBUFFER_STATUS()                            \
  {                                                           \
    GLenum status;                                            \
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); \
    switch(status) {                                          \
      case GL_FRAMEBUFFER_COMPLETE_EXT:                       \
        break;                                                \
      case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    \
        /* choose different formats */                        \
        break;                                                \
      default:                                                \
        /* programming error; will fail on all hardware */    \
	fprintf(stderr, "programming error\n");               \
        assert(0);                                            \
     }	                                                      \
   }

// use this to send debug messages back to the client
void debug(char *message);
void debug(char *message, double v1);
void debug(char *message, double v1, double v2);
void debug(char *message, double v1, double v2, double v3);

#endif
