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
#include "RpUnits.h"
#include "RpUnitsTclInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

// EXTERN int Rapptureunits_Init _ANSI_ARGS_((Tcl_Interp * interp));

static int RpTclUnitsConvert   _ANSI_ARGS_((    ClientData cdata,
                                                Tcl_Interp *interp,
                                                int argc, 
                                                const char *argv[]    ));

static int RpTclUnitsDesc      _ANSI_ARGS_((    ClientData cdata,
                                                Tcl_Interp *interp,
                                                int argc, 
                                                const char *argv[]    ));

static int RpTclUnitsSysFor   _ANSI_ARGS_((    ClientData cdata,
                                                Tcl_Interp *interp,
                                                int argc, 
                                                const char *argv[]    ));

static int RpTclUnitsSysAll   _ANSI_ARGS_((    ClientData cdata,
                                                Tcl_Interp *interp,
                                                int argc, 
                                                const char *argv[]    ));

#ifdef __cplusplus
}
#endif


static int list2str (std::list<std::string>& inList, std::string& outString);





/**********************************************************************/
// FUNCTION: Rapptureunits_Init()
/// Initializes the Rappture Units module and commands defined below
/**
 * Called in RapptureGUI_Init() to initialize the Rappture Units module.
 * Initialized commands include:
 * ::Rappture::Units::convert
 * ::Rappture::Units::description
 * ::Rappture::Units::System::for
 * ::Rappture::Units::System::all
 */

int
Rapptureunits_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::Units::convert",
        RpTclUnitsConvert, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::description",
        RpTclUnitsDesc, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::System::for",
        RpTclUnitsSysFor, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::System::all",
        RpTclUnitsSysAll, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclUnitsConvert()
