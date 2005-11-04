/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpConvert(fromVal, toUnitsName, showUnits)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpConvert(fromVal,toUnitsName,showUnits)
/// Convert between RpUnits return a string value with or without units
/**
 * Convert the value and units in the string @var{fromVal} to units specified
 * in string @var{toUnitsName}. If @var{showUnits} is set to 1, then show the
 * units in the returned string @var{retStr}, else leave the units off.
 * The second return value @var{err} specifies whether there was an error
 * during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpConvert, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpConvert(@var{fromVal},@var{toUnitsName},@var{showUnits})\n\
\n\
Convert the value and units in the string @var{fromVal} to units specified\n\
in string @var{toUnitsName}. If @var{showUnits} is set to 1, then show the\n\
units in the returned string @var{retStr}, else leave the units off.\n\
The second return value @var{err} specifies whether there was an error\n\
during conversion. \n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpConvert";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string fromVal = "";
    std::string toUnitsName = "";
    int showUnits = 0;
    std::string retStr = "";

    if (nargin == 3) {

        if (    args(0).is_string      () &&
                args(1).is_string      () &&
                args(2).is_real_scalar ()    ) {

            fromVal = args(0).string_value ();
            toUnitsName = args(1).string_value ();
            showUnits = args(2).int_value ();

            // Call the C++ subroutine.
            // we allow toUnitsName to be an empty string
            // to let the user not change the units of fromVal
            // but still remove the units from being shown.
            if ( !fromVal.empty() ) {

                retStr = RpUnits::convert(fromVal,toUnitsName,showUnits,&err);
            }
            else {
                print_usage (who.c_str());
            }
        }
        else {
            print_usage (who.c_str());
        }
    }
    else {
        print_usage (who.c_str());
    }

    retval(0) = retStr;
    retval(1) = err;
    return retval;
}
