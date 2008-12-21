/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Octave Rappture Library Source
 *
 *    [unitHandle,err] = rpUnitsFind(unitSymbol)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [unitHandle,err] = rpUnitsFind(unitSymbol)
/// Search the dictionary of Rappture Units for an instance named 'unitSymbol'
/**
 * This method searches the Rappture Units Dictionary for the
 * RpUnit named 'unitSymbol'. If it is found, its handle is 
 * returned, as a non-negative integer, to the caller for further use,
 * else a negative integer is returned signifying no unit of that 
 * name was found.
 * If unitSymbol is an empty string, unitHandle will be set to zero and err
 * will be set to a positive value.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpUnitsFind, args, ,
"-*- texinfo -*-\n\
[unitHandle,err] = rpUnitsFind(@var{unitSymbol})\n\
\n\
This method searches the Rappture Units Dictionary for the\n\
RpUnit named @var{unitSymbol}. If it is found, its handle is \n\
returned, as a non-negative integer, to the caller for further use,\n\
else a negative integer is returned signifying no unit of that \n\
name was found.\n\
If @var{unitSymbol} is an empty string, @var{unitHandle} will be set to\n\
zero and @var{err} will be set to a positive value.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpUnitsFind";

    // The list of values to return.
    octave_value_list retval;
    int err = 1;
    int nargin = args.length ();
    std::string unitSymbol = "";
    int retHandle = -1;
    const RpUnits* retUnit = NULL;

    if (nargin == 1) {

        if ( args(0).is_string() ) {

            unitSymbol = args(0).string_value ();

            /* Call the C subroutine. */
            if ( !unitSymbol.empty() ) {

                retUnit = RpUnits::find(unitSymbol);
                // store the returned unit
                if (retUnit) {
                    // store the unit
                    retHandle = storeObject_UnitsStr(retUnit->getUnitsName());
                    // adjust error code
                    if (retHandle >= 0) {
                        err = 0;
                    }
                }
            }
            else {
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

    retval(0) = retHandle;
    retval(1) = err;
    return retval;
}
