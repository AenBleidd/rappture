/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [err] = rpLibPutFile(libHandle,path,fileName,compress,append)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2007
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibPutFile (libHandle,path,fileName,fileType,append)
/// Set the value of a node.
/**
 * Clients use this to set the value of a node.  If the path
 * is not specified, it sets the value for the root node.
 * Otherwise, it sets the value for the element specified
 * by the path.  The value is treated as the text within the 
 * tag at the tail of the path.
 *
 * FileName is the name of the file to import into the rappture object
 * Compress is an integer telling if the provided data should be compressed
 * (use 1) or left uncompressed (use 0).
 * Append is an integer telling if this new data should overwrite
 * (use 0) or be appended (use 1) to previous data in this node.
 *
 */

DEFUN_DLD (rpLibPutFile, args, ,
"-*- texinfo -*-\n\
[err] = rpLibPutFile(@var{libHandle},@var{path},@var{fileName},@var{compress},@var{append})\n\
\n\
Clients use this to set the value of a node.  If the @var{path}\n\
is not specified (ie. empty string ""), it sets the value for the\n\
root node.  Otherwise, it sets the value for the element specified\n\
by the path.  The added data is treated as the text within the \n\
tag at the tail of the @var{path}.\n\
\n\
@var{fileName} is the name of the file to import into the rappture object\n\
@var{compress} is an integer telling if the provided data should be compressed\n\
(use 1) or left uncompressed (use 0).\n\
@var{append} is an integer telling if this new data should overwrite\n\
(use 0) or be appended (use 1) to previous data in this node.\n\
\n\
Error Codes: err = 0 is success, anything else is failure.")
{
    static std::string who = "rpLibPutFile";

    // The list of values to return.
    octave_value_list retval;
    int err               = 1;
    int nargin            = args.length ();
    int libHandle         = 0;
    std::string path      = "";
    std::string fileName  = "";
    unsigned int compress = 0;
    unsigned int append   = 0;
    RpLibrary* lib        = NULL;

    if (nargin == 5) {

        if ( args(0).is_real_scalar() &&
             args(1).is_string()      &&
             args(2).is_string()      &&
             args(3).is_real_scalar() &&
             args(4).is_real_scalar()   ) {

            libHandle = args(0).int_value ();
            path      = args(1).string_value ();
            fileName  = args(2).string_value ();
            compress  = (unsigned int) args(3).int_value ();
            append    = (unsigned int) args(4).int_value ();

            /* Call the C subroutine. */
            // the only input that has restrictions is libHandle
            // all other inputs may be empty strings or any integer value
            if ( (libHandle >= 0) ) {

                lib = (RpLibrary*) getObject_Void(libHandle);
                if (lib) {
                    lib->putFile(path,fileName,compress,append);
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
