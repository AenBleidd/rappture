/*
 * ----------------------------------------------------------------------
 *  Rappture::library
 *
 *  This is an interface to the rappture library module.
 *  It allows you to create rappture library objects.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include <sstream>
#include <string>
#include "core/RpLibrary.h"
#include "RpLibraryTclInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bltInt.h"


static int RpLibraryCmd   _ANSI_ARGS_((   ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpLibCallCmd   _ANSI_ARGS_(( ClientData cData, Tcl_Interp *interp,
                                        int argc, const char* argv[]    ));

static int RpTclLibChild  _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibCopy   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibDiff   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibElem   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibGet    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibInfo   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibIsa    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibParent _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibPut    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibRemove _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibResult _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibValue  _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibXml    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));

static int RpTclResult   _ANSI_ARGS_((  ClientData cdata,
                                        Tcl_Interp *interp,
                                        int argc,
                                        const char *argv[]    ));


static std::string rpLib2command _ANSI_ARGS_(( Tcl_Interp *interp,
                                               RpLibrary* newRpLibObj   ));
void appendExpectErr    _ANSI_ARGS_((   Tcl_Interp *interp,
                                        const char* errTxt,
                                        const char* receivedArg));

// member function, function pointer mainly used in 'element' implementation
typedef std::string (RpLibrary::*rpMbrFxnPtr) _ANSI_ARGS_(());

static Blt_OpSpec rpLibOps[] = {
    {"children", 2, (Blt_Op)RpTclLibChild, 2, 7,
        "?-as <fval>? ?-type <name>? ?<path>?",},
    {"copy", 2, (Blt_Op)RpTclLibCopy, 5, 6,
        "<path> from ?<xmlobj>? <path>",},
    {"diff", 1, (Blt_Op)RpTclLibDiff, 3, 3, "<xmlobj>",},
    {"element", 1, (Blt_Op)RpTclLibElem, 2, 5, "?-as <fval>? ?<path>?",},
    {"get", 1, (Blt_Op)RpTclLibGet, 2, 3, "?<path>?",},
    {"info", 1, (Blt_Op)RpTclLibInfo, 3, 3, "<objType>",},
    {"isa", 1, (Blt_Op)RpTclLibIsa, 3, 3, "<objType>",},
    {"parent", 2, (Blt_Op)RpTclLibParent, 2, 5, "?-as <fval>? ?<path>?",},
    {"put", 2, (Blt_Op)RpTclLibPut, 2, 8,
        "?-append yes? ?-id num? ?<path>? <string>",},
    {"remove", 3, (Blt_Op)RpTclLibRemove, 2, 3, "?<path>?",},
//    {"result", 3, (Blt_Op)RpTclLibResult, 2, 2, "",},
    {"xml", 1, (Blt_Op)RpTclLibXml, 2, 2, "",},
};

static int nRpLibOps = sizeof(rpLibOps) / sizeof(Blt_OpSpec);

#ifdef __cplusplus
}
#endif

/*
 * ------------------------------------------------------------------------
 *  rpLib2command()
 *
 *  dsk defined helper function for creating a new command out of a 
 *  rappture library
 *
 * ------------------------------------------------------------------------
 */

std::string
rpLib2command (Tcl_Interp *interp, RpLibrary* newRpLibObj) 
{
    static int libCount = 0;
    std::stringstream libName;

    libName << "::libraryObj" << libCount++;

    Tcl_CreateCommand(interp, libName.str().c_str(),
        RpLibCallCmd, (ClientData)newRpLibObj, (Tcl_CmdDeleteProc*)NULL);

    return libName.str();
}

/*
 * ------------------------------------------------------------------------
 *  appendExpectErr()
 *
 *  dsk defined helper function for appending error messages to the
 *  tcl interpreter without worrying if the receivedArg is empty
 *
 * ------------------------------------------------------------------------
 */

