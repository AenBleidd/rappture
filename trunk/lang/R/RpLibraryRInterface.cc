/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Library Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2011  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibrary.h"
#include "RpBindingsDict.h"

#include <R.h>
#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

static void
rp_lib_finalizer(SEXP ptr)
{
    RpLibrary* lib = NULL;
    if (!R_ExternalPtrAddr(ptr)) {
        return;
    }
    lib = (RpLibrary *) R_ExternalPtrAddr(ptr);
    if (lib != NULL) {
        delete lib;
        lib = NULL;
    }
    R_ClearExternalPtr(ptr);

}



/**********************************************************************/
// FUNCTION: rp_lib(const char* filePath, int handle)
/// Open the file at 'filePath' and return Rappture Library Object.
/**
 */

SEXP
RPRLib(SEXP fname)
{
    RpLibrary* lib = NULL;
    int handle = -1;
    SEXP ans, ptr;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);
    INTEGER(ans)[0] = -1;

    if (!isString(fname) || length(fname) != 1) {
        error("fname is not a single string");
        UNPROTECT(1);
        return ans;
    }

    // create a RapptureIO object and store in dictionary
    lib = new RpLibrary(CHAR(STRING_ELT(fname, 0)));
    if (lib == NULL) {
        error("could not allocate new RpLibrary object");
        UNPROTECT(1);
        return ans;
    }

    ptr = R_MakeExternalPtr((void *)lib,install("RapptureLib"),R_NilValue);
    PROTECT(ptr);
    R_RegisterCFinalizerEx(ptr, rp_lib_finalizer, TRUE);

    handle = storeObject_Void((void*)lib);

    INTEGER(ans)[0] = handle;
    UNPROTECT(2);

    return(ans);
}

/**********************************************************************/
// FUNCTION: RPRLibGetString()
/// Get data located at 'path' and return it as a string value.
/**
 */
SEXP
RPRLibGetString(
    SEXP handle,            // integer handle of library
    SEXP path)              // null terminated path
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    std::string data;

    ans = allocVector(STRSXP,1);
    PROTECT(ans);

    SET_STRING_ELT(ans,0,mkChar(""));

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    data = lib->getString(CHAR(STRING_ELT(path, 0)));

    SET_STRING_ELT(ans, 0, mkChar(data.c_str()));

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibGetDouble()
/// Get data located at 'path' and return it as a double precision value.
/**
 */
SEXP
RPRLibGetDouble(
    SEXP handle,            // integer handle of library
    SEXP path)              // null terminated path
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    double data;

    ans = allocVector(REALSXP,1);
    PROTECT(ans);

    REAL(ans)[0] = 0.0;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not a single integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    data = lib->getDouble(CHAR(STRING_ELT(path, 0)));

    REAL(ans)[0] = data;

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibInteger()
/// Get data located at 'path' and return it as an integer value.
/**
 */
SEXP
RPRLibGetInteger(
    SEXP handle,            // integer handle of library
    SEXP path)              // null terminated path
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    int data;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = 0;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not a single integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    data = lib->getInt(CHAR(STRING_ELT(path, 0)));

    INTEGER(ans)[0] = data;

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibGetBoolean()
/// Get data located at 'path' and return it as an integer value.
/**
 */
SEXP
RPRLibGetBoolean(
    SEXP handle,            // integer handle of library
    SEXP path)              // null terminated path
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    bool data;

    ans = allocVector(LGLSXP,1);
    PROTECT(ans);

    LOGICAL(ans)[0] = false;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not a single integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    data = lib->getBool(CHAR(STRING_ELT(path, 0)));

    LOGICAL(ans)[0] = data;

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibGetFile()
/// Get data located at 'path' and write it to the file 'fileName'.
/**
 * Returns if any bytes were written to the file
 */
SEXP
RPRLibGetFile (
    SEXP handle,            // integer handle of library
    SEXP path,              // null terminated path
    SEXP fileName)          // name of file to write data to
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    int nbytes = 0;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not a single integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(fileName) || length(fileName) != 1) {
        error("fileName is not a string");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    nbytes = lib->getFile(  CHAR(STRING_ELT(path, 0)),
                            CHAR(STRING_ELT(fileName, 0)));

    INTEGER(ans)[0] = nbytes;

    UNPROTECT(1);

    return ans;
}



/**********************************************************************/
// FUNCTION: RPRLibPutString()
/// Put string into Rappture Library Object at location 'path'.
/**
 */
