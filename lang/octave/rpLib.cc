/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [libHandle,err] = rpLib(fileName)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [libHandle,err] = rpLib (fileName)
/// Opens a Rappture Library Object based on the xml file 'fileName'.
/**
 * Usually called in the beginning of a program to load an xml file
 * for future reading.
 */

DEFUN_DLD (rpLib, args, ,
"-*- texinfo -*-\n\
[libHandle, err] = rpLib (@var{fileName})\n\
\n\
Opens a Rappture Library Object based on the xml file @var{fileName}.\n\
Returns a handle to the Rappture Library Object and an error flag.\n\
err = 0 is success, anything else is failure.")
{
    static std::string who = "rpLib";

    // The list of values to return.
    octave_value_list retval;

    int nargin = args.length ();
    std::string path = "";
    RpLibrary* lib = NULL;
    int libIndex = 0;
    int err = 1;

    retval(0) = -1;
    retval(1) = err;

    if (nargin == 1) {
        if (args(0).is_string ()) {
            path = args(0).string_value ();
            lib = new RpLibrary(path);
            if (lib != NULL) {
                err = 0;
            }
        }
    }
    else {
        print_usage ("rpLib");
    }

    retval(0) = storeObject_Void((void*)lib);
    retval(1) = err;

  return retval;
}
