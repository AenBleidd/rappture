/*
 * ----------------------------------------------------------------------
 *  rp_optimizer_tcl
 *
 *  This is the Tcl API for the functions in rp_optimizer.  This code
 *  allows you to call all of the core optimization functions from Tcl.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "rp_optimizer.h"
#include "rp_optimizer_plugin.h"

/*
 * ----------------------------------------------------------------------
 * KNOWN OPTIMIZATION PACKAGES
 * Add an entry below for each new optimization package that is
 * plugged in and available via the -using option.  End with all
 * NULL values.
 * ----------------------------------------------------------------------
 */
RpOptimInit PgapackInit;
RpOptimCleanup PgapackCleanup;
extern RpTclOption PgapackOptions;

static RpOptimPlugin rpOptimPlugins[] = {
    {"pgapack", PgapackInit, PgapackCleanup, &PgapackOptions},
    {NULL, NULL, NULL},
};

typedef struct RpOptimPluginData {
    RpOptimPlugin *pluginDefn;      /* points back to plugin definition */
    ClientData clientData;          /* data needed for particular plugin */
} RpOptimPluginData;

/*
 * ----------------------------------------------------------------------
 *  Options for the various parameter types
 * ----------------------------------------------------------------------
 */
RpTclOption rpOptimNumberOpts[] = {
  {"-min", RP_OPTION_DOUBLE, NULL, Rp_Offset(RpOptimParamNumber,min)},
  {"-max", RP_OPTION_DOUBLE, NULL, Rp_Offset(RpOptimParamNumber,max)},
  {NULL, NULL, NULL, 0}
};

RpTclOption rpOptimStringOpts[] = {
  {"-values", RP_OPTION_LIST, NULL, Rp_Offset(RpOptimParamString,values)},
  {NULL, NULL, NULL, 0}
};

static int RpOptimizerCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static void RpOptimCmdDelete _ANSI_ARGS_((ClientData cdata));
static int RpOptimInstanceCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static void RpOptimInstanceCleanup _ANSI_ARGS_((ClientData cdata));

#ifdef BUILD_Rappture
__declspec( dllexport )
#endif

int
Rapptureoptimizer_Init(Tcl_Interp *interp)   /* interpreter being initialized */
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "RapptureOptimizer", PACKAGE_VERSION)
          != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "::Rappture::optimizer", RpOptimizerCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpOptimizerCmd()
 *
 *  Invoked whenever someone uses the "optimizer" command to create a
 *  new optimizer context.  Handles the following syntax:
 *
 *      optimizer ?<name>? ?-using <pluginName>?
 *
 *  Creates a command called <name> that can be used to manipulate
 *  the optimizer context.  Returns TCL_OK on success, and TCL_ERROR
 *  (along with an error message in the interpreter) if anything goes
 *  wrong.
 * ------------------------------------------------------------------------
 */
