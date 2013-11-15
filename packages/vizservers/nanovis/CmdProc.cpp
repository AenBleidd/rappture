/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <string.h>

#include <tcl.h>

#include "CmdProc.h"

using namespace nv;

/**
 * Performs a linear search on the array of command operation
 * specifications to find a partial, anchored match for the given
 * operation string.
 *
 * \return If the string matches unambiguously the index of the specification in
 * the array is returned.  If the string does not match, even as an
 * abbreviation, any operation, -1 is returned.  If the string matches,
 * but ambiguously -2 is returned.
 */
static int
LinearOpSearch(CmdSpec *specs, int low, int high, const char *string,
               int length)
{
    CmdSpec *specPtr;
    char c;
    int nMatches, last;
    int i;

    c = string[0];
    nMatches = 0;
    last = -1;
    for (specPtr = specs+low, i = low; i <= high; i++, specPtr++) {
        if ((c == specPtr->name[0]) && 
            (strncmp(string, specPtr->name, length) == 0)) {
            last = i;
            nMatches++;
            if (length == specPtr->minChars) {
                break;
            }
        }
    }
    if (nMatches > 1) {
        return -2;      /* Ambiguous operation name */
    } 
    if (nMatches == 0) {
        return -1;      /* Can't find operation */
    } 
    return last;        /* Op found. */
}

/**
 * Performs a binary search on the array of command operation
 * specifications to find a partial, anchored match for the given
 * operation string.  If we get to a point where the sample string
 * partially matches an entry, we then revert to a linear search
 * over the given range to check if it's an exact match or
 * ambiguous (example: "color" with commands color and colormode).
 *
 * \return If the string matches unambiguously the index of the specification in
 * the array is returned.  If the string does not match, even as an
 * abbreviation, any operation, -1 is returned.  If the string matches,
 * but ambiguously -2 is returned.
 */
static int
BinaryOpSearch(CmdSpec *specs, int low, int high, const char *string,
               int length)
{
    char c;

    c = string[0];
    while (low <= high) {
        CmdSpec *specPtr;
        int compare;
        int median;
    
        median = (low + high) >> 1;
        specPtr = specs + median;

        /* Test the first character */
        compare = c - specPtr->name[0];
        if (compare == 0) {
            /* Now test the entire string */
            compare = strncmp(string, specPtr->name, length);
        }
        if (compare < 0) {
            high = median - 1;
        } else if (compare > 0) {
            low = median + 1;
        } else {
            if (length < specPtr->minChars) {
                /* Verify that the string is either ambiguous or an exact
                 * match of another command by doing a linear search over the
                 * given interval. */
                return LinearOpSearch(specs, low, high, string, length);
            }
            return median;  /* Op found. */
        }
    }
    return -1;          /* Can't find operation */
}

/**
 * \brief Find the command operation given a string name.
 * 
 * This is useful where a group of command operations have the same argument 
 * signature.
 *
 * \return If found, a pointer to the procedure (function pointer) is returned.
 * Otherwise NULL is returned and an error message containing a list of
 * the possible commands is returned in interp->result.
 */
Tcl_ObjCmdProc *
nv::GetOpFromObj(Tcl_Interp *interp,	/* Interpreter to report errors to */
                 int nSpecs,		/* Number of specifications in array */
                 CmdSpec *specs,		/* Op specification array */
                 int operPos,		/* Position of operation in argument
                                         * list. */
                 int objc,		/* Number of arguments in the argument
                                         * vector.  This includes any prefixed
                                         * arguments */ 
                 Tcl_Obj *const *objv,	/* Argument vector */
                 int flags)
{
    CmdSpec *specPtr;
    char *string;
    int length;
    int n;

    if (objc <= operPos) {  /* No operation argument */
        if (interp != NULL) {
            Tcl_AppendResult(interp, "wrong # args: ", (char *)NULL);
        usage:
            Tcl_AppendResult(interp, "should be one of...", (char *)NULL);
            for (n = 0; n < nSpecs; n++) {
                int i;

                Tcl_AppendResult(interp, "\n  ", (char *)NULL);
                for (i = 0; i < operPos; i++) {
                    Tcl_AppendResult(interp, Tcl_GetString(objv[i]), " ", 
                                     (char *)NULL);
                }
                specPtr = specs + n;
                Tcl_AppendResult(interp, specPtr->name, " ", specPtr->usage,
                                 (char *)NULL);
            }
        }
        return NULL;
    }
    string = Tcl_GetStringFromObj(objv[operPos], &length);
    if (flags & CMDSPEC_LINEAR_SEARCH) {
        n = LinearOpSearch(specs, 0, nSpecs - 1, string, length);
    } else {
        n = BinaryOpSearch(specs, 0, nSpecs - 1, string, length);
    }
    if (n == -2) {
        if (interp != NULL) {
            char c;

            Tcl_AppendResult(interp, "ambiguous", (char *)NULL);
            if (operPos > 2) {
                Tcl_AppendResult(interp, " ", Tcl_GetString(objv[operPos - 1]), 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, " operation \"", string, "\" matches: ",
                             (char *)NULL);

            c = string[0];
            length = strlen(string);
            for (n = 0; n < nSpecs; n++) {
                specPtr = specs + n;
                if ((c == specPtr->name[0]) &&
                    (strncmp(string, specPtr->name, length) == 0)) {
                    Tcl_AppendResult(interp, " ", specPtr->name, (char *)NULL);
                }
            }
        }
        return NULL;

    } else if (n == -1) {   /* Can't find operation, display help */
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad", (char *)NULL);
            if (operPos > 2) {
                Tcl_AppendResult(interp, " ", Tcl_GetString(objv[operPos - 1]), 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, " operation \"", string, "\": ", (char *)NULL);
            goto usage;
        }
        return NULL;
    }
    specPtr = specs + n;
    if ((objc < specPtr->minArgs) || 
        ((specPtr->maxArgs > 0) && (objc > specPtr->maxArgs))) {
        if (interp != NULL) {
            int i;

            Tcl_AppendResult(interp, "wrong # args: should be \"", (char *)NULL);
            for (i = 0; i < operPos; i++) {
                Tcl_AppendResult(interp, Tcl_GetString(objv[i]), " ", 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, specPtr->name, " ", specPtr->usage, "\"",
                             (char *)NULL);
        }
        return NULL;
    }
    return specPtr->proc;
}
