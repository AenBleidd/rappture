/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retVal,err] = rpConvertDbl(fromVal, toUnitsName)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retVal,err] = rpConvertDbl(fromVal,toUnitsName)
/// Convert between RpUnits return a double value without units
/**
 * Convert the value and units in the string @var{fromVal} to units specified
 * in string @var{toUnitsName}. Units will not be shown in @var{retVal}.
 * The second return value @var{err} specifies whether there was an error
 * during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpConvertDbl, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpConvertDbl(@var{fromVal},@var{toUnitsName})\n\
\n\
Convert the value and units in the string @var{fromVal} to units specified\n\
in string @var{toUnitsName}. Units will not be shown in @var{retVal}.\n\
The second return value @var{err} specifies whether there was an error\n\
during conversion. \n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpConvertDbl";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string fromVal = "";
    std::string toUnitsName = "";
    int showUnits = 0;
    std::string retStr = "";
    double retVal = 0;

    if (nargin == 2) {

        if (    args(0).is_string      () &&
                args(1).is_string      ()   ) {

            fromVal = args(0).string_value ();
            toUnitsName = args(1).string_value ();

            // Call the C++ subroutine.
            // we allow toUnitsName to be an empty string
            // to let the user not change the units of fromVal
            // but still remove the units from being shown.
            if ( !fromVal.empty() ) {

                retStr = RpUnits::convert(fromVal,toUnitsName,showUnits,&err);
                if ( !err && !retStr.empty() ) {
                    retVal = atof(retStr.c_str());
                }
            }
            else {
                print_usage ("rpConvertDbl");
            }
        }
        else {
            print_usage ("rpConvertDbl");
        }
    }
    else {
        print_usage ("rpConvertDbl");
    }

    retval(0) = retVal;
    retval(1) = err;
    return retval;
}
