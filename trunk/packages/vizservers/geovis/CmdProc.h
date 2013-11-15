/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef GEOVIS_CMDPROC_H
#define GEOVIS_CMDPROC_H

#include <tcl.h>

namespace GeoVis {

/**
 * \brief Structure to specify a set of operations for a Tcl command.
 *
 * This is passed to the GetOpFromObj procedure to look
 * for a function pointer associated with the operation name.
 */
struct CmdSpec {
    const char *name;		/**< Name of operation */
    int minChars;		/**< Minimum # characters to disambiguate */
    Tcl_ObjCmdProc *proc;
    int minArgs;		/**< Minimum # args required */
    int maxArgs;		/**< Maximum # args required */
    const char *usage;		/**< Usage message */
};

enum CmdSpecIndex {
    CMDSPEC_ARG0,		/**< Op is the first argument. */
    CMDSPEC_ARG1,		/**< Op is the second argument. */
    CMDSPEC_ARG2,		/**< Op is the third argument. */
    CMDSPEC_ARG3,		/**< Op is the fourth argument. */
    CMDSPEC_ARG4		/**< Op is the fifth argument. */
};

#define CMDSPEC_LINEAR_SEARCH	1
#define CMDSPEC_BINARY_SEARCH	0

extern Tcl_ObjCmdProc *
GetOpFromObj(Tcl_Interp *interp, int nSpecs, 
             CmdSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, int flags);

#define NumCmdSpecs(s) (sizeof(s) / sizeof(GeoVis::CmdSpec))

}

#endif
