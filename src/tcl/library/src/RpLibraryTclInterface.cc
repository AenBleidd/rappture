/*
 * ----------------------------------------------------------------------
 *  Rappture::library
 *
 *  This is an interface to the system getrlimit() and setrlimit()
 *  routines.  It allows you to get/set resource limits for child
 *  executables.  We use this in Rappture::exec, for example, to make
 *  sure that jobs don't run forever or fill up the disk.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include "core/RpLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bltInt.h"

EXTERN int Rapptureext_Init _ANSI_ARGS_((Tcl_Interp * interp));

static int RpLibraryCmd   _ANSI_ARGS_((   ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));

static int RpLibCallCmd   _ANSI_ARGS_(( ClientData cData, Tcl_Interp *interp,
                                        int argc, const char* argv[]    ));

static int RpTclLibElem   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibParent _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibChild  _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibGet    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibPut    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibCopy   _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibRemove _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));
static int RpTclLibXml    _ANSI_ARGS_(( ClientData cdata, Tcl_Interp *interp,
                                        int argc, const char *argv[]    ));


static std::string rpLib2command _ANSI_ARGS_(( Tcl_Interp *interp,
                                               RpLibrary* newRpLibObj   ));
static int print_library_howto _ANSI_ARGS_((const char** argv));
static int print_libcmds_howto _ANSI_ARGS_((const char** argv));

// member function, function pointer mainly used in 'element' implementation
typedef std::string (RpLibrary::*rpMbrFxnPtr) _ANSI_ARGS_(());

static Blt_OpSpec rpLibOps[] = {
    {"children", 2, (Blt_Op)RpTclLibChild, 2, 7,
        "?-as <fval>? ?-type <name>? ?<path>?",},
    {"copy", 2, (Blt_Op)RpTclLibCopy, 5, 6,
        "<path> from ?<xmlobj>? <path>",},
    {"element", 1, (Blt_Op)RpTclLibElem, 2, 5, "?-as <fval>? ?<path>?",},
    {"get", 1, (Blt_Op)RpTclLibGet, 2, 3, "?<path>?",},
    {"parent", 2, (Blt_Op)RpTclLibParent, 2, 5, "?-as <fval>? ?<path>?",},
    {"put", 2, (Blt_Op)RpTclLibPut, 2, 8,
        "?-append yes? ?-id num? ?<path>? <string>",},
    {"remove", 1, (Blt_Op)RpTclLibRemove, 2, 3, "?<path>?",},
    {"xml", 1, (Blt_Op)RpTclLibXml, 2, 2, "",},
};

static int nRpLibOps = sizeof(rpLibOps) / sizeof(Blt_OpSpec);

#ifdef __cplusplus
}
#endif

/*
 * ------------------------------------------------------------------------
 *  RpLibraryTclInterface_Init()
 *
 *  Called in RapptureGUI_Init() to initialize the commands defined
 *  in this file.
 * ------------------------------------------------------------------------
 */

int
Rapptureext_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::library_test",
        RpLibraryCmd, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

int
RpLibraryCmd (  ClientData cData,
                Tcl_Interp *interp,
                int argc,
                const char* argv[]  )
{
    RpLibrary *rpptr = NULL;
    std::string libName = "";;

    if (argc != 2) {
        print_library_howto(argv);
        return TCL_ERROR;
    }

    rpptr = new RpLibrary(argv[1]);

    libName = rpLib2command(interp,rpptr);

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, libName.c_str(), (char*)NULL);

    return TCL_OK;
}

std::string
rpLib2command (Tcl_Interp *interp, RpLibrary* newRpLibObj) 
{
    static int libCount = 0;
    std::stringstream libName;

    libName << "library" << libCount++;

    Tcl_CreateCommand(interp, libName.str().c_str(),
        RpLibCallCmd, (ClientData)newRpLibObj, (Tcl_CmdDeleteProc*)NULL);

    return libName.str();
}

