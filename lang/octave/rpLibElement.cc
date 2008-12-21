/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retVal,err] = rpLibElement(libHandle,path)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retVal,err] = rpLibElement(libHandle,path)
/// Return a handle to the element at location 'path' in 'libHandle'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns
 * a handle to it.
 *
 * If path is an empty string, the root of the node is used. 'libHandle'
 * is the handle representing the instance of the RpLibrary object.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpLibElement, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpLibElement(@var{libHandle},@var{path})\n\
\n\
This method searches the Rappture Library Object @var{libHandle} for the\n\
node at the location described by the path @var{path} and returns\n\
a handle to it.\n\
If path is an empty string, the root of the node is used. @var{libHandle}\n\
is the handle representing the instance of the RpLibrary object.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpLibElement";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    int libHandle = 0;
    RpLibrary* lib = NULL;
    RpLibrary* eleLib = NULL;

    if (nargin == 2) {

        if (    args(0).is_real_scalar () &&
                args(1).is_string      ()   ) {

            libHandle = args(0).int_value ();
            path = args(1).string_value ();

            /* Call the C subroutine. */
            // path can be an empty string
            if ( (libHandle != 0) ) {

                lib = (RpLibrary*) getObject_Void(libHandle);

                if (lib) {
                    eleLib = lib->element(path);
                    err = 0;
                }
                else {
                    //error message
                } 
            }
            else {
                _PRINT_USAGE (who.c_str());
            }
        }
        else {
            _PRINT_USAGE (who.c_str());
        }
    }
    else {
        _PRINT_USAGE (who.c_str());
    }

    retval(0) = storeObject_Void((void*)eleLib);
    retval(1) = err;
    return retval;
}
