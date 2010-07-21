/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpLibNodeType(nodeHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpLibNodeType(nodeHandle)
/// Return the type name of the node represented by 'nodeHandle'
/**
 * This method returns the type name of the node represented 
 * by 'nodeHandle'.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibNodeType, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpLibNodeType(@var{nodeHandle})\n\
\n\
This method returns the type name of the node represented \n\
by @var{nodeHandle}.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibNodeType";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    int libHandle = 0;
    RpLibrary* lib = NULL;
    std::string retStr = "";

    if (nargin == 1) {

        if ( args(0).is_real_scalar() ) { 

            libHandle = args(0).int_value ();

            /* Call the C subroutine. */
            if ( (libHandle >= 0) ) {

                lib = (RpLibrary*) getObject_Void(libHandle);
                if (lib) {
                    retStr = lib->nodeType();
                    err = 0;
                }
                else {
                    // lib is NULL, not found in dictionary
                }
            }
            else {
                // libHandle is negative
                _PRINT_USAGE (who.c_str());
            }
        }
        else {
            // wrong argument types
            _PRINT_USAGE (who.c_str());
        }
    }
    else {
        // wrong number of arguments
        _PRINT_USAGE (who.c_str());
    }

    retval(0) = retStr;
    retval(1) = err;
    return retval;
}
