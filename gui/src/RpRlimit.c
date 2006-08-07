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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#else
#include "RpWinResource.h"
#endif

#include "bltInt.h"

static int RpRlimitCmd _ANSI_ARGS_((ClientData cdata,
    Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int RpRlimitGetOp _ANSI_ARGS_((ClientData cdata,
    Tcl_Interp *interp, int argc, char *argv[]));
static int RpRlimitSetOp _ANSI_ARGS_((ClientData cdata,
    Tcl_Interp *interp, int argc, char *argv[]));
static int RpRlimitOptionError _ANSI_ARGS_((Tcl_Interp *interp,
    char *option));

/*
 * rlimit subcommands:
 */
static Blt_OpSpec rlimitOps[] = {
    {"get", 1, (Blt_Op)RpRlimitGetOp, 2, 4, "?-soft|-hard? ?option?",},
    {"set", 1, (Blt_Op)RpRlimitSetOp, 2, 0, "?option value ...?",},
};
static int nRlimitOps = sizeof(rlimitOps) / sizeof(Blt_OpSpec);

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
 *  Called in RapptureGUI_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpRlimit_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    Tcl_CreateCommand(interp, "::Rappture::rlimit",
        RpRlimitCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

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
RpRlimitCmd(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    CONST84 char *argv[];     /* strings for command line args */
{
    Blt_Op proc;

    proc = Blt_GetOp(interp, nRlimitOps, rlimitOps, BLT_OP_ARG1,
        argc, (char**)argv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(cdata, interp, argc, argv);
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
RpRlimitGetOp(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    char *argv[];             /* strings for command line args */
{
    int hardlim = 0;  /* default: use -soft limit */
    int nextarg = 2;  /* start with this arg */

    int i, status;
    struct rlimit rvals;
    rlim_t *rvalptr;
    char buffer[64];

    if (nextarg < argc && *argv[nextarg] == '-') {
        if (strcmp(argv[nextarg],"-soft") == 0) {
            hardlim = 0;
            nextarg++;
        }
        else if (strcmp(argv[nextarg],"-hard") == 0) {
            hardlim = 1;
            nextarg++;
        }
        else {
            Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                "\": should be -soft or -hard", (char*)NULL);
            return TCL_ERROR;
        }
    }

    /*
     * No args?  Then return limits for all options.
     */
    if (nextarg >= argc) {
        for (i=0; rlimitOptions[i].name != NULL; i++) {
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

            Tcl_AppendElement(interp, rlimitOptions[i].name);
            if (*rvalptr == RLIM_INFINITY) {
                Tcl_AppendElement(interp, "unlimited");
            } else {
                sprintf(buffer, "%lu", (unsigned long)*rvalptr);
                Tcl_AppendElement(interp, buffer);
            }
        }
        return TCL_OK;
    }

    /*
     * Find the limit for the specified option.
     */
    for (i=0; rlimitOptions[i].name != NULL; i++) {
        if (strcmp(argv[nextarg], rlimitOptions[i].name) == 0) {
            break;
        }
    }
    if (rlimitOptions[i].name == NULL) {
        return RpRlimitOptionError(interp, argv[nextarg]);
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
        Tcl_SetResult(interp, "unlimited", TCL_STATIC);
    } else {
        sprintf(buffer, "%lu", (unsigned long)*rvalptr);
        Tcl_SetResult(interp, buffer, TCL_VOLATILE);
    }
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
RpRlimitSetOp(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    char *argv[];             /* strings for command line args */
{
    int i, n, lim, status;
    struct rlimit rvals;
    rlim_t newval;

    for (n=2; n < argc; n += 2) {
        if (n+1 >= argc) {
            Tcl_AppendResult(interp, "missing value for option \"",
                argv[n], "\"", (char*)NULL);
            return TCL_ERROR;
        }
        if (strcmp(argv[n+1],"unlimited") == 0) {
            newval = RLIM_INFINITY;
        }
        else if (Tcl_GetInt(interp, argv[n+1], &lim) == TCL_OK) {
            if (lim < 0) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "bad value \"", argv[n+1],
                    "\": should be integer >= 0 or \"unlimited\"",
                    (char*)NULL);
                return TCL_ERROR;
            }
            newval = (rlim_t)lim;
        }
        else {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "bad value \"", argv[n+1],
                "\": should be integer >= 0 or \"unlimited\"", (char*)NULL);
            return TCL_ERROR;
        }

        /*
         * Find the option being changed and set it.
         */
        for (i=0; rlimitOptions[i].name != NULL; i++) {
            if (strcmp(argv[n], rlimitOptions[i].name) == 0) {
                break;
            }
        }
        if (rlimitOptions[i].name == NULL) {
            return RpRlimitOptionError(interp, argv[n]);
        }
        status = getrlimit(rlimitOptions[i].resid, &rvals);
        if (status == 0) {
            rvals.rlim_cur = newval;
            status = setrlimit(rlimitOptions[i].resid, &rvals);
        }

        if (status == EPERM) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "value \"", argv[n+1],
                "\" set too high for option \"", argv[n],
                (char*)NULL);
            return TCL_ERROR;
        }
        else if (status != 0) {
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
    char *option;             /* bad option supplied by the user */
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