static int
RpOptimizerCmd(cdata, interp, objc, objv)
    ClientData cdata;         /* not used */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *CONST objv[];    /* command line args */
{
    /* use this for auto-generated names */
    static int autocounter = 0;

    /* use this plugin by default for -using */
    RpOptimPlugin *usingPluginPtr = &rpOptimPlugins[0];

    char *name = NULL;

    RpOptimEnv* envPtr;
    RpOptimPlugin* pluginPtr;
    RpOptimPluginData* pluginDataPtr;
    int n;
    char *option, autoname[32], *sep;
    Tcl_CmdInfo cmdInfo;

    /*
     * Make sure that a command with this name doesn't already exist.
     */
    n = 1;
    if (objc >= 2) {
        name = Tcl_GetStringFromObj(objv[1], (int*)NULL);
        if (*name != '-') {
            if (Tcl_GetCommandInfo(interp, name, &cmdInfo)) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "command \"", name, "\" already exists",
                    (char*)NULL);
                return TCL_ERROR;
            }
            n++;
        }
    }

    /*
     * Parse the rest of the arguments.
     */
    while (n < objc) {
        option = Tcl_GetStringFromObj(objv[n], (int*)NULL);
        if (strcmp(option,"-using") == 0) {
            if (n+1 >= objc) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "missing value for option \"", option, "\"",
                    (char*)NULL);
                return TCL_ERROR;
            }

            /* search for a plugin with the given name */
            option = Tcl_GetStringFromObj(objv[n+1], (int*)NULL);
            for (pluginPtr=rpOptimPlugins; pluginPtr->name; pluginPtr++) {
                if (strcmp(pluginPtr->name,option) == 0) {
                    break;
                }
            }
            if (pluginPtr->name == NULL) {
                /* oops! name not recognized */
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "bad plugin name \"", option, "\": should be ",
                    (char*)NULL);

                sep = "";
                for (pluginPtr=rpOptimPlugins; pluginPtr->name; pluginPtr++) {
                    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                        sep, pluginPtr->name, (char*)NULL);
                    sep = ", ";
                }
                return TCL_ERROR;
            }
            usingPluginPtr = pluginPtr;
            n += 2;
        }
        else {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option \"", option, "\": should be ",
                "-using", (char*)NULL);
            return TCL_ERROR;
        }
    }

    /*
     * If a name wasn't specified, then auto-generate one.
     */
    while (name == NULL) {
        sprintf(autoname, "optimizer%d", autocounter++);
        if (!Tcl_GetCommandInfo(interp, autoname, &cmdInfo)) {
            name = autoname;
        }
    }

    /*
     * Create an optimizer and install a Tcl command to access it.
     */
    pluginDataPtr = (RpOptimPluginData*)malloc(sizeof(RpOptimPluginData));
    pluginDataPtr->pluginDefn = usingPluginPtr;
    pluginDataPtr->clientData = NULL;
    if (usingPluginPtr->initPtr) {
        pluginDataPtr->clientData = (*usingPluginPtr->initPtr)();
    }
    envPtr = RpOptimCreate((ClientData)pluginDataPtr, RpOptimInstanceCleanup);

    Tcl_CreateObjCommand(interp, name, RpOptimInstanceCmd,
        (ClientData)envPtr, (Tcl_CmdDeleteProc*)RpOptimCmdDelete);

    Tcl_SetResult(interp, name, TCL_VOLATILE);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimDelete()
 *
 * Called whenever a optimizer object is deleted to clean up after
 * the command.  If the optimizer is running, it is aborted, and
 * the optimizer is deleted.
 * ----------------------------------------------------------------------
 */
static void
RpOptimCmdDelete(cdata)
    ClientData cdata;   /* optimizer being deleted */
{
    RpOptimEnv *envPtr = (RpOptimEnv*)cdata;
    int n;
    ClientData paramdata;

    for (n=0; n < envPtr->numParams; n++) {
        paramdata = (ClientData)envPtr->paramList[n];
        switch (envPtr->paramList[n]->type) {
        case RP_OPTIMPARAM_NUMBER:
            RpTclOptionsCleanup(rpOptimNumberOpts, paramdata);
            break;
        case RP_OPTIMPARAM_STRING:
            RpTclOptionsCleanup(rpOptimStringOpts, paramdata);
            break;
        }
    }
    RpOptimDelete(envPtr);
}

/*
 * ------------------------------------------------------------------------
 *  RpOptimInstanceCmd()
 *
 *  Invoked to handle the actions of an optimizer object.  Handles the
 *  following syntax:
 *
 *      <name> add number <path> ?-min <number>? ?-max <number>?
 *      <name> add string <path> ?-values <valueList>?
 *      <name> get ?<glob>? ?-option?
 *      <name> configure ?-option? ?value -option value ...?
 *      <name> perform ?-maxruns <num>? ?-abortvar <varName>?
 *
 *  The "add" command is used to add various parameter types to the
 *  optimizer context.  The "perform" command kicks off an optimization
 *  run.
 * ------------------------------------------------------------------------
 */
