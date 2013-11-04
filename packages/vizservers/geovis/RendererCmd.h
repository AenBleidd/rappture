/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_RENDERERCMD_H
#define GEOVIS_RENDERERCMD_H

#include <cstdio>
#include <tcl.h>

#include "ReadBuffer.h"
#include "ResponseQueue.h"

namespace GeoVis {

extern ssize_t queueResponse(const void *bytes, size_t len, 
                             Response::AllocationType allocType,
                             Response::ResponseType type = Response::DATA);

#ifdef USE_READ_THREAD
extern int queueCommands(Tcl_Interp *interp,
                         ClientData clientData,
                         ReadBuffer *inBufPtr);
#endif

extern int processCommands(Tcl_Interp *interp,
                           ClientData clientData,
                           ReadBuffer *inBufPtr,
                           int fdOut,
                           long timeout = -1);

extern int handleError(Tcl_Interp *interp,
                       ClientData clientData,
                       int status,
                       int fdOut);

extern void initTcl(Tcl_Interp *interp,
                    ClientData clientData);

extern void exitTcl(Tcl_Interp *interp);

}

#endif
