/*
 * ----------------------------------------------------------------------
 *  Rappture::slice
 *
 *  This is similar to the usual Tcl "split" command, in that it
 *  splits a string into a series of words.  The difference is that
 *  this command understands quoting characters and will ignore the
 *  split inside quotes, and it will also treat a bunch of split
 *  characters as synonymous and group them together, treating any
 *  combination of them together as a single split.  (Tcl gives you
 *  a bunch of {} empty strings between such characters.)
 *
 *  EXAMPLES:
 *    Rappture::slice -open { -close } -separators ", \t\n" $string
 *    Rappture::slice -open \" -close \" -separators ", " $string
 *
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

static Tcl_ObjCmdProc RpSliceCmd;

/*
 * ------------------------------------------------------------------------
 *  RpSlice_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpSlice_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    Tcl_CreateObjCommand(interp, "::Rappture::slice", RpSliceCmd, NULL, NULL);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpSliceCmd()
 *
 *  Invoked whenever someone uses the "slice" command to slice a string
 *  into multiple components.  Handles the following syntax:
 *
 *      slice ?-open <char>? ?-close <char>? ?-separators <abc>? <string>
 *
 *  Returns TCL_OK on success, and TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
static int
RpSliceCmd(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *const *objv;     /* strings for command line args */
{
    char *openq = ""; int openqLen = 0;
    char *closeq = ""; int closeqLen = 0;
    char *sep = " \t\n"; int sepLen = 3;

    Tcl_Obj *resultPtr, *objPtr;
    char *arg, *str, *token;
    int len, tlen, pos, quotec;

    /*
     * Handle any flags on the command line.
     */
    pos = 1;
    while (pos < objc-1) {
        arg = Tcl_GetStringFromObj(objv[pos], &len);
        if (*arg == '-') {
            if (strcmp(arg,"-open") == 0) {
                if (pos+1 < objc) {
                    openq = Tcl_GetStringFromObj(objv[pos+1], &openqLen);
                    pos += 2;
                } else {
                    Tcl_AppendResult(interp, "missing value for -open",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strcmp(arg,"-close") == 0) {
                if (pos+1 < objc) {
                    closeq = Tcl_GetStringFromObj(objv[pos+1], &closeqLen);
                    pos += 2;
                } else {
                    Tcl_AppendResult(interp, "missing value for -close",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strcmp(arg,"-separators") == 0) {
                if (pos+1 < objc) {
                    sep = Tcl_GetStringFromObj(objv[pos+1], &sepLen);
                    pos += 2;
                } else {
                    Tcl_AppendResult(interp, "missing value for -separators",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strcmp(arg,"--") == 0) {
                pos++;
                break;
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", arg, "\":",
                    " should be -open, -close, -separators, --", (char*)NULL);
                return TCL_ERROR;
            }
        } else {
            break;
        }
    }

    /*
     * Open/close quote strings must match in length.  Each char in openq
     * corresponds to a close quote in closeq.
     */
    if (openqLen != closeqLen) {
        Tcl_AppendResult(interp, "must have same number of quote characters"
            " for -open and -close", (char*)NULL);
        return TCL_ERROR;
    }

    if (pos != objc-1) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " ?-open chars? ?-close chars?",
            " ?-separators chars? ?--? string", (char*)NULL);
        return TCL_ERROR;
    }
    str = Tcl_GetStringFromObj(objv[pos], &len);

    /*
     * Scan through all characters.  If we find a match with an open quote,
     * then go into "quotes" mode; keep scanning until we find a matching
     * close quote.  If we find the start of a token, then mark it and keep
     * searching.  If we find a separator, then add the token to the list
     * and skip over remaining separators.
     */
    resultPtr = Tcl_NewListObj(0, NULL);
    token = NULL;
    quotec = -1;

    while (len > 0) {
        /*
         * If we're in a quoted part, then look for a closing quote.
         */
        if (quotec >= 0) {
            if (*str == '\\') {
                /* ignore the next character no matter what it is */
                str++; len--;
                if (len > 0) { str++; len--; }
                continue;
            }

            if (*str == closeq[quotec]) {
                /* found a close quote -- out of quote mode and move on */
                tlen = str-token;
                objPtr = Tcl_NewStringObj(token,tlen);
                Tcl_ListObjAppendElement(interp, resultPtr, objPtr);
                token = NULL;
                quotec = -1;
            }
        }

        /*
         * If we're in the middle of a token, then look for the next
         * separator.  When we find it, add the token to the result
         * list.
         */
        else if (token) {
            if (*str == '\\') {
                /* ignore the next character no matter what it is */
                str++; len--;
                if (len > 0) { str++; len--; }
                continue;
            }
            for (pos=0; pos < sepLen; pos++) {
                if (*str == sep[pos]) {
                    break;
                }
            }
            if (pos < sepLen) {
                /* found a separator -- add the token */
                tlen = str-token;
                objPtr = Tcl_NewStringObj(token,tlen);
                Tcl_ListObjAppendElement(interp, resultPtr, objPtr);
                token = NULL;
                continue;
            }
        }

        /*
         * If we're between tokens, then look for either a separator
         * or an open quote.  If we don't find either, then we've found
         * the start of a token.
         */
        else {
            for (pos=0; pos < sepLen; pos++) {
                if (*str == sep[pos]) {
                    break;
                }
            }
            if (pos >= sepLen) {
                /* no separator -- look for an open quote */
                for (pos=0; pos < openqLen; pos++) {
                    if (*str == openq[pos]) {
                        break;
                    }
                }
                if (pos < openqLen) {
                    /* found an open quote -- start quote mode */
                    quotec = pos;
                    token = str+1;
                } else {
                    /* not a sep or a quote -- start a token */
                    token = str;
                }
            }
        }

        /* next character */
        str++; len--;
    }

    /*
     * At the end of the string.  Have a token in progress?  Then add
     * it onto the result.
     */
    if (token) {
        tlen = str-token;
        objPtr = Tcl_NewStringObj(token, tlen);
        Tcl_ListObjAppendElement(interp, resultPtr, objPtr);
    }

    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}
