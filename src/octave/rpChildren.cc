/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [nodeHandle,err] = rpChildren(libHandle,path,prevNodeHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [nodeHandle,err] = rpChildren(libHandle,path,prevNodeHandle)
/// Retrieve the children of the node described by 'path'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * children. If 'prevNodeHandle' = 0, then the 
 * first child is returned, else, the next child is retrieved.
 * If 'prevNodeHandle' is an invalid child handle, an error will be returned.
 * Subsequent calls to rpChildren() should use previously 
 * returned 'nodeHandle's for it 'prevNodeHandle' argument.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpChildren, args, ,
"-*- texinfo -*-\n\
[nodeHandle,err] = rpChildren(@var{libHandle},@var{path},@var{prevNodeHandle})\n\
\n\
Retrieve the children of the node described by @var{path} \n\
This method searches the Rappture Library Object 'libHandle' \n\
for the node at the location described by @var{path} and returns its \n\
children. If @var{prevNodeHandle} = 0, then the·\n\
first child is returned, else, the next child is retrieved.\n\
If @var{prevNodeHandle} is an invalid child handle, an error will be \n\
returned. Subsequent calls to @code{rpChildren} should use \n\
previously returned 'nodeHandle's for it var{prevNodeHandle}. \n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpChildren";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    int prevNodeIndex = 0;
    int libIndex = 0;
    RpLibrary* lib = NULL;
    RpLibrary* prevLib = NULL;
    RpLibrary* newLib = NULL;

    if (nargin == 3) {

        if (    args(0).is_real_scalar () &&
                args(1).is_string      () &&
                args(2).is_real_scalar ()   ) {

            libIndex = args(0).int_value ();
            path = args(1).string_value ();
            prevNodeIndex = args(2).int_value ();

            /* Call the C subroutine. */
            if (    (libIndex != 0)      &&
                    !path.empty()        &&
                    (prevNodeIndex != 0)    ) {

                lib = getObject_Lib(libIndex);
                prevLib = getObject_Lib(prevNodeIndex);

                if (lib) {
                    newLib = lib->children(path,prevLib);
                    err = 0;
                }

            }
        }
        else {
            print_usage ("rpChildren");
        }
    }
    else {
        print_usage ("rpChildren");
    }

    retval(0) = storeObject_Lib(newLib);
    retval(1) = err;
    return retval;
}
