/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    libHandle = rpLib(fileName)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

DEFUN_DLD (rpLib, args, ,
  "[libHandle, err] = rpLib (fileName)\n\
\n\
Opens a Rappture Library Object based on the xml file 'fileName'.\n\
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

    retval(1) = 1;
    retval(0) = -1;

    if (nargin == 1) {
        if (args(0).is_string ()) {
            path = args(0).string_value ();
            if (!path.empty()) {
                lib = new RpLibrary(path);
                retval(1) = 0;
            }
        }
    }
    else {
        _PRINT_USAGE ("fopen");
    }

    retval(0) = storeObject_Void((void*)lib);

  return retval;
}
