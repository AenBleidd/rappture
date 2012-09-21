/*
 * ----------------------------------------------------------------------
 *  Rappture::rlimit
 *
 *  This is an interface to the system getrlimit() and setrlimit()
 *  routines.  It allows you to get/set resource limits for child
 *  executables.  We use this in Rappture::exec, for example, to make
 *  sure that jobs don't run forever or fill up the disk.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#else
#include "RpWinResource.h"
#endif

#include <errno.h>
#include "RpOp.h"

static Tcl_ObjCmdProc RpRlimitCmd;
static Tcl_ObjCmdProc RpRlimitGetOp;
static Tcl_ObjCmdProc RpRlimitSetOp;
static int RpRlimitOptionError(Tcl_Interp *interp, const char *string);

/*
 * rlimit subcommands:
 */
static Rp_OpSpec rlimitOps[] = {
    {"get", 1, RpRlimitGetOp, 2, 4, "?-soft|-hard? ?option?",},
    {"set", 1, RpRlimitSetOp, 2, 0, "?option value ...?",},
};
static int nRlimitOps = sizeof(rlimitOps) / sizeof(Rp_OpSpec);

/*
 * rlimit configuration options:
 */
typedef struct RpRlimitNames {
    char *name;
    int resid;
} RpRlimitNames;

static RpRlimitNames rlimitOptions[] = {
    {"coresize",  RLIMIT_CORE},
    {"cputime",   RLIMIT_CPU},
    {"filesize",  RLIMIT_FSIZE},
    {(char*)NULL, 0}
};

