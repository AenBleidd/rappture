/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [err] = rpUnitsMakeMetric(basisHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [err] = rpUnitsMakeMetric(basisHandle)
/// Create metric extensions for the Rappture Unit referenced by basisHandle
/**
 * Create the metric extensions for the Rappture Unit, referenced 
 * by basisHandle, within Rappture's internal units dictionary.
 * Return an error code, err, to specify success or failure.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpUnitsMakeMetric, args, ,
"-*- texinfo -*-\n\
[err] = rpUnitsMakeMetric(@var{basisHandle})\n\
\n\
Create the metric extensions for the Rappture Unit, referenced \n\
by @var{basisHandle}, within Rappture's internal units dictionary.\n\
Return an error code, @var{err}, to specify success or failure.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpUnitsMakeMetric";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();

    const RpUnits* myUnit = NULL;
    int unitHandle = 0;

    if (nargin == 1) {

        if ( args(0).is_real_scalar() ) {

            unitHandle = args(0).int_value ();

            /* Call the C subroutine. */
            if ( unitHandle >= 0 ) {

                // get the original unit
                myUnit = getObject_UnitsStr(unitHandle);
                if (myUnit) {
                    // make the metric extensions.
                    err = RpUnits::makeMetric(myUnit);
                    // adjust error code, because makeMetric returns 
                    // 0 on failure and 1 on success. need to fix this 
                    // in RpUnits.cc
                    if (err) {
                        err = 0;
                    }
                }
            }
            else {
                // invalid unitHandle
                _PRINT_USAGE (who.c_str());
            }
        }
        else {
            _PRINT_USAGE (who.c_str());
        }
    }
    else {
        _PRINT_USAGE (who.c_str());
    }

    retval(0) = err;
    return retval;
}
