/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Progress Source
 *
 *    [err] = rpUtilsProgress(percent,message)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2007
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpUtilsProgress (percent,message)
/// Print the Rappture Progress message.
/**
 * Clients use this to generate the super secret Rappture
 * Progress message, and generate a progress bar in the
 * Rappture graphical user interface.
 */

DEFUN_DLD (rpUtilsProgress, args, ,
"-*- texinfo -*-\n\
[err] = rpUtilsProgress(@var{percent},@var{message})\n\
\n\
Clients use this to generate the super secret Rappture\n\
Progress message, and generate a progress bar in the\n\
Rappture graphical user interface.")
{
    static std::string who = "rpUtilsProgress";

    // The list of values to return.
    octave_value_list retval;
    int err           = 1;
    int nargin        = args.length ();
    int percent       = 0;
    std::string message  = "";

    if (nargin == 2) {
        if ( args(0).is_real_scalar() &&
             args(1).is_string()   ) {

            percent   = args(0).int_value ();
            message   = args(1).string_value ();

            /* Call the C subroutine. */
            err = Rappture::Utils::progress(percent,message.c_str());
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

    retval(0) = err;
    return retval;
}