/*
 * ------------------------------------------------------------------------
 *  RpRlimit_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpRlimit_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    Tcl_CreateObjCommand(interp, "::Rappture::rlimit", RpRlimitCmd, NULL, NULL);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRlimitCmd()
 *
 *  Invoked whenever someone uses the "rlimit" command to get/set
 *  limits for child processes.  Handles the following syntax:
 *
 *      rlimit get ?-soft|-hard? ?<option>?
 *      rlimit set ?<option> <value> <option> <value> ...?
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRlimitCmd(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    Tcl_ObjCmdProc *proc;

    proc = Rp_GetOpFromObj(interp, nRlimitOps, rlimitOps, RP_OP_ARG1,
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(cdata, interp, objc, objv);
}

/*
 * ------------------------------------------------------------------------
 *  RpRlimitGetOp()
 *
 *  Invoked whenever someone uses the "rlimit get" command to query
 *  limits for child processes.  Handles the following syntax:
 *
 *      rlimit get ?-soft|-hard? ?<option>?
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRlimitGetOp(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    int hardlim = 0;  /* default: use -soft limit */
    int nextarg = 2;  /* start with this arg */
    Tcl_Obj *objPtr;
    int i, status;
    struct rlimit rvals;
    rlim_t *rvalptr;

    if (nextarg < objc) {
        const char *string;

        string = Tcl_GetString(objv[nextarg]);
        if (string[0] == '-') {
            if (strcmp(string,"-soft") == 0) {
                hardlim = 0;
                nextarg++;
            } else if (strcmp(string,"-hard") == 0) {
                hardlim = 1;
                nextarg++;
            } else {
                Tcl_AppendResult(interp, "bad option \"", string,
                "\": should be -soft or -hard", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    /*
     * No args?  Then return limits for all options.
     */
    if (nextarg >= objc) {
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, NULL);
        for (i=0; rlimitOptions[i].name != NULL; i++) {
            Tcl_Obj *objPtr;
            status = getrlimit(rlimitOptions[i].resid, &rvals);
            if (status != 0) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unexpected rlimit failure",
                    (char*)NULL);
                return TCL_ERROR;
            }
            if (hardlim) {
                rvalptr = &rvals.rlim_max;
            } else {
                rvalptr = &rvals.rlim_cur;
            }
            objPtr = Tcl_NewStringObj(rlimitOptions[i].name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            if (*rvalptr == RLIM_INFINITY) {
                objPtr = Tcl_NewStringObj("unlimited", -1);
            } else {
                objPtr = Tcl_NewLongObj((unsigned long)*rvalptr);
            }
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }

    /*
     * Find the limit for the specified option.
     */
    for (i=0; rlimitOptions[i].name != NULL; i++) {
        const char *string;

        string = Tcl_GetString(objv[nextarg]);
        if (strcmp(string, rlimitOptions[i].name) == 0) {
            break;
        }
    }
    if (rlimitOptions[i].name == NULL) {
        const char *string;

        string = Tcl_GetString(objv[nextarg]);
        return RpRlimitOptionError(interp, string);
    }

    status = getrlimit(rlimitOptions[i].resid, &rvals);
    if (status != 0) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unexpected rlimit failure",
            (char*)NULL);
        return TCL_ERROR;
    }
    if (hardlim) {
        rvalptr = &rvals.rlim_max;
    } else {
        rvalptr = &rvals.rlim_cur;
    }

    if (*rvalptr == RLIM_INFINITY) {
        objPtr = Tcl_NewStringObj("unlimited", -1);
    } else {
        objPtr = Tcl_NewLongObj((unsigned long)*rvalptr);
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRlimitSetOp()
 *
 *  Invoked whenever someone uses the "rlimit set" command to query
 *  limits for child processes.  Handles the following syntax:
 *
 *      rlimit set ?<option> <value> <option> <value> ...?
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRlimitSetOp(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;   /* strings for command line args */
{
    int i, n, lim, status;
    struct rlimit rvals;
    rlim_t newval;

    for (n=2; n < objc; n += 2) {
        const char *name, *value;

        name = Tcl_GetString(objv[n]);
        if (n+1 >= objc) {
            Tcl_AppendResult(interp, "missing value for option \"",
                name, "\"", (char*)NULL);
            return TCL_ERROR;
        }
        value = Tcl_GetString(objv[n+1]);
        if (strcmp(value, "unlimited") == 0) {
            newval = RLIM_INFINITY;
        } else if (Tcl_GetIntFromObj(interp, objv[n+1], &lim) == TCL_OK) {
            if (lim < 0) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "bad value \"", value,
                    "\": should be integer >= 0 or \"unlimited\"",
                    (char*)NULL);
                return TCL_ERROR;
            }
            newval = (rlim_t)lim;
        } else {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "bad value \"", value,
                "\": should be integer >= 0 or \"unlimited\"", (char*)NULL);
            return TCL_ERROR;
        }

        /*
         * Find the option being changed and set it.
         */
        for (i=0; rlimitOptions[i].name != NULL; i++) {
            if (strcmp(name, rlimitOptions[i].name) == 0) {
                break;
            }
        }
        if (rlimitOptions[i].name == NULL) {
            return RpRlimitOptionError(interp, name);
        }
        status = getrlimit(rlimitOptions[i].resid, &rvals);
        if (status == 0) {
            rvals.rlim_cur = newval;
            status = setrlimit(rlimitOptions[i].resid, &rvals);
        }
        if (status == EPERM) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "value \"", value,
                "\" set too high for option \"", name,
                (char*)NULL);
            return TCL_ERROR;
        } else if (status != 0) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "unexpected rlimit failure",
                (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRlimitOptionError()
 *
 *  Used internally to set an error message in the interpreter
 *  describing an unrecognized rlimit option.  Sets the result of
 *  the Tcl interpreter and returns an error status code.
 * ------------------------------------------------------------------------
 */
static int
RpRlimitOptionError(interp, option)
    Tcl_Interp *interp;       /* interpreter handling this request */
    const char *option;       /* bad option supplied by the user */
{
    int i;

    Tcl_AppendResult(interp, "bad option \"", option,
        "\": should be one of ",(char*)NULL);

    for (i=0; rlimitOptions[i].name != NULL; i++) {
        if (i > 0) {
            Tcl_AppendResult(interp, ", ", (char*)NULL);
        }
        Tcl_AppendResult(interp, rlimitOptions[i].name, (char*)NULL);
    }
    return TCL_ERROR;
}
