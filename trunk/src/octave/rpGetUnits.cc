/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retStr,err] = rpGetUnits(unitHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpGetUnits(unitHandle)
/// Return the units of the Rappture Unit represented by unitHandle.
/**
 * Retrieve the units of the Rappture Units object with the handle 
 * 'unitHandle'. Note this does not include the exponent.
 * For units and exponent in one string see rpGetUnitsName().
 * Return the units as a string.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpGetUnits, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpGetUnits(@var{unitHandle})\n\
\n\
Retrieve the units of the Rappture Units object with the handle \n\
'unitHandle'. Note this does not include the exponent.\n\
For units and exponent in one string see rpGetUnitsName().\n\
Return the units as a string.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpGetUnits";

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
                    retStr = myUnit->getUnits();
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
