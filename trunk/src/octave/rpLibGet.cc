/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpLibGet(libHandle,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpLibGet(libHandle,path)
/// Query the value of a node.
/**
 * Clients use this to query the value of a node.  If the path
 * is not specified, it returns the value associated with the
 * root node.  Otherwise, it returns the value for the element
 * specified by the path. Values are returned as strings.
 *
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibGet, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpLibGet(@var{libHandle},@var{path})\n\
\n\
Clients use this to query the value of a node.  If the path\n\
is not specified, it returns the value associated with the\n\
root node.  Otherwise, it returns the value for the element\n\
specified by the path. Values are returned as strings.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibGet";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    int libHandle = 0;
    RpLibrary* lib = NULL;
    std::string retStr = "";

    if (nargin == 2) {

        if (    args(0).is_real_scalar () &&
                args(1).is_string      ()   ) {

            libHandle = args(0).int_value ();
            path = args(1).string_value ();

            /* Call the C subroutine. */
            // path can be an empty string
            if ( (libHandle >= 0) ) {

                lib = getObject_Lib(libHandle);

                if (lib) {
                    retStr = lib->get(path);
                    err = 0;
                }
                else {
                    // lib was null (not found in dictionary)
                }
            }
            else {
                // invalid libHandle
                print_usage (who.c_str());
            }
        }
        else {
            // wrong arg types
            print_usage (who.c_str());
        }
    }
    else {
        // wrong number of args.
        print_usage (who.c_str());
    }

    retval(0) = retStr;
    retval(1) = err;
    return retval;
}
