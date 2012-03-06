/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#include <Trace.h>
#include <stdio.h>

#ifdef notdef
#define CHECK_FRAMEBUFFER_STATUS() \
{\
 GLenum status; \
 status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); \
 TRACE("%x\n", status);			   \
 switch(status) { \
 case GL_FRAMEBUFFER_COMPLETE_EXT: \
     TRACE("framebuffer complete!\n");	\
   break; \
 case GL_FRAMEBUFFER_UNSUPPORTED_EXT: \
     ERROR("framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");	\
    /* you gotta choose different formats */ \
   assert(0); \
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:		\
     TRACE("framebuffer INCOMPLETE_ATTACHMENT\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: \
     TRACE("framebuffer FRAMEBUFFER_MISSING_ATTACHMENT\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: \
     TRACE("framebuffer FRAMEBUFFER_DIMENSIONS\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT: \
     TRACE("framebuffer INCOMPLETE_DUPLICATE_ATTACHMENT\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: \
     TRACE("framebuffer INCOMPLETE_FORMATS\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT: \
     TRACE("framebuffer INCOMPLETE_DRAW_BUFFER\n");	\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT: \
     TRACE("framebuffer INCOMPLETE_READ_BUFFER\n");	\
   break; \
 case GL_FRAMEBUFFER_BINDING_EXT: \
     TRACE("framebuffer BINDING_EXT\n");	\
   break; \
/* 
 *  case GL_FRAMEBUFFER_STATUS_ERROR_EXT: \
 *     TRACE("framebuffer STATUS_ERROR\n");\
 *        break; \
 *        */ \
 default: \
   /* programming error; will fail on all hardware */ \
   assert(0); \
 }\
}

#define CHECK_FRAMEBUFFER_STATUS()                            \
  {                                                           \
    GLenum status;                                            \
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); \
    switch(status) {                                          \
      case GL_FRAMEBUFFER_COMPLETE_EXT:                       \
        break;                                                \
      case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    \
~        /* choose different formats */                        \
        break;                                                \
      default:                                                \
        /* programming error; will fail on all hardware */    \
	ERROR("programming error\n");               \
        assert(0);                                            \
     }	                                                      \
   }
#endif

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
    INFO("-----------------------------------------------------------\n");
    INFO("OpenGL driver: %s %s\n", glGetString(GL_VENDOR), 
	   glGetString(GL_VERSION));
    INFO("Graphics hardware: %s\n", glGetString(GL_RENDERER));
    INFO("-----------------------------------------------------------\n");
}

extern CGprogram LoadCgSourceProgram(CGcontext context, const char *filename, 
	CGprofile profile, const char *entryPoint);

#endif
