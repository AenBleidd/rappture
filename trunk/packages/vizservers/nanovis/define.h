/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#define CHECK_FRAMEBUFFER_STATUS() \
{ \
 GLenum status;  \
 status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);  \
 TRACE("%x\n", status); \
 switch(status) {  \
 case GL_FRAMEBUFFER_COMPLETE_EXT:  \
   TRACE("framebuffer complete!\n"); \
   break;  \
 case GL_FRAMEBUFFER_UNSUPPORTED_EXT:  \
   TRACE("framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n"); \
    /* you gotta choose different formats */  \
   assert(0);  \
   break;  \
 case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:  \
   TRACE("framebuffer INCOMPLETE_ATTACHMENT\n"); \
   break;  \
 case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:  \
   TRACE("framebuffer FRAMEBUFFER_MISSING_ATTACHMENT\n"); \
   break;  \
 case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:  \
   TRACE("framebuffer FRAMEBUFFER_DIMENSIONS\n");\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: \
   TRACE("framebuffer INCOMPLETE_FORMATS\n");\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT: \
   TRACE("framebuffer INCOMPLETE_DRAW_BUFFER\n");\
   break; \
 case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT: \
   TRACE("framebuffer INCOMPLETE_READ_BUFFER\n");\
   break; \
 case GL_FRAMEBUFFER_BINDING_EXT: \
   TRACE("framebuffer BINDING_EXT\n");\
   break; \
/* 
 *  case GL_FRAMEBUFFER_STATUS_ERROR_EXT: \
 *     TRACE("framebuffer STATUS_ERROR\n");\
 *        break; \
 *        */ \
 default: \
   ERROR("unknown framebuffer error %d\n", status);\
   /* programming error; will fail on all hardware */ \
   assert(0); \
 }\
}

#ifdef notdef


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
	ERROR(stderr, "programming error\n");               \
        assert(0);                                            \
     }	                                                      \
   }
#endif

// use this to send debug messages back to the client
void debug(char *message);
void debug(char *message, double v1);
void debug(char *message, double v1, double v2);
void debug(char *message, double v1, double v2, double v3);

#define __NANOVIS_DEBUG__ 0
//#define __NANOVIS_DEBUG__ 1

#endif
