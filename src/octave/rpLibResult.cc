/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [err] = rpLibResult(libHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibResult (libHandle)
/// Write Rappture Library to run.xml and signal end of processing.
/** 
 * Usually the last call of the program, this function signals to the gui
 * telling it that processing has completed and the output is ready to be
 * displayed
 */

DEFUN_DLD (rpLibResult, args, ,
"-*- texinfo -*-\n\
[err] = rpLibResult (@var{libHandle})\n\
\n\
Usually the last call of the program, this function signals to the gui\n\
telling it that processing has completed and the output is ready to be\n\
displayed\n\
Error Codes: @var{err} = 0 is success, anything else is failure.")
{
    static std::string who = "rpLibResult";

    // The list of values to return.
    octave_value_list retval;

    int nargin = args.length ();
    RpLibrary* lib = NULL;
    int libHandle = 0;
    int err = 0;

    if (nargin == 1) {
        if (args(0).is_real_scalar ()) {
            libHandle= args(0).int_value ();
            /* Call the C subroutine. */
            if (libHandle > 0) {
                lib = getObject_Lib(libHandle);
                if (lib) {
                    lib->result();
                    // cleanLibDict();
                    err = 0;
                }
            }
        }
        else {
            // wrong argument type
            print_usage ("rpLib");
        }
    }
    else {
        // wrong number of arguments
        print_usage ("rpLib");
    }

    retval(0) = err;

  return retval;
}
