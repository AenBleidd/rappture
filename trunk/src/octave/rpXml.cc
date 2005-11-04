/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpXml(libHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpXml (libHandle)
/// Returns the xml from the Rappture Object represented by 'libHandle'.
/**
 * Usually called by user when they need to retrieve the character 
 * representation of the xml being stored in the Rappture Library Object
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpXml, args, ,
"-*- texinfo -*-\n\
[retText, err] = rpXml (@var{libHandle})\n\
\n\
Returns the xml text of the Rappture Object represented by the\n\
handle @var{libHandle}. Also returns an error code,\n\
err = 0 is success, anything else is failure.")
{
    static std::string who = "rpXml";

    // The list of values to return.
    octave_value_list retval;

    int nargin = args.length ();
    std::string output_buf = "";
    RpLibrary* lib = NULL;
    int libIndex = 0;

    retval(0) = "";
    retval(1) = 1;

    if (nargin == 1) {
        if (args(0).is_real_scalar ()) {
            libIndex = args(0).int_value ();
            if (libIndex > 0) {
                lib = getObject_Lib(libIndex);
                if (lib) {
                    output_buf = lib->xml();
                    if (output_buf.length() > 0) {
                        retval(1) = 0;
                    }
                    retval(0) = output_buf;
                }
            }
        }
    }
    else {
        print_usage ("rpXml");
    }

  return retval;
}
