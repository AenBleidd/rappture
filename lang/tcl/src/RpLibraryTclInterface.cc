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
#include "RpLibrary.h"

#define RAPPTURE_OBJ_TYPE "::Rappture::LibraryObj"

extern "C" Tcl_AppInitProc RpLibrary_Init;

#include "RpOp.h"

static Tcl_ObjCmdProc RpLibraryCmd;
static Tcl_ObjCmdProc RpLibCallCmd;
static Tcl_ObjCmdProc RpTclLibChild;
static Tcl_ObjCmdProc RpTclLibCopy;
static Tcl_ObjCmdProc RpTclLibDiff;
static Tcl_ObjCmdProc RpTclLibElem;
static Tcl_ObjCmdProc RpTclLibGet;
static Tcl_ObjCmdProc RpTclLibInfo;
static Tcl_ObjCmdProc RpTclLibIsa;
static Tcl_ObjCmdProc RpTclLibParent;
static Tcl_ObjCmdProc RpTclLibPut;
static Tcl_ObjCmdProc RpTclLibRemove;
static Tcl_ObjCmdProc RpTclLibResult;
static Tcl_ObjCmdProc RpTclLibValue;
static Tcl_ObjCmdProc RpTclLibXml;
static Tcl_ObjCmdProc RpTclResult;

static std::string rpLib2command _ANSI_ARGS_((Tcl_Interp *interp,
    RpLibrary* newRpLibObj));
static int rpGetLibraryFromObj _ANSI_ARGS_((Tcl_Interp *interp,
    Tcl_Obj* obj, RpLibrary **rval));

// member function, function pointer mainly used in 'element' implementation
typedef std::string (RpLibrary::*rpMbrFxnPtr) _ANSI_ARGS_(());

static Rp_OpSpec rpLibOps[] = {
    {"children", 2, (void *)RpTclLibChild, 2, 7,
       	"?-as <fval>? ?-type <name>? ?<path>?",},
    {"copy",     2, (void *)RpTclLibCopy, 5, 6,
        "<path> from ?<xmlobj>? <path>",},
    {"diff",     1, (void *)RpTclLibDiff, 3, 3, "<xmlobj>",},
    {"element",  1, (void *)RpTclLibElem, 2, 5, "?-as <fval>? ?<path>?",},
    {"get",      1, (void *)RpTclLibGet, 2, 3, "?<path>?",},
    {"info",     1, (void *)RpTclLibInfo, 3, 3, "<objType>",},
    {"isa",      1, (void *)RpTclLibIsa, 3, 3, "<objType>",},
    {"parent",   2, (void *)RpTclLibParent, 2, 5, "?-as <fval>? ?<path>?",},
    {"put",      2, (void *)RpTclLibPut, 2, 8,
        "?-append yes? ?-id num? ?<path>? <string>",},
    {"remove",   3, (void *)RpTclLibRemove, 2, 3, "?<path>?",},
//    {"result", 3, (void *)RpTclLibResult, 2, 2, "",},
    {"xml",      1, (void *)RpTclLibXml, 2, 2, "",},
};

static int nRpLibOps = sizeof(rpLibOps) / sizeof(Rp_OpSpec);

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

    Tcl_CreateObjCommand(interp, libName.str().c_str(),
        RpLibCallCmd, (ClientData)newRpLibObj, (Tcl_CmdDeleteProc*)NULL);

    return libName.str();
}

/*
 * ------------------------------------------------------------------------
 *  rpGetLibraryFromObj()
 *
 *  Tries to decode the given argument as a Rappture Library object.
 *  Returns TCL_OK if successful, along with a pointer to the library
 *  in rval.  Otherwise, it returns TCL_ERROR, along with an error
 *  message in the interp.
 * ------------------------------------------------------------------------
 */
