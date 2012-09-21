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
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

static Tcl_ObjCmdProc RpRusageCmd;
static Tcl_ObjCmdProc RpRusageForgetOp;
static Tcl_ObjCmdProc RpRusageMarkOp;
static Tcl_ObjCmdProc RpRusageMeasureOp;

static int RpRusageCapture _ANSI_ARGS_((Tcl_Interp *interp,
    RpRusageStats *rptr));
static double RpRusageTimeDiff _ANSI_ARGS_((struct timeval *currptr,
    struct timeval *prevptr));
static void RpDestroyMarkNames _ANSI_ARGS_((ClientData cdata,
    Tcl_Interp *interp));

/*
 * rusage subcommands:
 */
static Rp_OpSpec rusageOps[] = {
    {"forget", 1, RpRusageForgetOp, 2, 0, "?name...?",},
    {"mark",    2, RpRusageMarkOp, 2, 3, "?name?",},
    {"measure", 2, RpRusageMeasureOp, 2, 3, "?name?",},
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
    Tcl_HashTable *markNamesPtr;

    Tcl_CreateObjCommand(interp, "::Rappture::rusage", RpRusageCmd, 
	NULL, NULL);

    /* set up a hash table for different mark names */
    markNamesPtr = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(markNamesPtr, TCL_STRING_KEYS);

    Tcl_SetAssocData(interp, "RpRusageMarks",
        RpDestroyMarkNames, (ClientData)markNamesPtr);

    /* capture the starting time for this program */
    if (RpRusageCapture(interp, &RpRusage_Start) != TCL_OK) {
        return TCL_ERROR;
    }

    /* set an initial "global" mark automatically */
    if (RpRusageMarkOp(NULL, interp, 0, (Tcl_Obj**)NULL) != TCL_OK) {
        return TCL_ERROR;
    }

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
 *      rusage mark ?name?
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
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    char *markName;
    Tcl_HashTable *markNamesPtr;
    Tcl_HashEntry *entryPtr;
    int newEntry;
    RpRusageStats *markPtr;

    markNamesPtr = (Tcl_HashTable *)
        Tcl_GetAssocData(interp, "RpRusageMarks", NULL);

    markName = (objc > 2) ? Tcl_GetString(objv[2]): "global";
    entryPtr = Tcl_CreateHashEntry(markNamesPtr, markName, &newEntry);
    if (newEntry) {
        markPtr = (RpRusageStats*)ckalloc(sizeof(RpRusageStats));
        Tcl_SetHashValue(entryPtr, (ClientData)markPtr);
    } else {
        markPtr = (RpRusageStats*)Tcl_GetHashValue(entryPtr);
    }

    return RpRusageCapture(interp, markPtr);
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageMeasureOp()
 *
 *  Invoked whenever someone uses the "rusage measure" command to
 *  measure resource usage since the last "mark" operation.  Handles
 *  the following syntax:
 *
 *      rusage measure ?name?
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
    char *markName;
    Tcl_HashTable *markNamesPtr;
    Tcl_HashEntry *entryPtr;
    RpRusageStats *markPtr;
    double tval;
    RpRusageStats curstats;
    Tcl_Obj *listObjPtr, *objPtr;

    if (RpRusageCapture(interp, &curstats) != TCL_OK) {
        return TCL_ERROR;
    }

    markNamesPtr = (Tcl_HashTable *)
        Tcl_GetAssocData(interp, "RpRusageMarks", NULL);

    markName = (objc > 2) ? Tcl_GetString(objv[2]): "global";
    entryPtr = Tcl_FindHashEntry(markNamesPtr, markName);
    if (entryPtr == NULL) {
        Tcl_AppendResult(interp, "mark \"", markName,
            "\" doesn't exist", NULL);
        return TCL_ERROR;
    }
    markPtr = (RpRusageStats*)Tcl_GetHashValue(entryPtr);

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    /*
     * Compute: START TIME
     */
    objPtr = Tcl_NewStringObj("start", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    tval = RpRusageTimeDiff(&markPtr->times, &RpRusage_Start.times);
    objPtr = Tcl_NewDoubleObj(tval);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    /*
     * Compute: WALL TIME
     */
    objPtr = Tcl_NewStringObj("walltime", 8);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    tval = RpRusageTimeDiff(&curstats.times, &markPtr->times);
    objPtr = Tcl_NewDoubleObj(tval);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);


    /*
     * Compute: CPU TIME = user time + system time
     */
    objPtr = Tcl_NewStringObj("cputime", 7);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    tval = RpRusageTimeDiff(&curstats.resources.ru_utime,
             &markPtr->resources.ru_utime)
         + RpRusageTimeDiff(&curstats.resources.ru_stime,
             &markPtr->resources.ru_stime);
    objPtr = Tcl_NewDoubleObj(tval);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpRusageForgetOp()
 *
 *  Invoked whenever someone uses the "rusage forget" command to release
 *  information previous set by "rusage mark".  With no args, it releases
 *  all known marks; otherwise, it releases just the specified mark
 *  names.  This isn't usually needed, but if a program creates thousands
 *  of marks, this gives a way to avoid a huge memory leak.
 *  Handles the following syntax:
 *
 *      rusage forget ?name name...?
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpRusageForgetOp(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    int i;
    char *markName;
    Tcl_HashTable *markNamesPtr;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;

    markNamesPtr = (Tcl_HashTable *)
        Tcl_GetAssocData(interp, "RpRusageMarks", NULL);

    /*
     * No args? Then clear all entries in the hash table.
     */
    if (objc == 2) {
        for (entryPtr = Tcl_FirstHashEntry(markNamesPtr, &search);
             entryPtr != NULL;
             entryPtr = Tcl_NextHashEntry(&search)) {
            ckfree((char *)Tcl_GetHashValue(entryPtr));
        }
        Tcl_DeleteHashTable(markNamesPtr);
        Tcl_InitHashTable(markNamesPtr, TCL_STRING_KEYS);
    }

    /*
     * Otherwise, delete only the specified marks.  Be forgiving.
     * If a mark isn't recognized, ignore it.
     */
    else {
        for (i=2; i < objc; i++) {
            markName = Tcl_GetString(objv[i]);
            entryPtr = Tcl_FindHashEntry(markNamesPtr, markName);
            if (entryPtr) {
                ckfree((char *)Tcl_GetHashValue(entryPtr));
                Tcl_DeleteHashEntry(entryPtr);
            }
        }
    }
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

/*
 * ------------------------------------------------------------------------
 *  RpDestroyMarkNames()
 *
 *  Used internally to clean up the marker names when the interpreter
 *  that owns them is being destroyed.
 * ------------------------------------------------------------------------
 */
static void
RpDestroyMarkNames(
    ClientData cdata,			/* data being destroyed */
    Tcl_Interp *interp)			/* Interpreter that owned the data */
{
    Tcl_HashTable *markNamesPtr = cdata;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;

    for (entryPtr = Tcl_FirstHashEntry(markNamesPtr, &search); entryPtr != NULL;
         entryPtr = Tcl_NextHashEntry(&search)) {
        ckfree( (char*)Tcl_GetHashValue(entryPtr) );
    }
    Tcl_DeleteHashTable(markNamesPtr);
    ckfree((char*)markNamesPtr);
}
