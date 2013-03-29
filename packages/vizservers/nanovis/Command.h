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

namespace Rappture {
class Buffer;
}
class Volume;

extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          int *axisVal);

extern int GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                             bool *boolVal);

extern int GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes);

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           float *floatVal);

extern int GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                            Volume **volume);

extern Tcl_Interp *initTcl();

#endif