static int
rpGetLibraryFromObj(Tcl_Interp *interp, Tcl_Obj* obj, RpLibrary **rval)
{
    Tcl_CmdInfo info;
    char *cmdname = Tcl_GetString(obj);
    if (Tcl_GetCommandInfo(interp, cmdname, &info)) {
        if (info.objProc == RpLibCallCmd) {
            *rval = (RpLibrary*)(info.objClientData);
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "bad value \"", cmdname,
        "\": should be a Rappture library object",
        (char*)NULL);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  RpLibrary_Init()
 *
 *  Called in Rappture_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */
int
RpLibrary_Init(Tcl_Interp *interp)
{

    Tcl_CreateObjCommand(interp, "::Rappture::library",
        RpLibraryCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateObjCommand(interp, "::Rappture::result",
        RpTclResult, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    /*
    Tcl_CreateObjCommand(interp, "::Rappture::LibraryObj::value",
        RpTclLibValue, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    */

    return TCL_OK;
}

/*
 * USAGE: library <file>
 * USAGE: library standard
 * USAGE: library isvalid <object>
 */
static int
RpLibraryCmd (ClientData cData, Tcl_Interp *interp,
    int objc, Tcl_Obj* const *objv)
{
    const char *flag = Tcl_GetString(objv[1]);
    if (objc > 2 && strcmp(flag,"isvalid") == 0) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), "\": isvalid object", 
                (char*)NULL);
            return TCL_ERROR;
        }

        //
        // Check to see if the object is valid.  It should be a
        // recognized Tcl command with the appropriate handler
        // function.
        //
        int found = 0;
        RpLibrary *rlib;
        if (rpGetLibraryFromObj(interp, objv[2], &rlib) == TCL_OK && rlib) {
            found = 1;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(found));
        return TCL_OK;
    }

    if (objc != 2) {
        Tcl_AppendResult(interp, "usage: ", Tcl_GetString(objv[0]),
            " <xmlfile>",(char*)NULL);
        return TCL_ERROR;
    }

    // create a new command
    RpLibrary *rpptr = new RpLibrary( Tcl_GetString(objv[1]) );
    std::string libName = rpLib2command(interp,rpptr);

    Tcl_AppendResult(interp, libName.c_str(), (char*)NULL);
    return TCL_OK;
}


