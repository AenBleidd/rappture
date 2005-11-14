/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [basisHandle,err] = rpUnitsGetBasis(unitHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [basisHandle,err] = rpUnitsGetBasis(unitHandle)
/// Return a handle to the basis of the provided instance of a Rappture Unit.
/**
 * Retrieve the basis of the Rappture Units object with the handle 
 * 'unitHandle'. Return the handle of the basis if it exists. If there 
 * is no basis, then return a negative integer.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpUnitsGetBasis, args, ,
"-*- texinfo -*-\n\
[basisHandle,err] = rpUnitsGetBasis(@var{unitHandle})\n\
\n\
Retrieve the basis of the Rappture Units object with the handle \n\
'unitHandle'. Return the handle of the basis if it exists. If there \n\
is no basis, then return a negative integer.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpUnitsGetBasis";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();

    const RpUnits* myBasis = NULL;
    const RpUnits* myUnit = NULL;
    int retHandle = -1;
    int unitHandle = 0;

    if (nargin == 1) {

        if ( args(0).is_real_scalar() ) {

            unitHandle = args(0).int_value ();

            /* Call the C subroutine. */
            if ( unitHandle >= 0 ) {

                // get the original unit
                myUnit = getObject_UnitsStr(unitHandle);
                if (myUnit) {
                    // get the basis
                    myBasis = myUnit->getBasis();
                    if (myBasis) {
                        // store the basis
                        retHandle = 
                            storeObject_UnitsStr(myBasis->getUnitsName());
                    }

                    // adjust error code
                    // if there is a basis retHandle will be a positive value
                    // if storing was successful. retHandle will be zero if
                    // storing failed. 
                    // if there is no basis, myBasis will be null and
                    // retHandle will not change from its original negative 
                    // value
                    if (retHandle != 0) {
                        err = 0;
                    }
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

    retval(0) = retHandle;
    retval(1) = err;
    return retval;
}
