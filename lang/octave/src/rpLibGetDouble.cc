/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpLibGetDouble(libHandle,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpLibGetDouble(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. The return value is returned as a double.
 *
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibGetDouble, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpLibGetDouble(@var{libHandle},@var{path})\n\
\n\
Clients use this to query the value of a node.  If the path\n\
is not specified, it returns the value associated with the\n\
root node.  Otherwise, it returns the value for the element\n\
specified by the path. The return value is returned as a double.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibGetDouble";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    int libHandle = 0;
    RpLibrary* lib = NULL;
    double retVal = 0;

    if (nargin == 2) {

        if (    args(0).is_real_scalar () &&
                args(1).is_string      ()   ) {

            libHandle = args(0).int_value ();
            path = args(1).string_value ();

            /* Call the C subroutine. */
            // path can be an empty string
            if ( (libHandle >= 0) ) {

                lib = (RpLibrary*) getObject_Void(libHandle);

                if (lib) {
                    retVal = lib->getDouble(path);
                    err = 0;
                }
                else {
                    // lib was null (not found in dictionary)
                }
            }
            else {
                // invalid libHandle
                _PRINT_USAGE (who.c_str());
            }
        }
        else {
            // wrong arg types
            _PRINT_USAGE (who.c_str());
        }
    }
    else {
        // wrong number of args.
        _PRINT_USAGE (who.c_str());
    }

    retval(0) = retVal;
    retval(1) = err;
    return retval;
}
