/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [nodeHandle,err] = rpChildrenByType(libHandle,path,prevNodeHandle,type)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [nodeHandle,err] = rpChildrenByType(libHandle,path,prevNodeHandle,type)
/// Retrieve the children of the node described by 'path' of type 'type'
/**
 * This method searches the Rappture Library Object 'libHandle' for the
 * node at the location described by the path 'path' and returns its
 * children that match the type 'type'. If 'prevNodeHandle' = 0, then the 
 * first child is returned, else, the next child is retrieved.
 * If 'prevNodeHandle' is an invalid child handle, an error will be returned.
 * Subsequent calls to rpChildrenByType() should use previously 
 * returned 'nodeHandle's for it 'prevNodeHandle' argument.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpChildrenByType, args, ,
"-*- texinfo -*-\n\
[nodeHandle,err] = rpChildrenByType(@var{libHandle},@var{path},@var{prevNodeHandle},@var{type})\n\
\n\
Retrieve the children of the node described by @var{path} of type\n\
@var{type} This method searches the Rappture Library Object 'libHandle' \n\
for the node at the location described by @var{path} and returns its \n\
children that match @var{type}. If @var{prevNodeHandle} = 0, then the·\n\
first child is returned, else, the next child is retrieved.\n\
If @var{prevNodeHandle} is an invalid child handle, an error will be \n\
returned. Subsequent calls to @code{rpChildrenByType} should use \n\
previously returned 'nodeHandle's for it var{prevNodeHandle}. \n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpChildrenByType";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string path = "";
    std::string type = "";
    int prevNodeIndex = 0;
    int libIndex = 0;
    RpLibrary* lib = NULL;
    RpLibrary* prevLib = NULL;
    RpLibrary* newLib = NULL;

    retval(0) = 1;

    if (nargin == 4) {

        if (    args(0).is_real_scalar () &&
                args(1).is_string      () &&
                args(2).is_real_scalar () &&
                args(3).is_string      ()       ) {

            libIndex = args(0).int_value ();
            path = args(1).string_value ();
            prevNodeIndex = args(2).int_value ();
            type = args(3).string_value ();

            /* Call the C subroutine. */
            if (    (libIndex != 0)      &&
                    !path.empty()        &&
                    (prevNodeIndex != 0) &&
                    !type.empty()           ) {

                lib = getObject_Lib(libIndex);
                prevLib = getObject_Lib(prevNodeIndex);

                if (lib) {
                    newLib = lib->children(path,prevLib,type);
                    err = 0;
                }

            }
        }
        else {
            print_usage ("rpChildrenByType");
        }
    }
    else {
        print_usage ("rpChildrenByType");
    }

    retval(0) = storeObject_Lib(newLib);
    retval(1) = err;
    return retval;
}