static int
RpOptimInstanceCmd(cdata, interp, objc, objv)
    ClientData cdata;         /* optimizer context */
    Tcl_Interp *interp;       /* interpreter handling this request */
    int objc;                 /* number of command line args */
    Tcl_Obj *CONST objv[];    /* command line args */
{
    RpOptimEnv* envPtr = (RpOptimEnv*)cdata;
    RpOptimPluginData* pluginDataPtr = (RpOptimPluginData*)envPtr->pluginData;

    int n, j, nmatches;
    char *option, *type, *path;
    RpOptimParam *paramPtr;
    RpTclOption *optSpecPtr;
    Tcl_Obj *rval, *rrval;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args...?");
        return TCL_ERROR;
    }
    option = Tcl_GetStringFromObj(objv[1], (int*)NULL);

    /*
     * OPTION:  add type ?args...?
     */
    if (*option == 'a' && strcmp(option,"add") == 0) {
        if (objc < 4) {
            Tcl_WrongNumArgs(interp, 1, objv, "add type path ?args...?");
            return TCL_ERROR;
        }
        type = Tcl_GetStringFromObj(objv[2], (int*)NULL);
        path = Tcl_GetStringFromObj(objv[3], (int*)NULL);

        /*
         * OPTION:  add number name ?-min num? ?-max num?
         */
        if (*type == 'n' && strcmp(type,"number") == 0) {
            paramPtr = RpOptimAddParamNumber(envPtr, path);
            if (RpTclOptionsProcess(interp, objc-4, objv+4,
                  rpOptimNumberOpts, (ClientData)paramPtr) != TCL_OK) {
                RpOptimDeleteParam(envPtr, path);
                return TCL_ERROR;
            }
        }

        /*
         * OPTION:  add string name ?-values list?
         */
        else if (*type == 's' && strcmp(type,"string") == 0) {
            paramPtr = RpOptimAddParamString(envPtr, path);
            if (RpTclOptionsProcess(interp, objc-4, objv+4,
                  rpOptimStringOpts, (ClientData)paramPtr) != TCL_OK) {
                RpOptimDeleteParam(envPtr, path);
                return TCL_ERROR;
            }
        }
        else {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad parameter type \"", type, "\": should be number, string",
                (char*)NULL);
            return TCL_ERROR;
        }
    }

    /*
     * OPTION:  get ?globPattern? ?-option?
     */
    else if (*option == 'g' && strcmp(option,"get") == 0) {
        if (objc > 2) {
            path = Tcl_GetStringFromObj(objv[2], (int*)NULL);
        } else {
            path = NULL;
        }
        if (objc > 3) {
            option = Tcl_GetStringFromObj(objv[3], (int*)NULL);
        } else {
            option = NULL;
        }
        if (objc > 4) {
            Tcl_WrongNumArgs(interp, 1, objv, "get ?pattern? ?-option?");
            return TCL_ERROR;
        }

        /* count the number of matches */
        nmatches = 0;
        for (n=0; n < envPtr->numParams; n++) {
            if (path == NULL
                  || Tcl_StringMatch(envPtr->paramList[n]->name,path)) {
                nmatches++;
            }
        }

        rval = Tcl_NewListObj(0,NULL);
        Tcl_IncrRefCount(rval);
        for (n=0; n < envPtr->numParams; n++) {
            if (path == NULL
                  || Tcl_StringMatch(envPtr->paramList[n]->name,path)) {

                rrval = Tcl_NewListObj(0,NULL);
                Tcl_IncrRefCount(rrval);

                /* add the parameter name as the first element */
                if (nmatches > 1 || path == NULL) {
                    if (Tcl_ListObjAppendElement(interp, rrval,
                          Tcl_NewStringObj(envPtr->paramList[n]->name,-1))
                          != TCL_OK) {
                        Tcl_DecrRefCount(rrval);
                        Tcl_DecrRefCount(rval);
                        return TCL_ERROR;
                    }
                }

                /* get the option specifications for this parameter */
                switch (envPtr->paramList[n]->type) {
                case RP_OPTIMPARAM_NUMBER:
                    optSpecPtr = rpOptimNumberOpts;
                    break;
                case RP_OPTIMPARAM_STRING:
                    optSpecPtr = rpOptimStringOpts;
                    break;
                default:
                    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                        "internal error: unrecognized parameter type",
                        " for \"", envPtr->paramList[n]->name,"\"",
                        (char*)NULL);
                    Tcl_DecrRefCount(rrval);
                    Tcl_DecrRefCount(rval);
                    return TCL_ERROR;
                }

                if (option == NULL) {
                    /* no particular option value */
                    for (j=0; optSpecPtr[j].optname; j++) {
                        char *curOpt = optSpecPtr[j].optname;
                        /* append -option name */
                        if (Tcl_ListObjAppendElement(interp, rrval,
                              Tcl_NewStringObj(curOpt,-1)) != TCL_OK) {
                            Tcl_DecrRefCount(rrval);
                            Tcl_DecrRefCount(rval);
                            return TCL_ERROR;
                        }
                        /* append option value */
                        if (RpTclOptionGet(interp, optSpecPtr,
                            (ClientData)envPtr->paramList[n],
                            optSpecPtr[j].optname) != TCL_OK) {
                            Tcl_DecrRefCount(rrval);
                            Tcl_DecrRefCount(rval);
                            return TCL_ERROR;
                        }
                        if (Tcl_ListObjAppendElement(interp, rrval,
                              Tcl_GetObjResult(interp)) != TCL_OK) {
                            Tcl_DecrRefCount(rrval);
                            Tcl_DecrRefCount(rval);
                            return TCL_ERROR;
                        }
                    }
                } else {
                    if (RpTclOptionGet(interp, optSpecPtr,
                        (ClientData)envPtr->paramList[n], option) != TCL_OK) {
                        Tcl_DecrRefCount(rrval);
                        Tcl_DecrRefCount(rval);
                        return TCL_ERROR;
                    }
                    if (Tcl_ListObjAppendElement(interp, rrval,
                          Tcl_GetObjResult(interp)) != TCL_OK) {
                        Tcl_DecrRefCount(rrval);
                        Tcl_DecrRefCount(rval);
                        return TCL_ERROR;
                    }
                }
                if (Tcl_ListObjAppendElement(interp, rval, rrval) != TCL_OK) {
                    Tcl_DecrRefCount(rrval);
                    Tcl_DecrRefCount(rval);
                    return TCL_ERROR;
                }
                Tcl_DecrRefCount(rrval);
            }
        }

        if (nmatches == 1) {
            /* only one result? then return it directly */
            Tcl_ListObjIndex(interp, rval, 0, &rrval);
            Tcl_SetObjResult(interp, rrval);
        } else {
            /* return a whole list */
            Tcl_SetObjResult(interp, rval);
        }
        Tcl_DecrRefCount(rval);
        return TCL_OK;
    }

    /*
     * OPTION:  configure ?-option? ?value -option value ...?
     */
    else if (*option == 'c' && strcmp(option,"configure") == 0) {
        optSpecPtr = pluginDataPtr->pluginDefn->optionSpec;
        if (objc == 2) {
            /* report all values: -option val -option val ... */

            rval = Tcl_NewListObj(0,NULL);
            Tcl_IncrRefCount(rval);

            for (n=0; optSpecPtr[n].optname; n++) {
                if (RpTclOptionGet(interp, optSpecPtr,
                    (ClientData)pluginDataPtr->clientData,
                    optSpecPtr[n].optname) != TCL_OK) {
                    Tcl_DecrRefCount(rval);
                    return TCL_ERROR;
                }
                if (Tcl_ListObjAppendElement(interp, rval,
                      Tcl_NewStringObj(optSpecPtr[n].optname,-1)) != TCL_OK) {
                    Tcl_DecrRefCount(rval);
                    return TCL_ERROR;
                }
                if (Tcl_ListObjAppendElement(interp, rval,
                      Tcl_GetObjResult(interp)) != TCL_OK) {
                    Tcl_DecrRefCount(rval);
                    return TCL_ERROR;
                }
            }
            Tcl_SetObjResult(interp, rval);
            Tcl_DecrRefCount(rval);
            return TCL_OK;
        }
        else if (objc == 3) {
            /* report the value for just one option */
            option = Tcl_GetStringFromObj(objv[2], (int*)NULL);
            return RpTclOptionGet(interp, optSpecPtr,
                (ClientData)pluginDataPtr->clientData, option);
        }
        else {
            return RpTclOptionsProcess(interp, objc-2, objv+2,
                optSpecPtr, pluginDataPtr->clientData);
        }
    }

    /*
     * OPTION:  perform ?-maxruns num? ?-abortvar name?
     */
    else if (*option == 'p' && strcmp(option,"perform") == 0) {
    }

    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad option \"", option, "\": should be add, perform",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimInstanceCleanup()
 *
 * Called whenever a optimizer environment is being delete to clean
 * up any plugin data associated with it.  It's a little convoluted.
 * Here's the sequence:  A Tcl command is deleted, RpOptimCmdDelete()
 * gets called to clean it up, RpOptimDelete() is called within that,
 * and this method gets called to clean up the client data associated
 * with the underlying environment.
 * ----------------------------------------------------------------------
 */
static void
RpOptimInstanceCleanup(cdata)
    ClientData cdata;   /* plugin data being deleted */
{
    RpOptimPluginData *pluginDataPtr = (RpOptimPluginData*)cdata;

    /* if there are config options, clean them up first */
    if (pluginDataPtr->pluginDefn->optionSpec) {
        RpTclOptionsCleanup(pluginDataPtr->pluginDefn->optionSpec,
            pluginDataPtr->clientData);
    }

    /* call a specialized cleanup routine to handle the rest */
    if (pluginDataPtr->pluginDefn->cleanupPtr) {
        (*pluginDataPtr->pluginDefn->cleanupPtr)(pluginDataPtr->clientData);
    }
    pluginDataPtr->clientData = NULL;

    /* free the container */
    free((char*)pluginDataPtr);
}
