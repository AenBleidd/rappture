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
#ifndef GLOBAL_H
#define GLOBAL_H

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "Trace.h"

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
    TRACE("-----------------------------------------------------------\n");
    TRACE("OpenGL driver: %s %s\n", glGetString(GL_VENDOR), 
	   glGetString(GL_VERSION));
    TRACE("Graphics hardware: %s\n", glGetString(GL_RENDERER));
    TRACE("-----------------------------------------------------------\n");
}

extern CGprogram LoadCgSourceProgram(CGcontext context, const char *filename, 
                                     CGprofile profile, const char *entryPoint);

#endif
