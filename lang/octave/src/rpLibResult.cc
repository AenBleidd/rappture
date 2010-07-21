/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [err] = rpLibResult(libHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2008
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpLibResult (libHandle,status)
/// Write Rappture Library to run.xml and signal end of processing.
/** 
 * Usually the last call of the program, this function signals to the gui
 * that processing has completed and the output is ready to be
 * displayed
 */

DEFUN_DLD (rpLibResult, args, ,
"-*- texinfo -*-\n\
[err] = rpLibResult (@var{libHandle},@var{status})\n\
\n\
Usually the last call of the program, this function signals to the gui\n\
that processing has completed and the output is ready to be\n\
displayed\n\
Error Codes: @var{err} = 0 is success, anything else is failure.")
{
    static std::string who = "rpLibResult";

    // The list of values to return.
    octave_value_list retval;

    int nargin = args.length();
    RpLibrary* lib = NULL;
    int libHandle = 0;
    int status = 0;
    int err = 0;

    if ((nargin < 1) || (nargin > 2)) {
        // wrong number of arguments
        _PRINT_USAGE ("rpLibResult");
        goto done;
    }

    if (! args(0).is_real_scalar()) {
        // wrong argument type
        _PRINT_USAGE ("rpLibResult");
        goto done;
    }

    libHandle = args(0).int_value();

    if (nargin == 2) {
        if (! args(1).is_real_scalar()) {
            // wrong argument type
            _PRINT_USAGE ("rpLibResult");
            goto done;
        }

        status = args(1).int_value();
    }

    /* Call the C subroutine. */
    if (libHandle > 0) {
        lib = (RpLibrary*) getObject_Void(libHandle);
        if (lib) {
            lib->put("tool.version.rappture.language", "octave");
            lib->result(status);
            // cleanLibDict();
            err = 0;
        }
    }

done:
    retval(0) = err;
    return retval;
}
