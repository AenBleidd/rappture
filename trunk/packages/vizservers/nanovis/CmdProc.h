/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *-------------------------------------------------------------------------------
 *
 * CmdSpec.h --
 *
 * 	Generic function prototype of CmdOptions.
 *
 *-------------------------------------------------------------------------------
 */

#ifndef CMDSPEC_H
#define CMDSPEC_H

namespace Rappture {

/*
 *-------------------------------------------------------------------------------
 *
 * CmdSpec --
 *
 * 	Structure to specify a set of operations for a Tcl command.
 *      This is passed to the Blt_GetOp procedure to look
 *      for a function pointer associated with the operation name.
 *
 *-------------------------------------------------------------------------------
 */
typedef struct {
    const char *name;		/* Name of operation */
    int minChars;		/* Minimum # characters to disambiguate */
    Tcl_ObjCmdProc *proc;
    int minArgs;		/* Minimum # args required */
    int maxArgs;		/* Maximum # args required */
    const char *usage;		/* Usage message */
} CmdSpec;

typedef enum {
    CMDSPEC_ARG0,		/* Op is the first argument. */
    CMDSPEC_ARG1,		/* Op is the second argument. */
    CMDSPEC_ARG2,		/* Op is the third argument. */
    CMDSPEC_ARG3,		/* Op is the fourth argument. */
    CMDSPEC_ARG4		/* Op is the fifth argument. */
} CmdSpecIndex;

#define CMDSPEC_LINEAR_SEARCH	1
#define CMDSPEC_BINARY_SEARCH	0

extern Tcl_ObjCmdProc *GetOpFromObj(Tcl_Interp *interp, int nSpecs, 
	CmdSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, int flags);

#define NumCmdSpecs(s) (sizeof(s) / sizeof(Rappture::CmdSpec))

}


#endif /* CMDSPEC_H */
