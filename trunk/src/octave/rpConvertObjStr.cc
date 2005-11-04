/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Matlab Rappture Library Source
 *
 *    [retStr,err] = rpConvertObjStr(fromObjHandle,toObjHandle,value,showUnits)
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpOctaveInterface.h"

/**********************************************************************/
// METHOD: [retStr,err] = rpConvertObjStr(fromObjHandle,toObjHandle,value,showUnits)
/// Convert between RpUnits return a string value with or without units
/**
 * Convert @var{value} from the units represented by the RpUnits object
 * @var{fromObjHandle} to the units represented by the RpUnits object
 * @var{toObjHandle}. If @var{showUnits} is set to 0, no units will be
 * displayed in @var{retStr}, else units are displayed. 
 * On success, the converted value is returned through
 * @var{retStr}. The second return value, @var{err}, specifies whether 
 * there was an error during conversion.
 * Error code, err=0 on success, anything else is failure.
 */

DEFUN_DLD (rpConvertObjStr, args, ,
"-*- texinfo -*-\n\
[retStr,err] = rpConvertObjStr(@var{fromObjHandle},@var{toObjHandle},@var{value},@var{showUnits})\n\
\n\
Convert @var{value} from the units represented by the RpUnits object\n\
@var{fromObjHandle} to the units represented by the RpUnits object\n\
@var{toObjHandle}. If @var{showUnits} is set to 0, no units will be\n\
displayed in @var{retStr}, else units are displayed.\n\
On success, the converted value is returned through\n\
@var{retStr}. The second return value, @var{err}, specifies whether \n\
there was an error during conversion.\n\
Error code, err=0 on success, anything else is failure.")
{
    static std::string who = "rpConvertObjStr";

    // The list of values to return.
    octave_value_list retval;

    int err            = 1;
    int nargin         = args.length ();
    int fromObjHandle  = 0;
    int toObjHandle    = 0;
    int showUnits      = 0;
    double inVal       = 0;
    std::string outVal = "";

    const RpUnits* fromObj = NULL;
    const RpUnits* toObj   = NULL;

    if (nargin == 4) {

        if (    args(0).is_real_scalar () &&
                args(1).is_real_scalar () &&
                args(2).is_real_scalar () &&
                args(3).is_real_scalar ()   ) {

            fromObjHandle = args(0).int_value ();
            toObjHandle   = args(1).int_value ();
            inVal         = args(2).double_value ();
            showUnits     = args(3).int_value ();

            /* Call the C subroutine. */
            if (    (fromObjHandle != 0) &&
                    (toObjHandle != 0)    ) {

                fromObj = getObject_UnitsStr(fromObjHandle);
                toObj   = getObject_UnitsStr(toObjHandle);

                if ( fromObj && toObj) {
                    outVal = fromObj->convert(toObj,inVal,showUnits,&err);
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

    retval(0) = outVal;
    retval(1) = err;
    return retval;
}

