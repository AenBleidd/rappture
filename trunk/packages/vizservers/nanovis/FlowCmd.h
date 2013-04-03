/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   Michael McLennan <mmclennan@purdue.edu>
 */
#ifndef NV_FLOWCMD_H
#define NV_FLOWCMD_H

#include <tcl.h>

namespace nv {

extern void FlowCmdInitProc(Tcl_Interp *interp, ClientData clientData);

extern Tcl_ObjCmdProc FlowInstObjCmd;
extern Tcl_CmdDeleteProc FlowInstDeleteProc;

}

#endif
