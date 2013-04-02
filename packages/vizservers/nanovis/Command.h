/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef COMMAND_H
#define COMMAND_H

#include <tcl.h>

#include <RpBuffer.h>

#include "ResponseQueue.h"

namespace Rappture {
class Buffer;
}
class Volume;

namespace nv {
class ReadBuffer;

#ifdef USE_THREADS
extern void queueResponse(const void *bytes, size_t len, 
                          Response::AllocationType allocType,
                          Response::ResponseType type = Response::DATA);
#else
extern ssize_t SocketWrite(const void *bytes, size_t len);
#endif

extern bool SocketRead(char *bytes, size_t len);

extern bool SocketRead(Rappture::Buffer &buf, size_t len);

extern int processCommands(Tcl_Interp *interp,
                           ReadBuffer *inBufPtr,
                           int fdOut);

extern int handleError(Tcl_Interp *interp,
                       int status,
                       int fdOut);

extern void initTcl(Tcl_Interp *interp, ClientData clientData);
}

extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          int *axisVal);

extern bool GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                             bool *boolVal);

extern int GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes);

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           float *floatVal);

extern int GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                            Volume **volume);

#endif
