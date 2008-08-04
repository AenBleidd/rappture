/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [err] = rpLibPutString(libHandle,path,value,append)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibPutString (libHandle,path,value,append)
/// Set the value of a node.
/**
 * Clients use this to set the value of a node.  If the path
 * is not specified, it sets the value for the root node.
 * Otherwise, it sets the value for the element specified
 * by the path.  The value is treated as the text within the 
 * tag at the tail of the path.
 *
 * If the append flag is set to 1, then the 
 * value is appended to the current value.  Otherwise, the 
 * value specified in the function call replaces the current value.
 *
 */

DEFUN_DLD (rpLibPutString, args, ,
"-*- texinfo -*-\n\
[err] = rpLibPutString(@var{libHandle},@var{path},@var{value},@var{append})\n\
\n\
Clients use this to set the value of a node.  If the @var{path}\n\
is not specified (ie. empty string ""), it sets the value for the\n\
root node.  Otherwise, it sets the value for the element specified\n\
by the path.  The @var{value} is treated as the text within the \n\
tag at the tail of the @var{path}.\n\
\n\
If the @var{append} flag is set to 1, then the \n\
@var{value} is appended to the current value.  Otherwise, the \n\
@var{value} specified in the function call replaces the current value.\n\
Error Codes: err = 0 is success, anything else is failure.")
{
    static std::string who = "rpLibPutString";

    // The list of values to return.
    octave_value_list retval;
    int err           = 1;
    int nargin        = args.length ();
    int libHandle     = 0;
    std::string path  = "";
    std::string value = "";
    std::string id    = "";
    int append        = 0;
    RpLibrary* lib    = NULL;

    if (nargin == 4) {

        if ( args(0).is_real_scalar() &&
             args(1).is_string()      &&
             args(2).is_string()      &&
             args(3).is_real_scalar()   ) {

            libHandle = args(0).int_value ();
            path      = args(1).string_value ();
            value     = args(2).string_value ();
            append    = args(3).int_value ();

            /* Call the C subroutine. */
            // the only input that has restrictions is libHandle
            // all other inputs may be empty strings or any integer value
            if ( (libHandle >= 0) ) {

                lib = (RpLibrary*) getObject_Void(libHandle);
                if (lib) {
                    lib->put(path,value,id,append);
                    err = 0;
                }
                else {
                    // lib is NULL, not found in dictionary
                }
            }
            else {
                // libHandle is negative
                print_usage (who.c_str());
            }
        }
        else {
            // wrong argument types
            print_usage (who.c_str());
        }
    }
    else {
        // wrong number of arguments
        print_usage (who.c_str());
    }

    retval(0) = err;
    return retval;
}
