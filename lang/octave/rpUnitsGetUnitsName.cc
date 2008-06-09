/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpUnitsGetUnitsName(unitHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpUnitsGetUnitsName(unitHandle)
/// Return the unit and exponent of the Rappture Unit represented by unitHandle.
/**
 * Retrieve the unit and exponent of the Rappture Units object with 
 * the handle 'unitHandle'.
 * Return the unit and exponent as one concatinated string.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpUnitsGetUnitsName, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpUnitsGetUnitsName(@var{unitHandle})\n\
\n\
Retrieve the unit and exponent of the Rappture Units object with \n\
the handle 'unitHandle'.\n\
Return the unit and exponent as one concatinated string.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpUnitsGetUnitsName";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();

    const RpUnits* myUnit = NULL;
    int unitHandle = 0;
    std::string retStr = "";

    if (nargin == 1) {

        if ( args(0).is_real_scalar() ) {

            unitHandle = args(0).int_value ();

            /* Call the C subroutine. */
            if ( unitHandle >= 0 ) {

                // get the original unit
                myUnit = getObject_UnitsStr(unitHandle);
                if (myUnit) {
                    // get the basis
                    retStr = myUnit->getUnitsName();
                    // adjust error code
                    err = 0;
                }
            }
            else {
                // invalid unitHandle
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
