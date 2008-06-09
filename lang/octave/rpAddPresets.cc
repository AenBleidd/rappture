/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Units Source
 *
 *    [err] = rpAddPresets (presetName)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpAddPresets (presetName)
/// Adds the units associated with the group 'presetName' to the list of
/// available units within the Rappture Units Module.
/**
 * Usually called in the beginning of the program to seed the Rappture Units
 * Module with default units. Available valid presets include 'all',
 * 'energy', 'length', 'temp', 'time', 'volume'. To find out more 
 * check out the Rappture Units Howto.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpAddPresets, args, ,
"-*- texinfo -*-\n\
[err] = rpAddPresets (@var{presetName})\n\
\n\
Adds the units associated with the group @var{presetName} to the list of\n\
available units within the Rappture Units Module.\n\
Usually called in the beginning of the program to seed the Rappture Units\n\
Module with default units. Available valid presets include 'all',\n\
'energy', 'length', 'temp', 'time', 'volume'. To find out more\n\
check out the Rappture Units Howto.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpAddPresets";

    // The list of values to return.
    octave_value_list retval;

    int nargin = args.length ();
    std::string presetName = "";
    RpLibrary* lib = NULL;
    int libIndex = 0;

    retval(0) = 1;

    if (nargin == 1) {
        if (args(0).is_string ()) {
            presetName = args(0).string_value ();
            /* Call the C subroutine. */
            if ( !presetName.empty() ) {
                retval(0) = RpUnits::addPresets(presetName);
            }
        }
    }
    else {
        print_usage ("rpAddPresets");
    }

  return retval;
}