void
appendExpectErr   (Tcl_Interp *interp, const char* errTxt, const char* receivedArg)
{
    if (receivedArg) {
        Tcl_AppendResult(interp, errTxt, "\"",
            receivedArg, "\"", (char*)NULL);
    }
    else {
        Tcl_AppendResult(interp, errTxt, "\"\"", (char*)NULL);
    }
}

/*
 * ------------------------------------------------------------------------
 *  Rappturelibrary_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */

int
Rappturelibrary_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::library",
        RpLibraryCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "::Rappture::result",
        RpTclResult, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    /*
    Tcl_CreateCommand(interp, "::Rappture::LibraryObj::value",
        RpTclLibValue, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    */

    return TCL_OK;
}

/*
 * 
 * USAGE: library <file>
 * USAGE: library standard
 * USAGE: library isvalid <object>
 *
 */

int
RpLibraryCmd (  ClientData cData,
                Tcl_Interp *interp,
                int argc,
                const char* argv[]  )
{
    RpLibrary *rpptr = NULL;
    std::string libName = "";
    int noerr = 0;
    std::stringstream result;
    std::string path = "";
    Tcl_CmdInfo info;  // pointer to the command info
    const char * var = NULL; // the path of Rappture installation


    if ( (argc > 2) && (*argv[1] == 'i') && (strncmp(argv[1],"isvalid",7) == 0) ) {
        result.str("0");
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                "\": isvalid <object>", 
                (char*)NULL);
            return TCL_ERROR;
        }

        noerr = Tcl_GetCommandInfo(interp, argv[2], &info);
        if (noerr == 1) {
            if (info.proc == &RpLibCallCmd) {
                result.clear();
                result.str("1");
            }
        }

        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, result.str().c_str(), (char*)NULL);
        return TCL_OK;
    }

    if (argc != 2) {
        Tcl_AppendResult(interp,
                "wrong # args: should be \"", argv[0],
                "\" [ <file> | standard | isvalid <object> ]",
                (char*)NULL);
        return TCL_ERROR;
    }

    if ( (*argv[1] == 's') && (strncmp(argv[1],"standard",8) == 0) ) {
        var = Tcl_GetVar(interp,"Rappture::installdir",0);
        if (var) {
            path = std::string(var) + "/lib/library.xml";
            rpptr = new RpLibrary(path);
            libName = rpLib2command(interp,rpptr);
            Tcl_AppendResult(interp, libName.c_str(), (char*)NULL);
        }
        else {
            Tcl_AppendResult(interp,
                    "Rappture::installdir is not a valid variable",
                    (char*)NULL);
            return TCL_ERROR;
        }
    }
    else {
        // create a new command
        rpptr = new RpLibrary(argv[1]);
        libName = rpLib2command(interp,rpptr);
        Tcl_AppendResult(interp, libName.c_str(), (char*)NULL);
    }

    return TCL_OK;
}


int
RpLibCallCmd (  ClientData cData,
                Tcl_Interp *interp,
                int argc,
                const char* argv[]  )
{
    Blt_Op proc;

    if (argc < 2) {
        Tcl_AppendResult(interp,
                "wrong # args: should be one of...\n",
                "  ", argv[0], " cget -option\n",
                "  ", argv[0], " children ?arg arg ...?\n",
                "  ", argv[0], " configure ?-option? ?value -option value...?\n",
                "  ", argv[0], " copy path from ?arg arg ...?\n",
                "  ", argv[0], " diff libobj\n",
                "  ", argv[0], " element ?arg arg ...?\n",
                "  ", argv[0], " get ?path?\n",
                "  ", argv[0], " info infoType\n",
                "  ", argv[0], " isa className\n",
                "  ", argv[0], " parent ?arg arg ...?\n",
                "  ", argv[0], " put ?arg arg ...?\n",
                "  ", argv[0], " remove ?path?\n",
                "  ", argv[0], " xml\n",
                (char*)NULL);
        return TCL_ERROR;
    }

    proc = Blt_GetOp(interp, nRpLibOps, rpLibOps, BLT_OP_ARG1,
                    argc, (char**)argv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
                                }
    return (*proc)(cData, interp, argc, argv);

}

