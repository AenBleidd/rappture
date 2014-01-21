/*
 * ----------------------------------------------------------------------
 *  Rappture::sysinfo
 *
 *  This is an interface to the system sysinfo() function, which gives
 *  information about system load averages, use of virtual memory, etc.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "config.h"
#include <tcl.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

static Tcl_CmdProc RpSysinfoCmd;


/*
 * ------------------------------------------------------------------------
 *  RpSysinfo_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpSysinfo_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    /* install the sysinfo command */
    Tcl_CreateCommand(interp, "::Rappture::sysinfo", RpSysinfoCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

#ifndef HAVE_SYSINFO

static int
RpSysinfoCmd(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    CONST84 char *argv[];     /* strings for command line args */
{
    Tcl_SetResult(interp, "command not implemented: no sysinfo", TCL_STATIC);
    return TCL_ERROR;
}

#else

#ifdef HAVE_SYS_SYSINFO_H
#include <sys/sysinfo.h>
#endif	/* HAVE_SYS_SYSINFO_H */

#define RP_SLOT_LONG   1
#define RP_SLOT_ULONG  2
#define RP_SLOT_USHORT 3
#define RP_SLOT_LOAD   4

struct rpSysinfoList {
    const char *name;		/* Name of this system parameter */
    int type;		        /* Parameter type (long, unsigned long,
				 * etc.) */
    int offset;			/* Offset into sysinfo struct */
};

#ifdef offsetof
#define Offset(type, field) ((int) offsetof(type, field))
#else
#define Offset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

static struct rpSysinfoList RpSysinfoList [] = {
    {"freeram",   RP_SLOT_ULONG,  Offset(struct sysinfo, freeram)},
    {"freeswap",  RP_SLOT_ULONG,  Offset(struct sysinfo, freeswap)},
    {"load1",     RP_SLOT_LOAD,   Offset(struct sysinfo, loads[0])},
    {"load5",     RP_SLOT_LOAD,   Offset(struct sysinfo, loads[1])},
    {"load15",    RP_SLOT_LOAD,   Offset(struct sysinfo, loads[2])},
    {"procs",     RP_SLOT_USHORT, Offset(struct sysinfo, procs)},
    {"totalram",  RP_SLOT_ULONG,  Offset(struct sysinfo, totalram)},
    {"totalswap", RP_SLOT_ULONG,  Offset(struct sysinfo, totalswap)},
    {"uptime",    RP_SLOT_LONG,   Offset(struct sysinfo, uptime)},
    {NULL, 0, 0}
};

static Tcl_Obj* RpSysinfoValue(struct sysinfo *sinfo, int idx);

/*
 * ------------------------------------------------------------------------
 *  RpSysinfoCmd()
 *
 *  Invoked whenever someone uses the "sysinfo" command to get info
 *  about system load.  Handles the following syntax:
 *
 *      sysinfo ?<keyword>...?
 *
 *  With no extra args, it returns a list of the form "key1 val1 key2
 *  val2 ..." with all known system info.  Otherwise, it treats each
 *  argument as a desired keyword and returns only the values for the
 *  specified keywords.
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpSysinfoCmd(cdata, interp, argc, argv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int argc;                 /* number of command line args */
    CONST84 char *argv[];     /* strings for command line args */
{
    char c;
    int n, i;
    struct sysinfo info;
    Tcl_Obj *resultPtr;

    if (sysinfo(&info) < 0) {
        Tcl_SetResult(interp, "oops! can't query system info", TCL_STATIC);
        return TCL_ERROR;
    }
    resultPtr = Tcl_GetObjResult(interp);

    /*
     *  If no args are given, return all info in the form:
     *    key1 value1 key2 value2 ...
     */
    if (argc < 2) {
        for (i=0; RpSysinfoList[i].name != NULL; i++) {
            Tcl_ListObjAppendElement(interp, resultPtr,
                Tcl_NewStringObj(RpSysinfoList[i].name, -1));
            Tcl_ListObjAppendElement(interp, resultPtr,
                RpSysinfoValue(&info, i));
        }
        return TCL_OK;
    }

    /*
     *  Query and return each of the specified values.
     */
    for (n=1; n < argc; n++) {
        c = *argv[n];
        for (i=0; RpSysinfoList[i].name != NULL; i++) {
            if (c == *RpSysinfoList[i].name
                  && strcmp(argv[n], RpSysinfoList[i].name) == 0) {
                break;
            }
        }

        /* couldn't find the name in the list?  then return an error */
        if (RpSysinfoList[i].name == NULL) {
            Tcl_ResetResult(interp);
            Tcl_AppendResult(interp, "bad parameter \"",
                argv[n], "\": should be one of ", (char*)NULL);
            for (i=0; RpSysinfoList[i].name != NULL; i++) {
                Tcl_AppendResult(interp, (i == 0) ? "" : ", ",
                    RpSysinfoList[i].name, (char*)NULL);
            }
            return TCL_ERROR;
        }
        Tcl_ListObjAppendElement(interp, resultPtr, RpSysinfoValue(&info, i));
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpSysinfoValue()
 *
 *  Returns the value for a desired system info parameter, specified as
 *  as offset into the global RpSysinfoList array.  The value is returned
 *  as a Tcl_Obj, which should be used or freed by the caller.
 * ------------------------------------------------------------------------
 */
static Tcl_Obj*
RpSysinfoValue(sinfo, idx)
    struct sysinfo *sinfo;    /* system load info */
    int idx;                  /* index for desired sysinfo value */
{
    char *ptr;
    int ival; long lval; double dval, loadscale;
    ptr = (char*)sinfo + RpSysinfoList[idx].offset;

    switch (RpSysinfoList[idx].type) {
        case RP_SLOT_LONG:
            return Tcl_NewLongObj(*(long*)ptr);
            break;
        case RP_SLOT_ULONG:
            lval = *(unsigned long*)ptr;
            return Tcl_NewLongObj(lval);
            break;
        case RP_SLOT_USHORT:
            ival = *(unsigned short*)ptr;
            return Tcl_NewIntObj(ival);
            break;
        case RP_SLOT_LOAD:
            loadscale = (double)(1 << SI_LOAD_SHIFT);
            dval = (double)(*(unsigned long*)ptr)/loadscale;
            return Tcl_NewDoubleObj(dval);
            break;
    }
    return NULL;
}

#endif /*HAVE_SYSINFO*/
