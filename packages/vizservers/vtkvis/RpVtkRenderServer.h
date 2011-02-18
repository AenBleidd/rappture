/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERSERVER_H__
#define __RAPPTURE_VTKVIS_RENDERSERVER_H__

#include "RpVtkRenderer.h"

namespace Rappture {
namespace VtkVis {

extern int g_fdIn;
extern int g_fdOut;
extern FILE *g_fIn;
extern FILE *g_fOut;
extern FILE *g_fLog;
extern Renderer *g_renderer;

}
}

#endif
