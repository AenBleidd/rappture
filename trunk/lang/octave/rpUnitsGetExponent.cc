/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [retVal,err] = rpUnitsGetExponent(unitHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retVal,err] = rpUnitsGetExponent(unitHandle)
/// Return the exponent of the Rappture Unit represented by unitHandle.
/**
 * Retrieve the exponent of the Rappture Units object with the handle 
 * 'unitHandle'. Return the exponent as a double.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpUnitsGetExponent, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpUnitsGetExponent(@var{unitHandle})\n\
\n\
Retrieve the exponent of the Rappture Units object with the handle \n\
'unitHandle'. Return the exponent as a double.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpUnitsGetExponent";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();

    const RpUnits* myUnit = NULL;
    int unitHandle = 0;
    double exponent = 0;

    if (nargin == 1) {

        if ( args(0).is_real_scalar() ) {

            unitHandle = args(0).int_value ();

            /* Call the C subroutine. */
            if ( unitHandle >= 0 ) {

                // get the original unit
                myUnit = getObject_UnitsStr(unitHandle);
                if (myUnit) {
                    // get the basis
                    exponent = myUnit->getExponent();
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

    retval(0) = exponent;
    retval(1) = err;
    return retval;
}
