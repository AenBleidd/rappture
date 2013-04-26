/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpLibGetString(libHandle,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpLibGetString(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. Values are returned as strings.
 *
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibGetString, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpLibGetString(@var{libHandle},@var{path})\n\
\n\
Clients use this to query the value of a node.  If the path\n\
is not specified, it returns the value associated with the\n\
root node.  Otherwise, it returns the value for the element\n\
specified by the path. Values are returned as strings.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibGetString";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int argc;
    std::string retStr = "";

    argc = args.length();
    if (argc == 2) {
        if ((args(0).is_real_scalar()) && (args(1).is_string())) {
	    std::string path;
	    int libHandle;

            libHandle = args(0).int_value ();

            path = args(1).string_value ();
            /* Call the C subroutine. */
            // path can be an empty string
            if (libHandle >= 0) {
		RpLibrary* lib;

                lib = (RpLibrary*) getObject_Void(libHandle);
                if (lib != NULL) {
		    retval(0) = lib->getString(path);
		    retval(1) = 0;
		    return retval;
                }
            } else {
                // invalid libHandle
                _PRINT_USAGE(who.c_str());
            }
        } else {
            // wrong arg types
            _PRINT_USAGE(who.c_str());
        }
    } else {
        // wrong number of args.
        _PRINT_USAGE(who.c_str());
    }
    retval(0) = retStr;
    retval(1) = 1;
    return retval;
}