SEXP
RPRLibPutString(
    SEXP handle,
    SEXP path,
    SEXP value,
    SEXP append)
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    int nbytes = 0;
    unsigned int appendVal;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(value) || length(value) != 1) {
        error("value is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(append) || length(append) != 1) {
        error("append is not a logical");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    appendVal = asLogical(append);
    if (appendVal == 1) {
        appendVal = RPLIB_APPEND;
    } else if (appendVal == 0) {
        appendVal = RPLIB_OVERWRITE;
    } else {
        // NA_LOGICAL was returned?
        error("invalid append value");
        UNPROTECT(1);
        return ans;
    }

    lib->put(CHAR(STRING_ELT(path, 0)),
             CHAR(STRING_ELT(value, 0)),
             "",
             appendVal,
             RPLIB_TRANSLATE);


    INTEGER(ans)[0] = 1;

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibPutData()
/// Put string into Rappture Library Object at location 'path'.
/**
 */
/*
SEXP
RPRLibPutData(
    SEXP handle,
    SEXP path,
    SEXP bytes,
    SEXP nbytes,
    SEXP append)
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    int nbytes = 0;
    unsigned int appendVal;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(bytes) || length(bytes) != 1) {
        error("bytes is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isInteger(nbytes) || length(nbytes) != 1) {
        error("nbytes is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(append) || length(append) != 1) {
        error("append is not a logical");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    if (asLogical(append)) {
        appendVal = RPLIB_APPEND;
    } else {
        appendVal = RPLIB_OVERWRITE;
    }

    lib->putData(CHAR(STRING_ELT(path, 0)),
                 CHAR(STRING_ELT(value, 0)),
                 INTEGER(nbytes),
                 appendVal);

    INTEGER(ans)[0] = 1;

    UNPROTECT(1);

    return ans;
}
*/

/**********************************************************************/
// FUNCTION: RPRLibPutDouble()
/// Put double value into Rappture Library Object at location 'path'.
/**
 */
SEXP
RPRLibPutDouble(
    SEXP handle,
    SEXP path,
    SEXP value,
    SEXP append)
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    int nbytes = 0;
    unsigned int appendVal;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isReal(value) || length(value) != 1) {
        error("value is not a real");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(append) || length(append) != 1) {
        error("append is not a logical");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    appendVal = asLogical(append);
    if (appendVal == 1) {
        appendVal = RPLIB_APPEND;
    } else if (appendVal == 0) {
        appendVal = RPLIB_OVERWRITE;
    } else {
        // NA_LOGICAL was returned?
        error("invalid append value");
        UNPROTECT(1);
        return ans;
    }

    lib->put(CHAR(STRING_ELT(path, 0)),
             asReal(value),
             "",
             appendVal);

    INTEGER(ans)[0] = 1;

    UNPROTECT(1);

    return ans;
}


/**********************************************************************/
// FUNCTION: RPRLibPutFile()
/// Put double value into Rappture Library Object at location 'path'.
/**
 */
SEXP
RPRLibPutFile(
    SEXP handle,
    SEXP path,
    SEXP fname,
    SEXP compress,
    SEXP append)
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;
    unsigned int appendVal;
    unsigned int compressVal;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(path) || length(path) != 1) {
        error("path is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isString(fname) || length(fname) != 1) {
        error("fileName is not a string");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(compress) || length(compress) != 1) {
        error("compress is not a logical");
        UNPROTECT(1);
        return ans;
    }

    if (!isLogical(append) || length(append) != 1) {
        error("append is not a logical");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    compressVal = asLogical(compress);
    if (compressVal == 1) {
        compressVal = RPLIB_COMPRESS;
    } else {
        compressVal = RPLIB_NO_COMPRESS;
    }

    appendVal = asLogical(append);
    if (appendVal == 1) {
        appendVal = RPLIB_APPEND;
    } else {
        appendVal = RPLIB_OVERWRITE;
    }

    lib->putFile(CHAR(STRING_ELT(path, 0)),
                 CHAR(STRING_ELT(fname,0)),
                 compressVal,
                 appendVal);

    INTEGER(ans)[0] = 1;

    UNPROTECT(1);

    return ans;
}

/**********************************************************************/
// FUNCTION: rp_result()
/// Write xml text to a run.xml file and signal the program has completed
/**
 */
SEXP
RPRLibResult(
    SEXP handle)
{
    RpLibrary* lib = NULL;
    SEXP ans;
    int handleVal = -1;

    ans = allocVector(INTSXP,1);
    PROTECT(ans);

    INTEGER(ans)[0] = -1;

    if (!isInteger(handle) || length(handle) != 1) {
        error("handle is not an integer");
        UNPROTECT(1);
        return ans;
    }

    handleVal = asInteger(handle);

    if (handleVal == 0) {
        error("invalid handle");
        UNPROTECT(1);
        return ans;
    }

    lib = (RpLibrary*) getObject_Void(handleVal);

    if (lib == NULL) {
        error("invalid Rappture Library Object");
        UNPROTECT(1);
        return ans;
    }

    lib->put("tool.version.rappture.language", "R");
    lib->result();

    INTEGER(ans)[0] = 1;

    UNPROTECT(1);

    return ans;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

