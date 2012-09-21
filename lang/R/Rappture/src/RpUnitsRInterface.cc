/*
 * ----------------------------------------------------------------------
 *  INTERFACE: R Rappture Units Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUnits.h"
#include "RpBindingsDict.h"

# include "RpUnitsRInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************/
// FUNCTION: RPRUnitsConvertDouble(fromVal,toUnitsName)
/// Perform units convertion on fromVal, return a double
/**
 */

SEXP
RPRUnitsConvertDouble(
    SEXP fromVal,
    SEXP toUnitsName)
{
    SEXP ans;
    std::string convStr = "";
    int showUnitsFlag = 0;
    double convResult = 0.0;
    int err = 0;

    ans = allocVector(REALSXP,1);
    PROTECT(ans);

    REAL(ans)[0] = 0.0;

    if (!isString(fromVal) || length(fromVal) != 1) {
        error("fromVal is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(toUnitsName) || length(toUnitsName) != 1) {
        error("toUnitsName is not a string");
        UNPROTECT(1);
        return ans;
    }

    // the call to RpUnits::convert() populates the result flag
    convStr = RpUnits::convert(CHAR(STRING_ELT(fromVal,0)),
                               CHAR(STRING_ELT(toUnitsName,0)),
                               showUnitsFlag,
                               &err);

    if (err) {
        error("unrecognized unit conversion");
        UNPROTECT(1);
        return ans;
    }

    REAL(ans)[0] = atof(convStr.c_str());

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRUnitsConvertString(fromVal,toUnitsName,showUnits)
/// Perform units convertion on fromVal, return a string
/**
 */

SEXP
RPRUnitsConvertString(
    SEXP fromVal,
    SEXP toUnitsName,
    SEXP showUnits)
{
    SEXP ans;
    std::string convStr = "";
    int showUnitsFlag = 1;
    double convResult = 0.0;
    int err = 0;

    ans = allocVector(STRSXP,1);
    PROTECT(ans);

    SET_STRING_ELT(ans,0,mkChar(""));

    if (!isString(fromVal) || length(fromVal) != 1) {
        error("fromVal is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(toUnitsName) || length(toUnitsName) != 1) {
        error("toUnitsName is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(showUnits) || length(showUnits) != 1) {
        error("showUnits is not a logical");
        UNPROTECT(1);
        return ans;
    }

    showUnitsFlag = asLogical(showUnits);
    if (showUnitsFlag == 1) {
        showUnitsFlag = RPUNITS_UNITS_ON;
    } else {
        showUnitsFlag = RPUNITS_UNITS_OFF;
    }

    // the call to RpUnits::convert() populates the result flag
    convStr = RpUnits::convert(CHAR(STRING_ELT(fromVal,0)),
                               CHAR(STRING_ELT(toUnitsName,0)),
                               showUnitsFlag,
                               &err);

    if (err) {
        error("unrecognized unit conversion");
        UNPROTECT(1);
        return ans;
    }

    SET_STRING_ELT(ans,0,mkChar(convStr.c_str()));

    UNPROTECT(1);

    return ans;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

