/*
 * ----------------------------------------------------------------------
 *  Rappture::rusage
 *
 *  This is an interface to the system getrusage() routine.  It allows
 *  you to query resource used by child executables.  We use this in
 *  Rappture to track the usage during each click of the "Simulate"
 *  button.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#include <sys/resource.h>
#else
#include "RpWinResource.h"
#endif

#include "RpOp.h"

/*
 * Store rusage info in this data structure:
 */
typedef struct RpRusageStats {
    struct timeval times;
    struct rusage resources;
} RpRusageStats;

static RpRusageStats RpRusage_Start;      /* time at start of program */
static RpRusageStats RpRusage_MarkStats;  /* stats from last "mark" */


static Tcl_ObjCmdProc RpRusageCmd;
static Tcl_ObjCmdProc RpRusageMarkOp;
static Tcl_ObjCmdProc RpRusageMeasureOp;

static int RpRusageCapture _ANSI_ARGS_((Tcl_Interp *interp,
    RpRusageStats *rptr));
static double RpRusageTimeDiff _ANSI_ARGS_((struct timeval *currptr,
    struct timeval *prevptr));

/*
 * rusage subcommands:
 */
static Rp_OpSpec rusageOps[] = {
    {"mark",    2, RpRusageMarkOp, 2, 2, "",},
    {"measure", 2, RpRusageMeasureOp, 2, 2, "",},
};
static int nRusageOps = sizeof(rusageOps) / sizeof(Rp_OpSpec);

/*
 * ------------------------------------------------------------------------
 *  RpRusage_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpRusage_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    Tcl_CreateObjCommand(interp, "::Rappture::rusage", RpRusageCmd, 
	NULL, NULL);

    /* set an initial mark automatically */
    if (RpRusageMarkOp(NULL, interp, 0, (Tcl_Obj**)NULL) != TCL_OK) {
        return TCL_ERROR;
    }

    /* capture the starting time for this program */
    memcpy(&RpRusage_Start, &RpRusage_MarkStats, sizeof(RpRusageStats));

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageCmd()
 *
 *  Invoked whenever someone uses the "rusage" command to get/set
 *  limits for child processes.  Handles the following syntax:
 *
 *      rusage mark
 *      rusage measure
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRusageCmd(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    Tcl_ObjCmdProc *proc;

    proc = Rp_GetOpFromObj(interp, nRusageOps, rusageOps, RP_OP_ARG1,
        objc, objv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(cdata, interp, objc, objv);
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageMarkOp()
 *
 *  Invoked whenever someone uses the "rusage mark" command to mark
 *  the start of an execution.  Handles the following syntax:
 *
 *      rusage mark
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRusageMarkOp(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;             /* strings for command line args */
{
    return RpRusageCapture(interp, &RpRusage_MarkStats);
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageMeasureOp()
 *
 *  Invoked whenever someone uses the "rusage measure" command to
 *  measure resource usage since the last "mark" operation.  Handles
 *  the following syntax:
 *
 *      rusage measure
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRusageMeasureOp(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    double tval;
    RpRusageStats curstats;
    char buffer[TCL_DOUBLE_SPACE];

    if (RpRusageCapture(interp, &curstats) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Compute: START TIME
     */
    Tcl_AppendElement(interp, "start");
    tval = RpRusageTimeDiff(&RpRusage_MarkStats.times, &RpRusage_Start.times);
    Tcl_PrintDouble(interp, tval, buffer);
    Tcl_AppendElement(interp, buffer);

    /*
     * Compute: WALL TIME
     */
    Tcl_AppendElement(interp, "walltime");
    tval = RpRusageTimeDiff(&curstats.times, &RpRusage_MarkStats.times);
    Tcl_PrintDouble(interp, tval, buffer);
    Tcl_AppendElement(interp, buffer);

    /*
     * Compute: CPU TIME = user time + system time
     */
    Tcl_AppendElement(interp, "cputime");
    tval = RpRusageTimeDiff(&curstats.resources.ru_utime,
             &RpRusage_MarkStats.resources.ru_utime)
         + RpRusageTimeDiff(&curstats.resources.ru_stime,
             &RpRusage_MarkStats.resources.ru_stime);
    Tcl_PrintDouble(interp, tval, buffer);
    Tcl_AppendElement(interp, buffer);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageCapture()
 *
 *  Used internally to capture a snapshot of current time and resource
 *  usage.  Stores the stats in the given data structure.
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRusageCapture(interp, rptr)
    Tcl_Interp *interp;       /* interpreter handling this request */
    RpRusageStats *rptr;      /* returns: snapshot of stats */
{
    int status;

    status = getrusage(RUSAGE_CHILDREN, &rptr->resources);
    if (status != 0) {
        Tcl_AppendResult(interp, "unexpected error from getrusage()",
            (char*)NULL);
        return TCL_ERROR;
    }

    status = gettimeofday(&rptr->times, (struct timezone*)NULL);
    if (status != 0) {
        Tcl_AppendResult(interp, "unexpected error from gettimeofday()",
            (char*)NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageTimeDiff()
 *
 *  Used internally to compute the difference between two timeval
 *  structures.  Returns a double precision value representing the
 *  time difference.
 * ------------------------------------------------------------------------
 */
static double
RpRusageTimeDiff(currptr, prevptr)
    struct timeval *currptr;  /* current time */
    struct timeval *prevptr;  /*  - previous time */
{
    double tval;

    if (prevptr) {
        tval = (currptr->tv_sec - prevptr->tv_sec)
                 + 1.0e-6*(currptr->tv_usec - prevptr->tv_usec);
    } else {
        tval = currptr->tv_sec + 1.0e-6*currptr->tv_usec;
    }
    return tval;
}

