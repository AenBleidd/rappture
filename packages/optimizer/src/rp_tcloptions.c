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
#include "rp_tcloptions.h"
#include <string.h>
#include <malloc.h>

/*
 *  Built-in types:
 */
RpCustomTclOptionParse RpOption_ParseBoolean;
RpCustomTclOptionGet RpOption_GetBoolean;
RpTclOptionType RpOption_Boolean = {
    "boolean", RpOption_ParseBoolean, RpOption_GetBoolean, NULL
};

RpCustomTclOptionParse RpOption_ParseInt;
RpCustomTclOptionGet RpOption_GetInt;
RpTclOptionType RpOption_Int = {
    "integer", RpOption_ParseInt, RpOption_GetInt, NULL
};

RpCustomTclOptionParse RpOption_ParseDouble;
RpCustomTclOptionGet RpOption_GetDouble;
RpTclOptionType RpOption_Double = {
    "double", RpOption_ParseDouble, RpOption_GetDouble, NULL
};

RpCustomTclOptionParse RpOption_ParseString;
RpCustomTclOptionGet RpOption_GetString;
RpCustomTclOptionCleanup RpOption_CleanupString;
RpTclOptionType RpOption_String = {
    "string", RpOption_ParseString, RpOption_GetString, RpOption_CleanupString
};

RpCustomTclOptionParse RpOption_ParseList;
RpCustomTclOptionGet RpOption_GetList;
RpCustomTclOptionCleanup RpOption_CleanupList;
RpTclOptionType RpOption_List = {
    "list", RpOption_ParseList, RpOption_GetList, RpOption_CleanupList
};

/*
 * ------------------------------------------------------------------------
 *  RpTclOptionsProcess()
 *
 *  Used internally to handle options processing for all optimization
 *  parameters.  Expects a list of "-switch value" parameters in the
 *  (objc,objv) arguments, and processes them according to the given
 *  specs.  Returns TCL_OK if successful.  If anything goes wrong, it
 *  leaves an error message in the interpreter and returns TCL_ERROR.
 * ------------------------------------------------------------------------
 */
int
RpTclOptionsProcess(interp, objc, objv, options, cdata)
    Tcl_Interp *interp;           /* interpreter for errors */
    int objc;                     /* number of args to process */
    Tcl_Obj *CONST objv[];        /* arg values to process */
    RpTclOption *options;         /* specification for known options */
    ClientData cdata;             /* option values inserted in here */
{
    int n, status;
    RpTclOption *spec;
    char *opt;

    for (n=0; n < objc; n++) {
        opt = Tcl_GetStringFromObj(objv[n], (int*)NULL);

        for (spec=options; spec->optname; spec++) {
            if (strcmp(opt,spec->optname) == 0) {
                /* found matching option spec! */
                if (n+1 >= objc) {
                    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                        "missing value for option \"", spec->optname, "\"",
                        (char*)NULL);
                    return TCL_ERROR;
                }

                status = (*spec->typePtr->parseProc)(interp, objv[n+1], cdata,
                    spec->offset);

                if (status != TCL_OK) {
                    return TCL_ERROR;
                }
                n++;     /* skip over value just processed */
                break;   /* indicate that arg was handled */
            }
        }
        if (spec->optname == NULL) {
            char *sep = "";
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option \"", opt, "\": should be ", (char*)NULL);
            for (spec=options; spec->optname; spec++) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    sep, spec->optname, (char*)NULL);
                sep = ", ";
            }
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpTclOptionGet()
 *
 *  Used internally to query the value of an option associated with
 *  an optimization parameter.  Returns TCL_OK along with the current
 *  value in the interpreter.  If the desired option name is not
 *  recognized or anything else goes wrong, it returns TCL_ERROR along
 *  with an error message.
 * ------------------------------------------------------------------------
 */
int
RpTclOptionGet(interp, options, cdata, desiredOpt)
    Tcl_Interp *interp;           /* interp for result or errors */
    RpTclOption *options;         /* specification for known options */
    ClientData cdata;             /* option values are found in here */
    char *desiredOpt;             /* look for this option name */
{
    RpTclOption *spec;
    char *sep;

    for (spec=options; spec->optname; spec++) {
        if (strcmp(desiredOpt,spec->optname) == 0) {
            /* found matching option spec! */
            return (*spec->typePtr->getProc)(interp, cdata, spec->offset);
        }
    }

    /* oops! desired option name not found */
    sep = "";
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
        "bad option \"", desiredOpt, "\": should be ", (char*)NULL);
    for (spec=options; spec->optname; spec++) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            sep, spec->optname, (char*)NULL);
        sep = ", ";
    }
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  RpTclOptionsCleanup()
 *
 *  Used internally to free all memory associated with various values
 *  configured via the options package.  Frees any allocated memory
 *  and resets values to their null state.
 * ------------------------------------------------------------------------
 */
