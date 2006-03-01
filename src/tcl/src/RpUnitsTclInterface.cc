/*
 * ----------------------------------------------------------------------
 *  Rappture::units
 *
 *  This is an interface to the rappture units module.
 *  It allows you to convert between units and format values.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include "core/RpUnits.h"
#include "RpUnitsTclInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

// EXTERN int Rapptureunits_Init _ANSI_ARGS_((Tcl_Interp * interp));

static int RpTclUnitsConvert   _ANSI_ARGS_((    ClientData cdata,
                                                Tcl_Interp *interp,
                                                int argc, 
                                                const char *argv[]    ));

#ifdef __cplusplus
}
#endif

/*
 * ------------------------------------------------------------------------
 *  Rapptureunits_Init()
 *
 *  Called in RapptureGUI_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */

int
Rapptureunits_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::Units::convert",
        RpTclUnitsConvert, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

int
RpTclUnitsConvert   (   ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    std::string inValue       = ""; // value provided by the user
    std::string fromUnitsName = ""; // name of the units for user's value
    std::string toUnitsName   = ""; // name of the units to convert to
    std::string option        = ""; // tmp var for parsing command line options
    std::string val           = ""; // inValue + fromUnitsName as one string
    std::string convertedVal  = ""; // result of conversion
    int showUnits             = 0;  // flag if we should show units in result
    int result                = 0;  // flag if the conversion was successful

    int nextarg          = 1; // start parsing using the '2'th argument
    int argsLeft         = 0; // temp variable for calculation
    int retVal           = 0; // TCL_OK or TCL_ERROR depending on result val
    char *endptr         = NULL;

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc < 2) {
        Tcl_AppendResult(interp,
            "usage: ", argv[0], 
            " <value> ?-context units? ?-to units? ?-units on/off?",
            (char*)NULL);
        return TCL_ERROR;
    }

    inValue = std::string(argv[nextarg++]);

    argsLeft = (argc-nextarg);
    while (argsLeft > 0 ) {
        if (*argv[nextarg] == '-') {
            option = std::string(argv[nextarg]);

            if ( option == "-context" ) {
                nextarg++;
                fromUnitsName = std::string(argv[nextarg]);
                if (RpUnits::find(fromUnitsName) == NULL) {
                    Tcl_AppendResult(interp,
                        "-context value \"", fromUnitsName.c_str(), 
                        "\" is not a recognized unit for rappture",
                        (char*)NULL);
                    return TCL_ERROR;
                }

            }
            else if ( option == "-to" ) {
                nextarg++;
                toUnitsName = std::string(argv[nextarg]);
                if (RpUnits::find(toUnitsName) == NULL) {
                    Tcl_AppendResult(interp,
                        "-to value \"", toUnitsName.c_str(), 
                        "\" is not a recognized unit for rappture",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if ( option == "-units" ) {
                nextarg++;
                if (        (*(argv[nextarg]+1) == 'n') 
                        &&  (std::string(argv[nextarg]) == "on") ) {
                    showUnits = 1;
                }
                else if (   (*(argv[nextarg]+1) == 'f') 
                        &&  (std::string(argv[nextarg]) == "off") ) {
                    showUnits = 0;
                }
                else {
                    // unrecognized value for -units option
                    Tcl_AppendResult(interp,
                        "value for -units must be \"yes\" or \"no\"",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                // unrecognized option
                Tcl_AppendResult(interp, "invalid option: ", option.c_str(), 
                    ".\nusage: ", argv[0], 
                    " <value> ?-context units? ?-to units? ?-units on/off?",
                    (char*)NULL);
                return TCL_ERROR;
            }

            nextarg++;
        }
        else {
            // unrecognized input
            Tcl_AppendResult(interp, "invalid input: ", argv[nextarg],
                ".\nusage: ", argv[0], 
                " <value> ?-context units? ?-to units? ?-units on/off?",
                (char*)NULL);
            return TCL_ERROR;

        }

        argsLeft = (argc-nextarg);
    }

    // check the inValue to see if it has units
    // or if we should use those provided in -context option
    strtod(inValue.c_str(),&endptr);
    if ( ((unsigned)(endptr - inValue.c_str())) == inValue.length() ) {
        // there were no units at the end of the inValue string
        // rappture units convert expects the val variable to be 
        // the quantity and units in one string
        val = inValue + fromUnitsName;
    }
    else {
        // there seemed to be units at the end of the inValue string
        // we will ignore the -context flag and use the units in inValue
        val = inValue;
    }

    // call the rappture units convert function
    convertedVal = RpUnits::convert(val, toUnitsName, showUnits, &result);

    if ( (!convertedVal.empty()) && (result == 0) ) {
        // store the new result in the interpreter
        Tcl_AppendResult(interp, convertedVal.c_str(), (char*)NULL);
        retVal = TCL_OK;
    }
    else {
        // error while converting
        Tcl_AppendResult(interp, 
                "There was an error while converting\n"
                "The convert function returned the following string: \"",
                convertedVal.c_str(),"\"",
                (char*)NULL);
        retVal = TCL_ERROR;
    }

    return retVal;
}