int
RpLibCallCmd (  ClientData cData,
                Tcl_Interp *interp,
                int argc,
                const char* argv[]  )
{
    Blt_Op proc;

    if (argc < 2) {
        print_libcmds_howto(argv);
        return TCL_ERROR;
    }

    proc = Blt_GetOp(interp, nRpLibOps, rpLibOps, BLT_OP_ARG1,
                    argc, (char**)argv, 0);

    if (proc == NULL) {
        return TCL_ERROR;
                                }
    return (*proc)(cData, interp, argc, argv);

}

int print_library_howto (const char **argv)
{
    std::cout << "usage: " << argv[0] << " <xmlfile>" << std::endl;
    return TCL_OK;
}

int print_libcmds_howto (const char **argv)
{
    std::cout << "usage: " << argv[0] << " <command> <arg-list>" << std::endl;

    return TCL_OK;
}

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
                    Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                        "\": should be -as <fval> where <fval> is ", 
                        "\'component\', \'id\', \'type\',",
                        "\'path\', \'object\'",
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
    if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp, "incorrect number of arguments \"", argv[nextarg],
            "\":", (char*)NULL);
        return TCL_ERROR;
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
                    Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                        "\": should be -as <fval> where <fval> is ", 
                        "\'component\', \'id\', \'type\',",
                        "\'path\', \'object\'",
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
    if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp, "incorrect number of arguments \"", argv[nextarg],
            "\":", (char*)NULL);
        return TCL_ERROR;
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
                    Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                        "\": should be -as <fval> where <fval> is ", 
                        "\'component\', \'id\', \'type\',",
                        "\'path\', \'object\'",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }
            else if (strncmp(argv[nextarg],"-type",5) == 0) {
                nextarg++;
                type = std::string(argv[nextarg]);
                nextarg++;
            }
            else {
                Tcl_AppendResult(interp, "bad option \"", argv[nextarg],
                    "\": should be -as", (char*)NULL);
                return TCL_ERROR;
            }
        }
    }

    argsLeft = (argc-nextarg);
    if (argsLeft == 1) {
        path = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp, "incorrect number of arguments \"", argv[nextarg],
            "\":", (char*)NULL);
        return TCL_ERROR;
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
RpTclLibGet     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string retStr = ""; // return value of rappture get fxn
    std::string path = "";

    if (argc > 2) {
        path = std::string(argv[2]);
    }

    // call the Rappture Library Get Function
    retStr = ((RpLibrary*) cdata)->getString(path);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

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
                if (strncmp(argv[++nextarg],"yes",3) == 0 ) {
                    append = 1;
                    nextarg++;
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
        Tcl_AppendResult(interp, "incorrect number of arguments \"", argv[nextarg],
            "\":", (char*)NULL);
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

    toPath = std::string(argv[nextarg++]);
    from = std::string(argv[nextarg++]);

    argsLeft = (argc-nextarg);
    if (argsLeft == 2) {
        fromObjStr = std::string(argv[nextarg++]);
        fromObj = NULL; // gotta figure out how to get the actual lib here
        fromPath = std::string(argv[nextarg++]);
    }
    else if (argsLeft == 1) {
        fromPath = std::string(argv[nextarg++]);
    }
    else {
        Tcl_AppendResult(interp, 
            "wrong # args: should be \"copy path from ?xmlobj? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    if (from != "from") {
        Tcl_AppendResult(interp,
            "bad syntax: should be \"copy path from ?xmlobj? path\"",
            (char*)NULL);
        return TCL_ERROR;
    }



    // call the rappture library put function
    ((RpLibrary*) cdata)->copy(toPath, fromPath,fromObj);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

int
RpTclLibRemove  (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string path = std::string("");    // path of where to copy data from
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

    // call the rappture library put function
    ((RpLibrary*) cdata)->remove(path);

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "", (char*)NULL);

    return TCL_OK;
}

int
RpTclLibXml     (   ClientData cdata,
                    Tcl_Interp *interp,
                    int argc,
                    const char *argv[]  )
{

    std::string retStr = ""; // return value of rappture get fxn

    // call the Rappture Library Get Function
    retStr = ((RpLibrary*) cdata)->xml();

    // clear any previous result in the interpreter
    // store the new result in the interpreter
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, retStr.c_str(), (char*)NULL);

    return TCL_OK;
}

