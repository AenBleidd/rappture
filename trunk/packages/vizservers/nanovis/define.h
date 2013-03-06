/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef DEFINE_H
#define DEFINE_H

#include <GL/glew.h>

#define CHECK_FRAMEBUFFER_STATUS()                              \
do {                                                            \
    GLenum status;                                              \
    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);   \
    switch(status) {                                            \
    case GL_FRAMEBUFFER_COMPLETE_EXT:                           \
        TRACE("Framebuffer complete");                          \
        break;                                                  \
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                        \
        ERROR("Framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT");    \
        assert(0);                                              \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:              \
        TRACE("Framebuffer INCOMPLETE_ATTACHMENT");             \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:      \
        TRACE("Framebuffer FRAMEBUFFER_MISSING_ATTACHMENT");    \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:              \
        TRACE("Framebuffer FRAMEBUFFER_DIMENSIONS");            \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:                 \
        TRACE("Framebuffer INCOMPLETE_FORMATS");                \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:             \
        TRACE("Framebuffer INCOMPLETE_DRAW_BUFFER");            \
        break;                                                  \
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:             \
        TRACE("Framebuffer INCOMPLETE_READ_BUFFER");            \
        break;                                                  \
    case GL_FRAMEBUFFER_BINDING_EXT:                            \
        TRACE("Framebuffer BINDING_EXT");                       \
        break;                                                  \
    default:                                                    \
        ERROR("Unknown framebuffer error %d", status);          \
        /* programming error; will fail on all hardware */      \
        assert(0);                                              \
    }                                                           \
} while(0)

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

#endif