static int
RpLibCallCmd (ClientData cData, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = (Tcl_ObjCmdProc *)Rp_GetOpFromObj(interp, nRpLibOps, rpLibOps, 
	RP_OP_ARG1, objc, objv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(cData, interp, objc, objv);
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
static int
RpTclLibChild (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::string path   = "";    // path of where to place data inside xml tree
    std::string type   = "";    // type of nodes to be returned
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_objc       = 2;     // max number of optional parameters

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_objc--) {
        char *opt = Tcl_GetString(objv[nextarg]);
        if (nextarg < objc && *opt == '-') {
            if (strcmp(opt,"-as") == 0) {
                nextarg++;
                if (nextarg < objc) {
                    if (*opt == 'c' && strcmp(opt,"component") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if (*opt == 'i' && strcmp(opt,"id") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if (*opt == 't' && strcmp(opt,"type") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if (*opt == 'p' && strcmp(opt,"path") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if (*opt == 'o' && strcmp(opt,"object") == 0) {
                        asProc = (rpMbrFxnPtr)NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"", opt,
                            "\" for -as: should be component, id, type,",
                            " path, object", (char*)NULL);
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
            else if (strcmp(opt,"-type") == 0) {
                nextarg++;
                if (nextarg < objc) {
                    type = std::string(opt);
                    nextarg++;
                }
                else {
                    Tcl_AppendResult(interp, "bad flavor \"\" for -type ",
                        "option: should be a type of node within the xml",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", opt,
                    "\": should be -as, -type", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    int argsLeft = (objc-nextarg);
    if (argsLeft > 1) {
        Tcl_AppendResult(interp, 
                "wrong # args: should be ",
                "\"children ?-as <fval>? ?-type <name>? ?<path>?\"",
                (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string( Tcl_GetString(objv[nextarg++]) );
    }
    else {
        path = "";
    }

    // call the rappture library children function
    while ( (node = ((RpLibrary*) cdata)->children(path,node,type)) ) {
        if (node) {
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

static int
RpTclLibCopy (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::string fromPath = "";    // path of where to copy data from
    std::string toPath   = "";    // path of where to copy data to
    std::string from     = "";    // string that should == "from"
    RpLibrary* fromObj   = NULL;
    int nextarg          = 2;     // start parsing using the '2'th argument
    int argsLeft         = 0;     // temp variable for calculation

    if (nextarg+2 < objc) {
        toPath = std::string( Tcl_GetString(objv[nextarg++]) );
        from = std::string( Tcl_GetString(objv[nextarg++]) );
    }
    else {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " path from ?<xmlobj>? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    argsLeft = (objc-nextarg);
    if (argsLeft == 2) {
        if (rpGetLibraryFromObj(interp, objv[nextarg++], &fromObj) != TCL_OK) {
            return TCL_ERROR;
        }
        fromPath = std::string( Tcl_GetString(objv[nextarg++]) );
    }
    else if (argsLeft == 1) {
        fromPath = std::string( Tcl_GetString(objv[nextarg++]) );
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
    RpLibrary *libPtr	= (RpLibrary *)cdata;
    libPtr->copy(toPath, fromObj, fromPath);

    // clear any previous result in the interpreter
    Tcl_ResetResult(interp);
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

static int
RpTclLibDiff (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    RpLibrary* otherLib = NULL;

    std::list<std::string> diffList; // list to store the return value 
                                     // from diff command
    std::list<std::string>::iterator diffListIter;

    // parse input arguments
    if (rpGetLibraryFromObj(interp, objv[2], &otherLib) != TCL_OK) {
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
    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclLibElem()
/// element function in Tcl, used to retrieve a xml objects
/**
 * Returns a xml object.
 * Full function call:
 * element ?-as <fval>? ?<path>?
 */
static int
RpTclLibElem (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::string path   = "";    // path of where to place data inside xml tree
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_objc       = 1;     // max number of optional parameters

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_objc--) {
        char *opt = Tcl_GetString(objv[nextarg]);
        if (nextarg < objc && *opt == '-') {
            if (strcmp(opt,"-as") == 0) {
                nextarg++;
                if (nextarg < objc) {
                    if (*opt == 'c' && strcmp(opt,"component") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if (*opt == 'i' && strcmp(opt,"id") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if (*opt == 't' && strcmp(opt,"type") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if (*opt == 'p' && strcmp(opt,"path") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if (*opt == 'o' && strcmp(opt,"object") == 0) {
                        asProc = NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"",
                            Tcl_GetString(objv[nextarg]),
                            "\" for -as: should be component, id, type,"
                            " path, object", (char*)NULL);
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
                Tcl_AppendResult(interp, "bad option \"", opt,
                    "\": should be -as", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    int argsLeft = (objc-nextarg);
    if (argsLeft > 1) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"element ?-as <fval>? ?<path>?\"",
            (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string( Tcl_GetString(objv[nextarg++]) );
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
static int
RpTclLibGet (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{

    std::string retStr = ""; // return value of rappture get fxn
    std::string path = "";

    if (objc == 3) {
        path = std::string( Tcl_GetString(objv[2]) );
    }
    else if (objc != 2) {
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

static int
RpTclLibInfo (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{

    std::string infoType = ""; // string value of type of info being requested
    std::string retStr   = ""; // return value of rappture get fxn

    if (objc == 3) {
        infoType = std::string( Tcl_GetString(objv[2]) );
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

static int
RpTclLibIsa (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{

    std::string compVal = ""; // string value of object being compared to
    std::string retStr  = ""; // return value of rappture get fxn

    if (objc == 3) {
        compVal = std::string( Tcl_GetString(objv[2]) );
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

static int
RpTclLibParent (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{

    std::string path   = "";    // path of where to place data inside xml tree
    std::string retStr = "";    // path of where to place data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int opt_objc       = 1;     // max number of optional parameters

    RpLibrary* node    = NULL;
    rpMbrFxnPtr asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;

    // parse through -'d arguments
    while (opt_objc--) {
        char *opt = Tcl_GetString(objv[nextarg]);
        if (nextarg < objc && *opt == '-') {
            if (strcmp(opt,"-as") == 0) {
                nextarg++;
                if (nextarg < objc) {
                    if (*opt == 'c' && strcmp(opt,"component") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeComp;
                        nextarg++;
                    }
                    else if (*opt == 'i' && strcmp(opt,"id") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeId;
                        nextarg++;
                    }
                    else if (*opt == 't' && strcmp(opt,"type") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodeType;
                        nextarg++;
                    }
                    else if (*opt == 'p' && strcmp(opt,"path") == 0) {
                        asProc = (rpMbrFxnPtr)&RpLibrary::nodePath;
                        nextarg++;
                    }
                    else if (*opt == 'o' && strcmp(opt,"object") == 0) {
                        asProc = NULL;
                        nextarg++;
                    }
                    else {
                        Tcl_AppendResult(interp, "bad flavor \"",
                            Tcl_GetString(objv[nextarg]),
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
                Tcl_AppendResult(interp, "bad option \"", opt,
                    "\": should be -as", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    int argsLeft = (objc-nextarg);
    if ( argsLeft > 1) {
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"parent ?-as <fval>? ?<path>?\"",
            (char*)NULL);
        return TCL_ERROR;
    }
    else if (argsLeft == 1) {
        path = std::string( Tcl_GetString(objv[nextarg++]) );
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
 * put <path>
 *
 * Return Value:
 * On success, None. 
 * On failure, an error message is returned
 */
static int
RpTclLibPut (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::string id     = "";    // id tag for the given path
    std::string path   = "";    // path of where to place data inside xml tree
    std::string addStr = "";    // string to be added to data inside xml tree
    int nextarg        = 2;     // start parsing using the '2'th argument
    int append         = 0;     // append flag - 0 means no, 1 means yes
    int opt_objc       = 2;     // max number of optional parameters

    while (opt_objc--) {
        char *opt = Tcl_GetString(objv[nextarg]);
        if (nextarg < objc && *opt == '-') {
            if (strcmp(opt,"-append") == 0) {
                ++nextarg;
                if (nextarg < objc) {
                    if (Tcl_GetBooleanFromObj(interp, objv[nextarg],
                          &append) != TCL_OK) {
                        return TCL_ERROR;
                    }
                }
                else {
                    Tcl_AppendResult(interp, "missing value for -append",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strcmp(opt,"-id") == 0) {
                id = std::string( Tcl_GetString(objv[++nextarg]) );
                nextarg++;
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", opt,
                    "\": should be -append or -id", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    int argsLeft = (objc-nextarg);
    if (argsLeft == 2) {
        path = std::string( Tcl_GetString(objv[nextarg++]) );
        addStr = std::string( Tcl_GetString(objv[nextarg++]) );
    }
    else if (argsLeft == 1) {
        path = "";
        addStr = std::string( Tcl_GetString(objv[nextarg++]) );
    }
    else {
        Tcl_AppendResult(interp, "wrong # args: should be ",
            "\"put ?-append yes? ?-id num? ?<path>? <string>\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    // call the rappture library put function
    ((RpLibrary*) cdata)->put(path, addStr, id, append);

    // return nothing for this operation
    Tcl_ResetResult(interp);
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
static int
RpTclLibRemove (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::string path = std::string("");  // path of where to remove data from

    if (objc == 3) {
        path = std::string( Tcl_GetString(objv[2]) );
    }

    // call the rappture library remove function
    ((RpLibrary*) cdata)->remove(path);

    // clear any previous result in the interpreter
    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int
RpTclLibResult (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    // call the rappture library result function
    ((RpLibrary*) cdata)->result();

    // clear any previous result in the interpreter
    Tcl_ResetResult(interp);
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
static int
RpTclLibValue (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    std::list<std::string> valList;  // list to store the return value 
                                     // from diff command
    std::list<std::string>::iterator valListIter;

    if (objc != 3) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"", Tcl_GetString(objv[0]),
            " <xmlobj> <path>\"", (char*)NULL);
        return TCL_ERROR;
    }

    // parse input arguments
    RpLibrary* lib;
    if (rpGetLibraryFromObj(interp, objv[1], &lib) != TCL_OK) {
        return TCL_ERROR;
    }
    std::string path = std::string(Tcl_GetString(objv[2]));

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

static int
RpTclLibXml (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{

    std::string retStr = ""; // return value of rappture get fxn

    if (objc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
            Tcl_GetString(objv[0]), " xml\"", (char*)NULL);
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
static int
RpTclResult (ClientData cdata, Tcl_Interp *interp,
    int objc, Tcl_Obj *const *objv)
{
    // parse through command line options
    if (objc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
            Tcl_GetString(objv[0]), " <xmlobj>\"", (char*)NULL);
        return TCL_ERROR;
    }

    RpLibrary* lib;
    if (rpGetLibraryFromObj(interp, objv[1], &lib) != TCL_OK) {
        return TCL_ERROR;
    }
    lib->result();

    // returns nothing
    Tcl_ResetResult(interp);
    return TCL_OK;
}
