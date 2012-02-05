/*
 * ----------------------------------------------------------------------
 *  INTERFACE: R Rappture Utils Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2011  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUtils.h"
#include "RpUtilsRInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************/
// FUNCTION: RPRUtilsProgress(percent,handle)
/// Send the =RAPPTURE-PROGRESS=> message to stdout for the progress widget
/**
 */

SEXP
RPRUtilsProgress(SEXP percent, SEXP message)
{
    SEXP ans;
    int err = 0;
    int percentInt = 0;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (length(percent) != 1) {
        error("percent is not a scalar value");
        UNPROTECT(1);
        return ans;
    }

    if (isReal(percent)) {
        percentInt = (int) asReal(percent);
    } else if (isInteger(percent)) {
        percentInt = asInteger(percent);
    } else {
        error("percent is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(message) || length(message) != 1) {
        error("message is not a single string");
        UNPROTECT(1);
        return ans;
    }

    err = Rappture::Utils::progress(percentInt,CHAR(STRING_ELT(message,0)));

    INTEGER(ans)[0] = err;

    UNPROTECT(1);

    return ans;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

