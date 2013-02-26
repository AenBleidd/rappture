/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERERCMD_H__
#define __RAPPTURE_VTKVIS_RENDERERCMD_H__

#include <cstdio>
#include <tcl.h>

#include "ReadBuffer.h"

namespace Rappture {
namespace VtkVis {

extern int processCommands(Tcl_Interp *interp,
                           ClientData clientData,
                           ReadBuffer *inBufPtr,
                           int fdOut);

extern int handleError(Tcl_Interp *interp,
                       ClientData clientData,
                       int status,
                       int fdOut);

extern void initTcl(Tcl_Interp *interp,
                    ClientData clientData);

extern void exitTcl(Tcl_Interp *interp);

}
}

#endif
