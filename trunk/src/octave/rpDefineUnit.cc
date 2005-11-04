/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [unitsHandle,err] = rpDefineUnit(unitSymbol, basisHandle)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [unitsHandle,err] = rpDefineUnit(unitSymbol, basisHandle)
/// Define a new Rappture Units type.
/**
 * Define a new Rappture Units type which can be searched for using
 * @var{unitSymbol} and has a basis of @var{basisHandle}. Because of
 * the way the Rappture Units module parses unit names, complex units must
 * be defined as multiple basic units. See the RpUnits Howto for more
 * information on this topic. A @var{basisHandle} equal to 0 means that
 * the unit being defined should be considered as a basis. Unit names must
 * not be empty strings.
 *
 * The first return value, @var{unitsHandle} represents the handle of the
 * instance of the RpUnits object inside the internal dictionary. On
 * success this value will be greater than zero (0), any other value is
 * represents failure within the function. The second return value
 * @var{err} represents the error code returned from the function.
 *
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpDefineUnit, args, ,
"-*- texinfo -*-\n\
[unitsHandle,err] = rpDefineUnit(@var{unitSymbol},@var{basisHandle})\n\
\n\
Define a new Rappture Units type which can be searched for using\n\
@var{unitSymbol} and has a basis of @var{basisHandle}. Because of\n\
the way the Rappture Units module parses unit names, complex units must\n\
be defined as multiple basic units. See the RpUnits Howto for more\n\
information on this topic. A @var{basisHandle} equal to 0 means that\n\
the unit being defined should be considered as a basis. Unit names must\n\
not be empty strings.\n\
\n\
The first return value, @var{unitsHandle} represents the handle of the\n\
instance of the RpUnits object inside the internal dictionary. On\n\
success this value will be greater than zero (0), any other value is\n\
represents failure within the function. The second return value\n\
@var{err} represents the error code returned from the function.\n\
\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpDefineUnit";

    // The list of values to return.
    octave_value_list retval;
    int err                  = 1;
    int nargin               = args.length ();
    std::string unitSymbol   = "";
    int basisHandle          = 0;
    RpUnits* newUnit         = NULL;
    const RpUnits* basisUnit = NULL;

    if (nargin == 2) {

        if (    args(0).is_string      () &&
                args(2).is_real_scalar ()    ) {

            unitSymbol  = args(0).string_value ();
            basisHandle = args(2).int_value ();

            // Call the C++ subroutine.
            if ( !unitSymbol.empty() ) {

                basisUnit = getObject_UnitsStr(basisHandle);

                if (basisUnit) {
                    newUnit = RpUnits::define(unitSymbol,basisUnit);
                    if (newUnit) {
                        err = 0;
                    }
                }
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

    retval(0) = storeObject_UnitsStr(newUnit->getUnitsName());
    retval(1) = err;
    return retval;
}
