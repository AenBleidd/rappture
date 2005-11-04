/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retVal,err] = rpConvertObjDbl(fromObjHandle, toObjHandle, value)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retVal,err] = rpConvertObjDbl(fromObjHandle,toObjHandle,value)
/// Convert between RpUnits return a double value without units
/**
 * Convert @var{value} from the units represented by the RpUnits object
 * @var{fromObjHandle} to the units represented by the RpUnits object
 * @var{toObjHandle}. On success, the converted value is returned through
 * @var{retVal}. The second return value, @var{err}, specifies whether 
 * there was an error during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpConvertObjDbl, args, ,
"-*- texinfo -*-\n\
[retVal,err] = rpConvertObjDbl(@var{fromObjHandle},@var{toObjHandle},@var{value})\n\
\n\
Convert @var{value} from the units represented by the RpUnits object\n\
@var{fromObjHandle} to the units represented by the RpUnits object\n\
@var{toObjHandle}. On success, the converted value is returned through\n\
@var{retVal}. The second return value, @var{err}, specifies whether \n\
there was an error during conversion.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpConvertObjDbl";

    // The list of values to return.
    octave_value_list retval;

    int err            = 1;
    int nargin         = args.length ();
    int fromObjHandle  = 0;
    int toObjHandle    = 0;
    double value       = 0;

    const RpUnits* fromObj = NULL;
    const RpUnits* toObj   = NULL;

    if (nargin == 3) {

        if (    args(0).is_real_scalar () &&
                args(1).is_real_scalar () &&
                args(2).is_real_scalar ()   ) {

            fromObjHandle = args(0).int_value ();
            toObjHandle   = args(1).int_value ();
            value         = args(2).double_value ();

            /* Call the C subroutine. */
            if (    (fromObjHandle != 0) &&
                    (toObjHandle != 0)    ) {

                fromObj = getObject_UnitsStr(fromObjHandle);
                toObj   = getObject_UnitsStr(toObjHandle);

                if ( fromObj && toObj) {
                    value = fromObj->convert(toObj,value,&err);
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

    retval(0) = value;
    retval(1) = err;
    return retval;
}