int checkAsArgs(Tcl_Interp *interp, const char* testStr, rpMbrFxnPtr asProc)
{
    if (    (*testStr == 'c') &&
            (strncmp(testStr,"component",9) == 0) ) {
        asProc = &RpLibrary::nodeComp;
    }
    else if ((*testStr == 'i') &&
             (strncmp(testStr,"id",2) == 0 ) ) {
        asProc = &RpLibrary::nodeId;
    }
    else if ((*testStr == 't') &&
             (strncmp(testStr,"type",4) == 0 ) ) {
        asProc = &RpLibrary::nodeType;
    }
    else if ((*testStr == 'p') &&
             (strncmp(testStr,"path",4) == 0 ) ) {
        asProc = &RpLibrary::nodePath;
    }
    else if ((*testStr == 'o') &&
             (strncmp(testStr,"object",6) == 0 ) ) {
        asProc = NULL;
    }
    else {
        return TCL_ERROR;
    }

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibChild()
/// children function in Tcl, retrieves a list of children for a given path
/**
 * Returns a list of children of the node located at <path>
 * Full function call:
 *
 * children ?-as <fval>? ?-type <name>? ?<path>?
 * 
 * -as option specifies how to return the result
 * <fval> is one of the following:
 *     component - return the component name of child node, ex: type(id)
 *     id        - return the id attribute of child node
 *     type      - return the type attribute of child node 
 *     path      - return the full path of child node
 *     object    - return a Rappture Library Object
 *
 * -type option tells function to only look for nodes of type <name>
 *
 * <path> is the path of the parent node you are requesting the children of
 *        if <path> is left blank, the current node will be considered the
 *        parent node and all its children will be returned.
 */

int
RpTclLibChild   (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{
    std::string path   = "";    // path of where to place data inside xml tree
    std::string type   = "";    // type of nodes to be returned
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_argc       = 2;     // max number of optional parameters
    int argsLeft       = 0;     // temp variable for calculation

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = &RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_argc--) {
        if (nextarg < argc && *argv[nextarg] == '-') {
            if (strncmp(argv[nextarg],"-as",3) == 0) {
                nextarg++;
                if (nextarg < argc) {
                    if (    (*argv[nextarg] == 'c') &&
                            (strncmp(argv[nextarg],"component",9) == 0) ) {
                        asProc = &RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'i') &&
                             (strncmp(argv[nextarg],"id",2) == 0 ) ) {
                        asProc = &RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 't') &&
                             (strncmp(argv[nextarg],"type",4) == 0 ) ) {
                        asProc = &RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'p') &&
                             (strncmp(argv[nextarg],"path",4) == 0 ) ) {
                        asProc = &RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'o') &&
                             (strncmp(argv[nextarg],"object",6) == 0 ) ) {
                        asProc = NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"", argv[nextarg],
                            "\" for -as: should be component, id, type, path, object",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    Tcl_AppendResult(interp, "bad flavor \"\" for -as",
                            ": should be component, id, type, path, object",
                            (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strncmp(argv[nextarg],"-type",5) == 0) {
                nextarg++;
                if (nextarg < argc) {
                    type = std::string(argv[nextarg]);
                    nextarg++;
                }
                else {
                    Tcl_AppendResult(interp, "bad flavor \"\" for -type option",
                        ": should be a type of node within the xml",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                    "\": should be -as, -type", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    argsLeft = (argc-nextarg);
    if (argsLeft > 1) {
        Tcl_AppendResult(interp, 
                "wrong # args: should be ",
                "\"children ?-as <fval>? ?-type <name>? ?<path>?\"",
                (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        path = "";
    }

    // call the rappture library children function
    while ( (node = ((RpLibrary*) cdata)->children(path,node,type)) ) {
        if (node) {

            // clear any previous result in the interpreter
            // Tcl_ResetResult(interp);

            if (asProc) {
                // evaluate the "-as" flag on the returned node
                retStr = (node->*asProc)();
            }
            else {
                // create a new command for the new rappture object
                retStr = rpLib2command(interp, node);
            }

            // store the new result string in the interpreter
            Tcl_AppendElement(interp,retStr.c_str());
        }
    }

    return TCL_OK;
}

int
RpTclLibCopy    (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{
    std::string fromPath = "";    // path of where to copy data from
    std::string toPath   = "";    // path of where to copy data to
    std::string from     = "";    // string that should == "from"
    std::string fromObjStr = "";  // string that represents the 
                                  // string name of the object
    RpLibrary* fromObj   = NULL;
    int nextarg          = 2;     // start parsing using the '2'th argument
    int argsLeft         = 0;     // temp variable for calculation
    int noerr              = 0;     // err flag for Tcl_GetCommandInfo
    Tcl_CmdInfo info;  // pointer to the command info

    if (nextarg+2 < argc) {
        toPath = std::string(argv[nextarg++]);
        from = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            argv[0], " path from ?<xmlobj>? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    argsLeft = (argc-nextarg);
    if (argsLeft == 2) {
        fromObjStr = std::string(argv[nextarg++]);
        noerr = Tcl_GetCommandInfo(interp, fromObjStr.c_str(), &info);
        if (noerr == 1) {
            if (info.proc == RpLibCallCmd) {
                fromObj = (RpLibrary*) (info.clientData);
            }
            else {
                Tcl_AppendResult(interp,
                    "wrong arg type: xmlobj should be a Rappture Library\"",
                    (char*)NULL);
                return TCL_ERROR;
            }
        }
        else {
            // Tcl_GetCommandInfo failed, not able to retrieve xmlobj
            Tcl_AppendResult(interp,
                "wrong arg type: xmlobj should be a valid Rappture Library\"",
                (char*)NULL);
            return TCL_ERROR;
        }
        fromPath = std::string(argv[nextarg++]);
    }
    else if (argsLeft == 1) {
        fromPath = std::string(argv[nextarg++]);
    }
    else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"copy path from ?xmlobj? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    if (from != "from") {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp,
            "bad syntax: should be \"copy path from ?xmlobj? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // call the rappture library copy function
    ((RpLibrary*) cdata)->copy(toPath, fromPath, fromObj);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibDiff()
/// diff function in Tcl, used to find the difference between two xml objects
/**
 * Returns a list of differences between two xml objects.
 * Full function call:
 * diff <xmlobj>
 */

int
RpTclLibDiff     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    Tcl_CmdInfo info;                // pointer to the command info
    std::string otherLibStr = "";
    RpLibrary* otherLib     = NULL;
    int nextarg             = 2;     // start parsing using the '2'th argument
    int argsLeft            = 0;     // temp variable for calculation
    int noerr               = 0;     // err flag for Tcl_GetCommandInfo

    std::list<std::string> diffList; // list to store the return value 
                                     // from diff command
    std::list<std::string>::iterator diffListIter;

    // parse input arguments
    argsLeft = (argc-nextarg);
    if (argsLeft == 1) {
        otherLibStr = std::string(argv[nextarg++]);
        noerr = Tcl_GetCommandInfo(interp, otherLibStr.c_str(), &info);
        if (noerr == 1) {
            if (info.proc == RpLibCallCmd) {
                otherLib = (RpLibrary*) (info.clientData);
            }
            else {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp,
                    "wrong arg type: xmlobj should be a Rappture Library",
                    (char*)NULL);
                return TCL_ERROR;
            }
        }
        else {
            // there was an error getting the command info
            Tcl_AppendResult(interp, 
                "There was an error getting the command info for "
                "the provided library \"", otherLibStr.c_str(), "\'",
                "\nAre you sure its a Rappture Library Object?",
                (char*)NULL);
            return TCL_ERROR;
        }
    }
    else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"diff <xmlobj>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // perform the diff command
    diffList = ((RpLibrary*) (cdata))->diff(otherLib,"input");
    diffListIter = diffList.begin();

    Tcl_ResetResult(interp);

    // parse through the output of the diff command
    // put it into the return result
    while (diffListIter != diffList.end()) {
        Tcl_AppendElement(interp,(*diffListIter).c_str());

        // increment the iterator
        diffListIter++;
    }


    // Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);
    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibElem()
/// element function in Tcl, retrieves the xml object for a given path
/**
 * Returns a xml object.
 * Returns a list of children of the node located at <path>
 * Full function call:
 *
 * element ?-as <fval>? ?<path>?
 *
 * -as option specifies how to return the result
 * <fval> is one of the following:
 *     component - return the component name of xml node, ex: type(id)
 *     id        - return the id attribute of xml node
 *     type      - return the type attribute of xml node 
 *     path      - return the full path of xml node
 *     object    - return a Rappture Library Object
 *
 * <path> is the path of the xml node you are requesting.
 *        if <path> is left blank, the current node will
 *        be considered the xml node and it will be returned.
 */

int
RpTclLibElem    (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{
    std::string path   = "";    // path of where to place data inside xml tree
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_argc       = 1;     // max number of optional parameters
    int argsLeft       = 0;     // temp variable for calculation

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = &RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_argc--) {
        if (nextarg < argc && *argv[nextarg] == '-') {
            if (strncmp(argv[nextarg],"-as",3) == 0) {
                nextarg++;
                if (nextarg < argc) {
                    if (    (*argv[nextarg] == 'c') &&
                            (strncmp(argv[nextarg],"component",9) == 0) ) {
                        asProc = &RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'i') &&
                             (strncmp(argv[nextarg],"id",2) == 0 ) ) {
                        asProc = &RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 't') &&
                             (strncmp(argv[nextarg],"type",4) == 0 ) ) {
                        asProc = &RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'p') &&
                             (strncmp(argv[nextarg],"path",4) == 0 ) ) {
                        asProc = &RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'o') &&
                             (strncmp(argv[nextarg],"object",6) == 0 ) ) {
                        asProc = NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"", argv[nextarg],
                            "\" for -as: should be component, id, type, path, object",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    Tcl_AppendResult(interp, "bad flavor \"\" for -as:", 
                           " should be component, id, type, path, object",
                            (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                    "\": should be -as", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    argsLeft = (argc-nextarg);
    if (argsLeft > 1) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"element ?-as <fval>? ?<path>?\"",
            (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        path = "";
    }

    // call the rappture library element function
    node = ((RpLibrary*) cdata)->element(path);
    if (node) {

        // clear any previous result in the interpreter
        Tcl_ResetResult(interp);

        if (asProc) {
            // evaluate the "-as" flag on the returned node
            retStr = (node->*asProc)();
        }
        else {
            // create a new command for the new rappture object
            retStr = rpLib2command(interp, node);
        }
        // store the new result string in the interpreter
        Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);
    }


    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibGet()
/// get function in Tcl, used to retrieve the value of a xml object
/**
 * Returns the value of a xml object.
 * Full function call:
 * get ?<path>?
 */

int
RpTclLibGet     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string retStr = ""; // return value of rappture get fxn
    std::string path = "";

    if (argc == 3) {
        path = std::string(argv[2]);
    }
    else if (argc != 2) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"get ?path?\"",
            "\":", (char*)NULL);
        return TCL_ERROR;
    }

    // call the Rappture Library Get Function
    retStr = ((RpLibrary*) cdata)->getString(path);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibInfo()
/// info function in Tcl, return specific information about this object's type
/**
 * Query the object about its data type.
 * This function is available for compatibility with old itcl 
 * Rappture::LibraryObj's
 *
 * Full function call:
 * info <infoType>
 *
 * <infoType> must be one of the following:
 *     class
 *
 * Return Values:
 * class -> ::Rappture::LibraryObj
 */

int
RpTclLibInfo    (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string infoType = ""; // string value of type of info being requested
    std::string retStr   = ""; // return value of rappture get fxn

    if (argc == 3) {
        infoType = std::string(argv[2]);
    }
    else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"info <infoType>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    if ("class" == infoType) {
        retStr = RAPPTURE_OBJ_TYPE;
    }
    else {
        retStr = "";
        Tcl_AppendResult(interp, 
            "bad flavor: \"", infoType.c_str(), "\" should be \"class\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // store the new result in the interpreter
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibIsa()
/// isa function in Tcl, check this objects type against a provided <objType>
/**
 * Query the object about its data type.
 * This function is available for compatibility with old itcl 
 * Rappture::LibraryObj's
 *
 * Full function call:
 * isa <objType>
 *
 * <infoType> must be one of the following:
 *     class
 *
 * Return Value:
 * if "::Rappture::LibraryObj" == <objType> returns "1"
 * else returns "0"
 */

int
RpTclLibIsa     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string compVal = ""; // string value of object being compared to
    std::string retStr  = ""; // return value of rappture get fxn

    if (argc == 3) {
        compVal = std::string(argv[2]);
    }
    else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"isa <objType>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    if (compVal == RAPPTURE_OBJ_TYPE) {
        retStr = "1";
    }
    else {
        retStr = "0";
    }

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibParent()
/// parent function in Tcl, retrieves the parent xml object for a given path
/**
 * Returns the xml object repersenting the parent of the given path.
 * Full function call:
 *
 * parent ?-as <fval>? ?<path>?
 *
 * -as option specifies how to return the result
 * <fval> is one of the following:
 *     component - return the component name of the parent node, ex: type(id)
 *     id        - return the id attribute of the parent node
 *     type      - return the type attribute of the parent node 
 *     path      - return the full path of the parent node
 *     object    - return a Rappture Library Object
 *
 * <path> is the path of the xml node you are requesting the parent of.
 *        if <path> is left blank, the current node will
 *        be considered the child xml node and it's parent will be returned.
 */

int
RpTclLibParent  (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string path   = "";    // path of where to place data inside xml tree
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_argc       = 1;     // max number of optional parameters
    int argsLeft       = 0;     // temp variable for calculation

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = &RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_argc--) {
        if (nextarg < argc && *argv[nextarg] == '-') {
            if (strncmp(argv[nextarg],"-as",3) == 0) {
                nextarg++;
                if (nextarg < argc) {
                    if (    (*argv[nextarg] == 'c') &&
                            (strncmp(argv[nextarg],"component",9) == 0) ) {
                        asProc = &RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'i') &&
                             (strncmp(argv[nextarg],"id",2) == 0 ) ) {
                        asProc = &RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 't') &&
                             (strncmp(argv[nextarg],"type",4) == 0 ) ) {
                        asProc = &RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'p') &&
                             (strncmp(argv[nextarg],"path",4) == 0 ) ) {
                        asProc = &RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if ((*argv[nextarg] == 'o') &&
                             (strncmp(argv[nextarg],"object",6) == 0 ) ) {
                        asProc = NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"", argv[nextarg],
                            "\": should be component, id, type, path, object",
                            (char*)NULL);
                        return TCL_ERROR;
                    }
                }
                else {
                    Tcl_AppendResult(interp, "bad flavor \"\" for -as: ",
                            "should be component, id, type, path, object",
                            (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                    "\": should be -as", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    argsLeft = (argc-nextarg);
    if ( argsLeft > 1) {
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"parent ?-as <fval>? ?<path>?\"",
            (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        path = "";
    }

    // call the rappture library parent function
    node = ((RpLibrary*) cdata)->parent(path);
    if (node) {

        // clear any previous result in the interpreter
        Tcl_ResetResult(interp);

        if (asProc) {
            // evaluate the "-as" flag on the returned node
            retStr = (node->*asProc)();
        }
        else {
            // create a new command for the new rappture object
            retStr = rpLib2command(interp, node);
        }
        // store the new result string in the interpreter
        Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    }


    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibPut()
/// put function in Tcl, put a value into a xml node at location <path>
/**
 * Put a value into a xml node at location <path>
 *
 * Full function call:
 * put ?-append yes? ?-id num? ?<path>? <string>
 *
 * Return Value:
 * On success, None. 
 * On failure, an error message is returned
 */

int
RpTclLibPut     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string id     = "";    // id tag for the given path
    std::string path   = "";    // path of where to place data inside xml tree
    std::string addStr = "";    // string to be added to data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int append         = 0;     // append flag - 0 means no, 1 means yes
    int opt_argc       = 2;     // max number of optional parameters
    int argsLeft       = 0;     // temp variable for calculation

    while (opt_argc--) {
        if (nextarg < argc && *argv[nextarg] == '-') {
            if (strncmp(argv[nextarg],"-append",7) == 0) {
                ++nextarg;
                if (nextarg < argc) {
                    if (strncmp(argv[nextarg],"yes",3) == 0 ) {
                        append = 1;
                        nextarg++;
                    }
                    else if (strncmp(argv[nextarg],"no",2) == 0 ) {
                        append = 0;
                        nextarg++;
                    }
                    else {
                        appendExpectErr(interp,
                                "expected boolean value but got ", 
                                argv[nextarg]);
                        return TCL_ERROR;
                    }
                }
                else {
                    appendExpectErr(interp,
                            "expected boolean value but got ", 
                            argv[nextarg]);
                    return TCL_ERROR;
                }
            }
            else if (strncmp(argv[nextarg],"-id",3) == 0) {
                id = std::string(argv[++nextarg]);
                nextarg++;
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                    "\": should be -append or -id", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    argsLeft = (argc-nextarg);
    if (argsLeft == 2) {
        path = std::string(argv[nextarg++]);
        addStr = std::string(argv[nextarg++]);
    }
    else if (argsLeft == 1) {
        path = "";
        addStr = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp, "wrong # args: should be ",
            "\"put ?-append yes? ?-id num? ?<path>? <string>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // call the rappture library put function
    ((RpLibrary*) cdata)->put(path, addStr, id, append);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibRemove()
/// remove function in Tcl, used to remove a xml node at location <path>
/**
 * Removes the xml node at location <path>, if it exists. Does nothing
 * if the node does not exist.
 *
 * Full function call:
 * remove ?<path>?
 *
 * Return Value:
 * On success, None. 
 * On failure, an error message is returned
 */

int
RpTclLibRemove  (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string path = std::string("");    // path of where to remove data from
    int nextarg = 2;


    if (argc == 2) {
        path = std::string("");
    }
    else if (argc == 3) {
        path = std::string(argv[nextarg]);
    }
    else {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"remove ?path?\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // call the rappture library remove function
    ((RpLibrary*) cdata)->remove(path);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

int
RpTclLibResult  (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{
    if (argc == 2) {
        // call the rappture library result function
        ((RpLibrary*) cdata)->result();
    }
    else {
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"result \"",
            (char*)NULL);
        return TCL_ERROR;
    }


    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibValue()
/// Rappture::LibraryObj::value function in Tcl, used to normalize a number.
/**
 * Normalizes values located at <path> in <xmlobj>.
 *
 * Full function call:
 * Rappture::LibraryObj::value <xmlobj> <path>
 *
 * Return Value:
 * 2 element list
 *   - first element is the original value at location <path>
 *   - second element is the normalization of the value at location <path>
 */

int
RpTclLibValue   (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    Tcl_CmdInfo info;                // pointer to the command info
    std::string path        = "";
    std::string libStr      = "";
    RpLibrary* lib          = NULL;
    int nextarg             = 1;     // start parsing using the '1'th argument
    int argsLeft            = 0;     // temp variable for calculation
    int noerr               = 0;     // err flag for Tcl_GetCommandInfo

    std::list<std::string> valList; // list to store the return value 
                                     // from diff command
    std::list<std::string>::iterator valListIter;

    if ( argc != 3 ) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"", argv[0], " <xmlobj> <path>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // parse input arguments
    argsLeft = (argc-nextarg);
    if (argsLeft == 2) {
        libStr = std::string(argv[nextarg++]);
        noerr = Tcl_GetCommandInfo(interp, libStr.c_str(), &info);
        if (noerr == 1) {
            if (info.proc == RpLibCallCmd) {
                lib = (RpLibrary*) (info.clientData);
            }
            else {
                Tcl_AppendResult(interp,
                    "wrong arg type: xmlobj should be a Rappture Library",
                    (char*)NULL);
                return TCL_ERROR;
            }
        }
        else {
            // there was an error getting the command info
            Tcl_AppendResult(interp, 
                "There was an error getting the command info for "
                "the provided library \"", libStr.c_str(), "\'",
                "\nAre you sure its a Rappture Library Object?",
                (char*)NULL);
            return TCL_ERROR;
        }
        path = std::string(argv[nextarg++]);
    }
    else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"", argv[0], " <xmlobj> <path>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // perform the value command
    valList = lib->value(path);
    valListIter = valList.begin();

    // parse through the output of the diff command
    // put it into the return result
    while (valListIter != valList.end()) {
        Tcl_AppendElement(interp,(*valListIter).c_str());

        // increment the iterator
        valListIter++;
    }

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibXml()
/// xml function in Tcl, returns the xml data that this object represents
/**
 * Prints the xml text for this object.
 *
 * Full function call:
 * xml
 *
 * Return Value:
 * xml text as a string
 */

int
RpTclLibXml     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string retStr = ""; // return value of rappture get fxn

    if ( argc != 2 ) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                argv[0], " xml\"", (char*)NULL);
        return TCL_ERROR;
    }

    // call the Rappture Library xml Function
    retStr = ((RpLibrary*) cdata)->xml();

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclResult()
/// Rappture::result function in Tcl, prints xml to file and signals gui.
/**
 * Prints xml text representing provided object to a runXXXX.xml file
 * and sends the '=RAPTURE=RUN=>runXXXX.xml' signal to the graphical
 * user interface so the data can be visualized.
 *
 * Full function call:
 * Rappture::result <rpObj>
 *
 * <rpObj> is a valid rappture object
 *
 * Return Value:
 * None
 */

int
RpTclResult   (   ClientData cdata,
                   Tcl_Interp *interp,
                   int argc,
                   const char *argv[]  )
{
    Tcl_CmdInfo info;            // pointer to the command info
    int noerr            = 0;    // err flag for Tcl_GetCommandInfo
    std::string libName  = "";
    RpLibrary* lib       = NULL;

    // parse through command line options
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                argv[0], " <xmlobj>\"", (char*)NULL);
        return TCL_ERROR;
    }

    libName = std::string(argv[1]);

    noerr = Tcl_GetCommandInfo(interp, libName.c_str(), &info);
    if (noerr == 1) {
        if (info.proc == RpLibCallCmd) {
            lib = (RpLibrary*) (info.clientData);
        }
        else {
            Tcl_AppendResult(interp,
                "wrong arg type: xmlobj should be a Rappture Library\"",
                (char*)NULL);
            return TCL_ERROR;
        }
    }
    else {
        // there was an error getting the command info
        Tcl_AppendResult(interp,
            "wrong arg type: \"", libName.c_str(),
            "\" should be a valid Rappture Library Object",
            (char*)NULL);
        return TCL_ERROR;
    }

    lib->result();
    Tcl_AppendResult(interp,"",(char*)NULL);
    return TCL_OK;
}
