/*
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


//system wide global or static functions

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include <stdio.h>

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


inline void 
draw_quad(int w, int h, int tw, int th)
{
    glBegin(GL_QUADS);
    glTexCoord2f(0,         0);         glVertex2f(0,        0);
    glTexCoord2f((float)tw, 0);         glVertex2f((float)w, 0);
    glTexCoord2f((float)tw, (float)th); glVertex2f((float)w, (float) h);
    glTexCoord2f(0,         (float)th); glVertex2f(0,        (float) h);
    glEnd();
}


//query opengl information
inline void 
system_info()
{
    fprintf(stderr, 
	    "-----------------------------------------------------------\n");
    fprintf(stderr, "OpenGL driver: %s %s\n", glGetString(GL_VENDOR), 
	    glGetString(GL_VERSION));
    fprintf(stderr, "Graphics hardware: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, 
	    "-----------------------------------------------------------\n");
}

extern CGprogram LoadCgSourceProgram(CGcontext context, const char *filename, 
	CGprofile profile, const char *entryPoint);

#endif
