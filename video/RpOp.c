
/*
 * RpOp.c --
 *
 * This module implements utility procedures for the BLT toolkit.
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <tcl.h>
#include <string.h>
#include "RpOp.h"
/*
 *---------------------------------------------------------------------------
 *
 * BinaryOpSearch --
 *
 *      Performs a binary search on the array of command operation
 *      specifications to find a partial, anchored match for the given
 *      operation string.
 *
 * Results:
 *	If the string matches unambiguously the index of the specification in
 *	the array is returned.  If the string does not match, even as an
 *	abbreviation, any operation, -1 is returned.  If the string matches,
 *	but ambiguously -2 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
BinaryOpSearch(
    Rp_OpSpec *specs, 
    int nSpecs, 
    char *string)		/* Name of minor operation to search for */
{
    char c;
    int high, low;
    size_t length;

    low = 0;
    high = nSpecs - 1;
    c = string[0];
    length = strlen(string);
    while (low <= high) {
	Rp_OpSpec *specPtr;
	int compare;
	int median;
	
	median = (low + high) >> 1;
	specPtr = specs + median;

	/* Test the first character */
	compare = c - specPtr->name[0];
	if (compare == 0) {
	    /* Now test the entire string */
	    compare = strncmp(string, specPtr->name, length);
	    if (compare == 0) {
		if ((int)length < specPtr->minChars) {
		    return -2;	/* Ambiguous operation name */
		}
	    }
	}
	if (compare < 0) {
	    high = median - 1;
	} else if (compare > 0) {
	    low = median + 1;
	} else {
	    return median;	/* Op found. */
	}
    }
    return -1;			/* Can't find operation */
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearOpSearch --
 *
 *      Performs a binary search on the array of command operation
 *      specifications to find a partial, anchored match for the given
 *      operation string.
 *
 * Results:
 *	If the string matches unambiguously the index of the specification in
 *	the array is returned.  If the string does not match, even as an
 *	abbreviation, any operation, -1 is returned.  If the string matches,
 *	but ambiguously -2 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
LinearOpSearch(
    Rp_OpSpec *specs,
    int nSpecs,
    char *string)		/* Name of minor operation to search for */
{
    Rp_OpSpec *specPtr;
    char c;
    size_t length;
    int nMatches, last;
    int i;

    c = string[0];
    length = strlen(string);
    nMatches = 0;
    last = -1;
    for (specPtr = specs, i = 0; i < nSpecs; i++, specPtr++) {
	if ((c == specPtr->name[0]) && 
	    (strncmp(string, specPtr->name, length) == 0)) {
	    last = i;
	    nMatches++;
	    if ((int)length == specPtr->minChars) {
		break;
	    }
	}
    }
    if (nMatches > 1) {
	return -2;		/* Ambiguous operation name */
    } 
    if (nMatches == 0) {
	return -1;		/* Can't find operation */
    } 
    return last;		/* Op found. */
}

/*
 *---------------------------------------------------------------------------
 *
 * Rp_GetOpFromObj --
 *
 *      Find the command operation given a string name.  This is useful where
 *      a group of command operations have the same argument signature.
 *
 * Results:
 *      If found, a pointer to the procedure (function pointer) is returned.
 *      Otherwise NULL is returned and an error message containing a list of
 *      the possible commands is returned in interp->result.
 *
 *---------------------------------------------------------------------------
 */
void *
Rp_GetOpFromObj(
    Tcl_Interp *interp,		/* Interpreter to report errors to */
    int nSpecs,			/* Number of specifications in array */
    Rp_OpSpec *specs,		/* Op specification array */
    int operPos,		/* Position of operation in argument list. */
    int objc,			/* Number of arguments in the argument vector.
				 * This includes any prefixed arguments */
    Tcl_Obj *const *objv,	/* Argument vector */
    int flags)
{
    Rp_OpSpec *specPtr;
    char *string;
    int n;

    if (objc <= operPos) {	/* No operation argument */
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
	return NULL;
    }
    string = Tcl_GetString(objv[operPos]);
    if (flags & RP_OP_LINEAR_SEARCH) {
	n = LinearOpSearch(specs, nSpecs, string);
    } else {
	n = BinaryOpSearch(specs, nSpecs, string);
    }
    if (n == -2) {
	char c;
	size_t length;

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
	return NULL;

    } else if (n == -1) {	/* Can't find operation, display help */
	Tcl_AppendResult(interp, "bad", (char *)NULL);
	if (operPos > 2) {
	    Tcl_AppendResult(interp, " ", Tcl_GetString(objv[operPos - 1]), 
		(char *)NULL);
	}
	Tcl_AppendResult(interp, " operation \"", string, "\": ", (char *)NULL);
	goto usage;
    }
    specPtr = specs + n;
    if ((objc < specPtr->minArgs) || 
	((specPtr->maxArgs > 0) && (objc > specPtr->maxArgs))) {
	int i;

	Tcl_AppendResult(interp, "wrong # args: should be \"", (char *)NULL);
	for (i = 0; i < operPos; i++) {
	    Tcl_AppendResult(interp, Tcl_GetString(objv[i]), " ", 
		(char *)NULL);
	}
	Tcl_AppendResult(interp, specPtr->name, " ", specPtr->usage, "\"",
	    (char *)NULL);
	return NULL;
    }
    return specPtr->proc;
}
