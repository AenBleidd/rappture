/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retData,size,err] = rpLibGetData(libHandle,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2010
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"
#include "RpEncode.h"

/**********************************************************************/
// METHOD: [retData,size,err] = rpLibGetData(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. Values are returned as strings.
 *
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibGetData, args, ,
"-*- texinfo -*-\n\
[retData,size,err] = rpLibGetData(@var{libHandle},@var{path})\n\
\n\
Clients use this to query the value of a node.  If the path\n\
is not specified, it returns the value associated with the\n\
root node.  Otherwise, it returns the value for the element\n\
specified by the path. Values are returned as strings.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibGetData";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    int libHandle = 0;
    RpLibrary* lib = NULL;
    Rappture::Buffer retBuf = "";
    Rappture::Outcome status;

    if (nargin != 2) {
        // wrong number of args.
        _PRINT_USAGE (who.c_str());

        retval(0) = std::string("wrong number args");
        retval(1) = 0;
        retval(2) = err;
        return retval;
    }

    if (!args(0).is_real_scalar() || !args(1).is_string()) {
        // wrong arg types
        _PRINT_USAGE (who.c_str());

        retval(0) = std::string("wrong arg types");
        retval(1) = 0;
        retval(2) = err;
        return retval;
    }

    libHandle = args(0).int_value ();
    path = args(1).string_value ();

    /* Call the C subroutine. */
    // path can be an empty string
    if ( (libHandle < 0) ) {
        // invalid libHandle
        _PRINT_USAGE (who.c_str());

        retval(0) = std::string("invalid libHandle");
        retval(1) = 0;
        retval(2) = err;
        return retval;
    }

    lib = (RpLibrary*) getObject_Void(libHandle);

    if (lib == NULL) {
        // lib was null (not found in dictionary)
        _PRINT_USAGE (who.c_str());

        retval(0) = std::string("lib was null (not found in dictionary)");
        retval(1) = 0;
        retval(2) = err;
        return retval;
    }

    retBuf = lib->getData(path);
    if (retBuf.bad()) {
        retval(0) = std::string("error reading data from library");
        retval(1) = 0;
        retval(2) = err;
        return retval;
    }

    if (Rappture::encoding::headerFlags(retBuf.bytes(),retBuf.size()) != 0) {
        // data is encoded, decode it
        if (!Rappture::encoding::decode(status, retBuf, 0)) {
            // error decoding
            err = status;
            retval(0) = std::string(status.remark());
            retval(1) = 0;
            retval(2) = err;
            return retval;
        }
    }

    uint8NDArray out(dim_vector(1,retBuf.size()));
    octave_uint8* tmp = out.fortran_vec();
    memcpy(tmp,retBuf.bytes(),retBuf.size());
    out.transpose();
    err = status;

    retval(0) = out;
    retval(1) = retBuf.size();
    retval(2) = err;

    return retval;
}
