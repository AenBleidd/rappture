/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_RENDERERCMD_H__
#define __RAPPTURE_VTKVIS_RENDERERCMD_H__

#include <cstdio>
#include <tcl.h>

namespace Rappture {
namespace VtkVis {

extern int processCommands(Tcl_Interp *interp, FILE *fin, FILE *fout);
extern Tcl_Interp *initTcl();
extern void exitTcl(Tcl_Interp *interp);

}
}

#endif