/// Rappture::Units::convert function in Tcl, used to convert unit.
/**
 * Converts values between recognized units in Rappture.
 * Full function call:
 * ::Rappture::Units::convert <value> ?-context units? ?-to units? ?-units on/off?
 *
 * units attached to <value> take precedence over units 
 * provided in -context option.
 */

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
    int showUnits             = 1;  // flag if we should show units in result
    int result                = 0;  // flag if the conversion was successful

    int nextarg          = 1; // start parsing using the '2'th argument
    int argsLeft         = 0; // temp variable for calculation
    int retVal           = 0; // TCL_OK or TCL_ERROR depending on result val
    char *endptr         = NULL;

    int err                   = 0;  // err code for validate()
    std::string type          = ""; // junk variable that validate() needs
    std::list<std::string> compatList;
    std::list<std::string>::iterator compatListIter;
    std::string listStr       = ""; // string version of compatList
    std::string mesg          = ""; // error mesg text

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc < 2) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            // argv[0]," value args\"",
            argv[0], 
            " <value> ?-context units? ?-to units? ?-units on/off?\"",
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
                if (argv[nextarg] != NULL) {
                    fromUnitsName = std::string(argv[nextarg]);
                    err = 0;
                    err = RpUnits::validate(fromUnitsName,type,&compatList);
                    if ( err != 0) {
                        Tcl_AppendResult(interp,
                            "bad value \"", fromUnitsName.c_str(), 
                            "\": should be a recognized unit for Rappture",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    // if user does not specify wishes for this option,
                    // set fromUnitsName to an empty string.
                    fromUnitsName = "";
                }
            }
            else if ( option == "-to" ) {
                nextarg++;
                if (argv[nextarg] != NULL) {
                    toUnitsName = std::string(argv[nextarg]);
                    err = 0;
                    err = RpUnits::validate(toUnitsName,type);
                    if (err != 0) {
                        Tcl_AppendResult(interp,
                            "bad value \"", toUnitsName.c_str(), 
                            "\": should be a recognized unit for Rappture",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    // if user does not specify wishes for this option,
                    // set toUnitsName to an empty string.
                    toUnitsName = "";
                }
            }
            else if ( option == "-units" ) {
                nextarg++;
                if (argv[nextarg] != NULL) {
                    if (Tcl_GetBoolean(interp, argv[nextarg], &showUnits)) {
                        // unrecognized value for -units option
                        // Tcl_GetBoolean fills in error message
                        // Tcl_AppendResult(interp,
                        //     "expected boolean value but got \"", 
                        //     argv[nextarg], "\"", (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    // if user does not specify wishes for this option,
                    // return error.
                    // unrecognized value for -units option
                    Tcl_AppendResult(interp,
                        "expected boolean value but got \"\"", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                // unrecognized option
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg], 
                        "\": should be -context, -to, -units",
                        (char*)NULL);
                return TCL_ERROR;
            }

            nextarg++;
        }
        else {
            // unrecognized input
            Tcl_AppendResult(interp, "bad option \"", argv[nextarg], "\": ",
                "should be -context, -to, -units",
                (char*)NULL);
            return TCL_ERROR;

        }

        argsLeft = (argc-nextarg);
    }

    // check the inValue to see if it has units
    // or if we should use those provided in -context option
    strtod(inValue.c_str(),&endptr);
    if (endptr == inValue.c_str()) {
        // there was no numeric value that could be pulled from inValue
        // return error

        mesg =  "\": should be a real number with units";

        if (!fromUnitsName.empty()) {
            list2str(compatList,listStr);
            mesg = mesg + " of (" + listStr + ")";
        }
 
        Tcl_AppendResult(interp, "bad value \"", 
                inValue.c_str(), mesg.c_str(), (char*)NULL);
        return TCL_ERROR;
    }
    else if ( ((unsigned)(endptr - inValue.c_str())) == inValue.length() ) {
        // add 1 because we are subtracting indicies
        // there were no units at the end of the inValue string
        // rappture units convert expects the val variable to be 
        // the quantity and units in one string

        if (!fromUnitsName.empty()) {
            val = inValue + fromUnitsName;
        }
        else {
            Tcl_AppendResult(interp, "value: \"", inValue.c_str(),
                    "\" has unrecognized units", (char*)NULL);
            return TCL_ERROR;
        }
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
                "error while converting, returned string: \"",
                convertedVal.c_str(),"\"",
                (char*)NULL);
        retVal = TCL_ERROR;
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: RpTclUnitsDesc()
/// Rappture::Units::description function in Tcl, returns description of units
/**
 * Returns a description for the specified system of units.
 * The description includes the abstract type (length, temperature, etc.)
 * along with a list of all compatible systems.
 *
 * Full function call:
 * ::Rappture::Units::description <units>
 */

int
RpTclUnitsDesc      (   ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    std::string unitsName     = ""; // name of the units provided by user
    std::string type          = ""; // name of the units provided by user
    std::string listStr       = ""; // name of the units provided by user
    // const RpUnits* unitsObj   = NULL;
    std::list<std::string> compatList;
    std::list<std::string>::iterator compatListIter;

    int nextarg               = 1; // start parsing using the '2'th argument
    int err                   = 0; // err code for validate()

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc != 2) {
        Tcl_AppendResult(interp, 
                "wrong # args: should be \"", argv[0], 
                " units\"", (char*)NULL);
        return TCL_ERROR;
    }

    unitsName = std::string(argv[nextarg]);

    err = RpUnits::validate(unitsName,type,&compatList);
    if (err) {
        /*
         * according to tcl version, in this case we 
         * should return an empty string. i happen to disagree.
         * the next few lines is what i think the user should see.
        Tcl_AppendResult(interp,
            "bad value \"", unitsName.c_str(), 
            "\": should be a recognized unit for Rappture",
            (char*)NULL);
        return TCL_ERROR;
        */
        return TCL_OK;
    }

    Tcl_AppendResult(interp, type.c_str(), (char*)NULL);

    list2str(compatList,listStr);

    Tcl_AppendResult(interp, " (", listStr.c_str() ,")", (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclUnitsSysFor()
/// Rappture::Units::System::for fxn in Tcl, returns system for given units
/**
 * Returns the system, as a string, for the given system of units, or ""
 * if there is no system that matches the units string.
 *
 * Full function call:
 * ::Rappture::Units::System::for <units>
 */

int
RpTclUnitsSysFor    (   ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    std::string unitsName     = ""; // name of the units provided by user
    std::string type          = ""; // type/system of units to be returned to user
    int nextarg               = 1; // start parsing using the '2'th argument
    int err                   = 0;

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                argv[0], " units\"", (char*)NULL);
        return TCL_ERROR;
    }

    unitsName = std::string(argv[nextarg]);

    // look in our dictionary of units to see if 'unitsName' is a valid unit
    // if so, return its type (or system) in the variable 'type'.
    err = RpUnits::validate(unitsName,type);
    if (err) {
        /*
         * according to tcl version, in this case we
         * should return an empty string. i happen to disagree.
         * the next few lines is what i think the user should see.
        Tcl_AppendResult(interp,
            "The units named: \"", unitsName.c_str(),
            "\" is not a recognized unit for rappture",
            (char*)NULL);
        return TCL_ERROR;
        */
        return TCL_OK;
    }

    Tcl_AppendResult(interp, type.c_str(), (char*)NULL);
    return TCL_OK;

}

/**********************************************************************/
// FUNCTION: RpTclUnitsSysAll()
/// Rappture::Units::System::all fxn in Tcl, returns list of compatible units
/**
 * Returns a list of all units compatible with the given units string.
 * Compatible units are determined by following all conversion
 * relationships that lead to the same base system.
 *
 * Full function call:
 * ::Rappture::Units::System::all <units>
 */

int
RpTclUnitsSysAll    (   ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    std::string unitsName     = ""; // name of the units provided by user
    std::string type          = ""; // junk variable that validate() needs
    // const RpUnits* unitsObj   = NULL;
    std::list<std::string> compatList;
    std::list<std::string>::iterator compatListIter;
    int nextarg               = 1; // start parsing using the '2'th argument
    int err                   = 0; // err code for validate

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                argv[0], " units\"", (char*)NULL);
        return TCL_ERROR;
    }

    unitsName = std::string(argv[nextarg]);

    err = RpUnits::validate(unitsName,type,&compatList);
    if (err) {
        /*
         * according to tcl version, in this case we 
         * should return an empty string. i happen to disagree.
         * the next few lines is what i think the user should see.
        Tcl_AppendResult(interp,
            "The units named: \"", unitsName.c_str(), 
            "\" is not a recognized unit for rappture",
            (char*)NULL);
        return TCL_ERROR;
        */
        return TCL_OK;
    }

    compatListIter = compatList.begin();

    while (compatListIter != compatList.end()) {
        Tcl_AppendElement(interp,(*compatListIter).c_str());
        // increment the iterator
        compatListIter++;
    }

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: list2str()
/// Convert a std::list<std::string> into a comma delimited std::string
/**
 * Iterates through a std::list<std::string> and returns a comma 
 * delimited std::string containing the elements of the inputted std::list.
 *
 * Returns 0 on success, anything else is error
 */

int
list2str (std::list<std::string>& inList, std::string& outString)
{
    int retVal = 1;  // return Value 0 is success, everything else is failure
    unsigned int counter = 0; // check if we hit all elements of inList
    std::list<std::string>::iterator inListIter; // list interator

    inListIter = inList.begin();

    while (inListIter != inList.end()) {
        if ( outString.empty() ) {
            outString = *inListIter;
        }
        else {
            outString =  outString + "," + *inListIter;
        }

        // increment the iterator and loop counter
        inListIter++;
        counter++;
    }

    if (counter == inList.size()) {
        retVal = 0;
    }

    return retVal;
}
