/*
 * ----------------------------------------------------------------------
 *  Rappture::units
 *
 *  This is an interface to the rappture units module.
 *  It allows you to convert between units and format values.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include "RpUnits.h"

extern "C" Tcl_AppInitProc RpUnits_Init;

static Tcl_CmdProc RpTclUnitsConvert;
static Tcl_CmdProc RpTclUnitsDesc;
static Tcl_CmdProc RpTclUnitsSysFor;
static Tcl_CmdProc RpTclUnitsSysAll;
static Tcl_CmdProc RpTclUnitsSearchFor;


/**********************************************************************/
// FUNCTION: RpUnits_Init()
/// Initializes the Rappture Units module and commands defined below
/**
 * Called in Rappture_Init() to initialize the Rappture Units module.
 * Initialized commands include:
 * ::Rappture::Units::convert
 * ::Rappture::Units::description
 * ::Rappture::Units::System::for
 * ::Rappture::Units::System::all
 * ::Rappture::Units::Search::for
 */

#include <algorithm> 
#include <functional>
#include <cctype>

// Trim from start
static inline std::string &ltrim(std::string &s) 
{
    s.erase(s.begin(), 
	std::find_if(s.begin(), s.end(), 
		     std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// Trim from end
static inline std::string &rtrim(std::string &s) 
{
    s.erase(std::find_if(s.rbegin(), s.rend(), 
	std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// Trim from both ends
static inline std::string &trim(std::string &s) 
{
    return ltrim(rtrim(s));
}

extern "C" int
RpUnits_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::Units::convert",
        RpTclUnitsConvert, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::description",
        RpTclUnitsDesc, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::System::for",
        RpTclUnitsSysFor, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::System::all",
        RpTclUnitsSysAll, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::Units::Search::for",
        RpTclUnitsSearchFor, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

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
    std::string origUnitsName = "";
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
                    origUnitsName = fromUnitsName;
                    err = RpUnits::validate(fromUnitsName,type,&compatList);
                    if ( err != 0) {
                        Tcl_AppendResult(interp,
                            "bad value \"", origUnitsName.c_str(), 
                            "\": should be a recognized unit for Rappture",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                } else {
                    // if user does not specify wishes for this option,
                    // set fromUnitsName to an empty string.
                    fromUnitsName = "";
                }
            } else if ( option == "-to" ) {
                nextarg++;
                if (argv[nextarg] != NULL) {
                    toUnitsName = std::string(argv[nextarg]);
                    err = 0;
                    origUnitsName = toUnitsName;
                    err = RpUnits::validate(toUnitsName,type);
                    if (err != 0) {
                        Tcl_AppendResult(interp,
                            "bad value \"", origUnitsName.c_str(), 
                            "\": should be a recognized unit for Rappture",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                } else {
                    // if user does not specify wishes for this option,
                    // set toUnitsName to an empty string.
                    toUnitsName = "";
                }
            } else if ( option == "-units" ) {
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
                } else {
                    // if user does not specify wishes for this option,
                    // return error.
                    // unrecognized value for -units option
                    Tcl_AppendResult(interp,
                        "expected boolean value but got \"\"", (char*)NULL);
                    return TCL_ERROR;
                }
            } else {
                // unrecognized option
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg], 
                        "\": should be -context, -to, -units",
                        (char*)NULL);
                return TCL_ERROR;
            }

            nextarg++;
        } else {
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


    // Trim away white space from the value.  
    trim(inValue);

    strtod(inValue.c_str(), &endptr);
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
    } else if ( ((unsigned)(endptr - inValue.c_str())) == inValue.length() ) {
        // add 1 because we are subtracting indicies
        // there were no units at the end of the inValue string
        // rappture units convert expects the val variable to be 
        // the quantity and units in one string

        if (!fromUnitsName.empty()) {
            val = inValue + fromUnitsName;
        } else {
            Tcl_AppendResult(interp, "value: \"", inValue.c_str(),
                    "\" has unrecognized units", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
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
    } else {
        // error while converting
        Tcl_AppendResult(interp, 
                convertedVal.c_str(),
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
// FUNCTION: RpTclUnitsSearchfor()
/// Rappture::Units::Search::for fxn in Tcl, returns string of found units
/**
 * Returns a list of all units from the given units string that
 * were found within the units dictionary. This function takes in a
 * string with or without a value. The string at the very least should
 * contain the units you are searching for in the dictionary. If the
 * string contains a value as well, the value will be ignored. A value
 * is considered any numeric sequence as defined by the function
 * strtod().
 *
 * Full function call:
 * ::Rappture::Units::Search::for <units>
 */

int
RpTclUnitsSearchFor (  ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    std::string unitsName     = ""; // name of the units provided by user
    std::string origUnitsName = ""; // name of the units provided by user
    std::string type          = ""; // junk variable that validate() needs
    int nextarg               = 1; // start parsing using the '2'th argument
    int err                   = 0; // err code for validate
    double val                = 0;

    Tcl_ResetResult(interp);

    // parse through command line options
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                argv[0], " units\"", (char*)NULL);
        return TCL_ERROR;
    }

    // find where the unitsName begins
    unitSlice(std::string(argv[nextarg]),unitsName,val);

    err = RpUnits::validate(unitsName,type);
    if (err) {
        /*
         * according to tcl version, in this case we
         * should return an empty string. i happen to disagree.
         * the next few lines is what i think the user should see.
        Tcl_AppendResult(interp,
            "Unrecognized units: \"", origUnitsName.c_str(), "\"", (char*)NULL);
        return TCL_ERROR;
        */
        return TCL_OK;
    }

    Tcl_AppendResult(interp, unitsName.c_str(), (char*)NULL);

    return TCL_OK;
}
