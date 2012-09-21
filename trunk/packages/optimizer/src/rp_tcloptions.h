/*
 * ----------------------------------------------------------------------
 *  rp_tcloptions
 *
 *  This library is used to implement configuration options for the
 *  Tcl API used in Rappture.  It lets you define a series of
 *  configuration options like the ones used for Tk widgets, and
 *  provides functions to process them.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RP_TCLOPTIONS
#define RP_TCLOPTIONS

#include <tcl.h>

/*
 * These data structures are used to define configuration options
 * associated with parameters and optimizer cores.  They work just
 * like Tk widget options, but for objects that are not widgets.
 */
#ifdef offsetof
#define Rp_Offset(type, field) ((int) offsetof(type, field))
#else
#define Rp_Offset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

typedef int (RpCustomTclOptionParse)_ANSI_ARGS_((Tcl_Interp *interp,
    Tcl_Obj *valObj, ClientData cdata, int offset));
typedef int (RpCustomTclOptionGet)_ANSI_ARGS_((Tcl_Interp *interp,
    ClientData cdata, int offset));
typedef void (RpCustomTclOptionCleanup)_ANSI_ARGS_((ClientData cdata,
    int offset));

typedef struct RpTclOptionType {
    char *type;                            /* name describing this type */
    RpCustomTclOptionParse *parseProc;     /* procedure to parse new values */
    RpCustomTclOptionGet *getProc;         /* procedure to report cur value */
    RpCustomTclOptionCleanup *cleanupProc; /* procedure to free up value */
} RpTclOptionType;

typedef struct RpTclOption {
    char *optname;             /* name of option: -switch */
    RpTclOptionType *typePtr;  /* type of this switch */
    int offset;                /* location of data within struct */
} RpTclOption;

/*
 *  Built-in types defined in rp_tcloptions.c
 */
extern RpTclOptionType RpOption_Boolean;
#define RP_OPTION_BOOLEAN &RpOption_Boolean

extern RpTclOptionType RpOption_Int;
#define RP_OPTION_INT &RpOption_Int

extern RpTclOptionType RpOption_Double;
#define RP_OPTION_DOUBLE &RpOption_Double

extern RpTclOptionType RpOption_String;
#define RP_OPTION_STRING &RpOption_String

extern RpTclOptionType RpOption_List;
#define RP_OPTION_LIST &RpOption_List

extern RpTclOptionType RpOption_Choices;
#define RP_OPTION_CHOICES &RpOption_Choices

/*
 *  Here are the functions in the API:
 */
EXTERN int RpTclOptionsProcess _ANSI_ARGS_((Tcl_Interp *interp,
    int objc, Tcl_Obj *CONST objv[], RpTclOption *options,
    ClientData cdata));

EXTERN int RpTclOptionGet _ANSI_ARGS_((Tcl_Interp *interp,
    RpTclOption *options, ClientData cdata, char *desiredOpt));

EXTERN void RpTclOptionsCleanup _ANSI_ARGS_((RpTclOption *options,
    ClientData cdata));

#endif
