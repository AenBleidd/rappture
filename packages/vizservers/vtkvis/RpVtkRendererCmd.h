/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_RENDERERCMD_H
#define VTKVIS_RENDERERCMD_H

#include <cstdio>
#include <tcl.h>

#include "ReadBuffer.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

namespace VtkVis {

#ifdef USE_THREADS
extern void queueResponse(ClientData clientData, const void *bytes, size_t len, 
                          Response::AllocationType allocType,
                          Response::ResponseType type = Response::DATA);
#endif

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

#endif