void
RpTclOptionsCleanup(options, cdata)
    RpTclOption *options;         /* specification for known options */
    ClientData cdata;             /* option values are found in here */
{
    RpTclOption *spec;

    for (spec=options; spec->optname; spec++) {
        if (spec->typePtr->cleanupProc) {
            (*spec->typePtr->cleanupProc)(cdata, spec->offset);
        }
    }
}

/*
 * ======================================================================
 *  BOOLEAN
 * ======================================================================
 */
int
RpOption_ParseBoolean(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    if (Tcl_GetBooleanFromObj(interp, valObj, ptr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetBoolean(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(*ptr));
    return TCL_OK;
}

/*
 * ======================================================================
 *  INTEGER
 * ======================================================================
 */
int
RpOption_ParseInt(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    if (Tcl_GetIntFromObj(interp, valObj, ptr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetInt(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(*ptr));
    return TCL_OK;
}

/*
 * ======================================================================
 *  DOUBLE
 * ======================================================================
 */
int
RpOption_ParseDouble(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    double *ptr = (double*)(cdata+offset);
    if (Tcl_GetDoubleFromObj(interp, valObj, ptr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetDouble(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    double *ptr = (double*)(cdata+offset);
    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(*ptr));
    return TCL_OK;
}

/*
 * ======================================================================
 *  STRING
 * ======================================================================
 */
int
RpOption_ParseString(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    char **ptr = (char**)(cdata+offset);
    char *value = Tcl_GetStringFromObj(valObj, (int*)NULL);
    *ptr = strdup(value);
    return TCL_OK;
}

int
RpOption_GetString(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    char *ptr = (char*)(cdata+offset);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(ptr,-1));
    return TCL_OK;
}

void
RpOption_CleanupString(cdata, offset)
    ClientData cdata;    /* cleanup in this data structure */
    int offset;          /* cleanup at this offset in cdata */
{
    char **ptr = (char**)(cdata+offset);
    if (*ptr != NULL) {
        free((char*)(*ptr));
        *ptr = NULL;
    }
}

/*
 * ======================================================================
 *  LIST
 * ======================================================================
 */
int
RpOption_ParseList(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int j;
    int vc; Tcl_Obj **vv;
    char *value, **allowedValues;

    if (Tcl_ListObjGetElements(interp, valObj, &vc, &vv) != TCL_OK) {
        return TCL_ERROR;
    }

    /* transfer them to an array of char* values */
    allowedValues = (char**)malloc((vc+1)*(sizeof(char*)));
    for (j=0; j < vc; j++) {
        value = Tcl_GetStringFromObj(vv[j], (int*)NULL);
        allowedValues[j] = strdup(value);
    }
    allowedValues[j] = NULL;

    *(char***)(cdata+offset) = allowedValues;
    return TCL_OK;
}

int
RpOption_GetList(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int n;
    char **ptr = *(char***)(cdata+offset);
    Tcl_ResetResult(interp);
    for (n=0; ptr[n]; n++) {
        Tcl_AppendElement(interp, ptr[n]);
    }
    return TCL_OK;
}

void
RpOption_CleanupList(cdata, offset)
    ClientData cdata;    /* cleanup in this data structure */
    int offset;          /* cleanup at this offset in cdata */
{
    int n;
    char **ptr = *(char***)(cdata+offset);
    for (n=0; ptr[n]; n++) {
        free((char*)(ptr[n]));
        ptr[n] = NULL;
    }
    free((char*)ptr);
    *(char***)(cdata+offset) = NULL;
}

/*
 * ======================================================================
 *  CHOICES
 * ======================================================================
 */
int
RpOption_ParseChoices(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    char **ptr = (char**)(cdata+offset);
    char *value = Tcl_GetStringFromObj(valObj, (int*)NULL);
    *ptr = strdup(value);
    return TCL_OK;
}

int
RpOption_GetChoices(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    char *ptr = (char*)(cdata+offset);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(ptr,-1));
    return TCL_OK;
}

void
RpOption_CleanupChoices(cdata, offset)
    ClientData cdata;    /* cleanup in this data structure */
    int offset;          /* cleanup at this offset in cdata */
{
    char **ptr = (char**)(cdata+offset);
    if (*ptr != NULL) {
        free((char*)(*ptr));
        *ptr = NULL;
    }
}
