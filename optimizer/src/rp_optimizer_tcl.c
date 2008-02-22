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
RpOptimHandler PgapackRun;
RpOptimCleanup PgapackCleanup;
extern RpTclOption PgapackOptions;

static RpOptimPlugin rpOptimPlugins[] = {
    {"pgapack", PgapackInit, PgapackRun, PgapackCleanup, &PgapackOptions},
    {NULL, NULL, NULL},
};

typedef struct RpOptimPluginData {
    RpOptimPlugin *pluginDefn;      /* points back to plugin definition */
    ClientData clientData;          /* data needed for particular plugin */
} RpOptimPluginData;

typedef struct RpOptimToolData {
    Tcl_Interp *interp;             /* interp handling this tool */
    Tcl_Obj *toolPtr;               /* command for tool object */
    Tcl_Obj *updateCmdPtr;          /* command used to look for abort */
} RpOptimToolData;

/*
 * ----------------------------------------------------------------------
 *  Options for the various parameter types
 * ----------------------------------------------------------------------
 */
RpTclOption rpOptimNumberOpts[] = {
  {"-min", RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,min)},
  {"-max", RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,max)},
  {NULL, NULL, 0}
};

RpTclOption rpOptimStringOpts[] = {
  {"-values", RP_OPTION_LIST, Rp_Offset(RpOptimParamString,values)},
  {NULL, NULL, 0}
};

static int RpOptimizerCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static void RpOptimCmdDelete _ANSI_ARGS_((ClientData cdata));
static int RpOptimInstanceCmd _ANSI_ARGS_((ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static void RpOptimInstanceCleanup _ANSI_ARGS_((ClientData cdata));
static RpOptimStatus RpOptimizerPerformInTcl _ANSI_ARGS_((RpOptimEnv *envPtr,
    RpOptimParam *values, int numValues, double *fitnessPtr));
static int RpOptimizerUpdateInTcl _ANSI_ARGS_((RpOptimEnv *envPtr));

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

    /* no good default for the tool being optimized */
    Tcl_Obj *toolPtr = NULL;

    /* no name for this object by default */
    char *name = NULL;

    RpOptimEnv* envPtr;
    RpOptimPlugin* pluginPtr;
    RpOptimPluginData* pluginDataPtr;
    RpOptimToolData* toolDataPtr;

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
        else if (strcmp(option,"-tool") == 0) {
            if (n+1 >= objc) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "missing value for option \"", option, "\"",
                    (char*)NULL);
                return TCL_ERROR;
            }
            toolPtr = objv[n+1];
            Tcl_IncrRefCount(toolPtr);
            n += 2;
        }
        else {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option \"", option, "\": should be ",
                "-tool, -using", (char*)NULL);
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
    if (usingPluginPtr->initProc) {
        pluginDataPtr->clientData = (*usingPluginPtr->initProc)();
    }
    envPtr = RpOptimCreate((ClientData)pluginDataPtr, RpOptimInstanceCleanup);

    toolDataPtr = (RpOptimToolData*)malloc(sizeof(RpOptimToolData));
    toolDataPtr->interp = interp;
    toolDataPtr->toolPtr = toolPtr;
    toolDataPtr->updateCmdPtr = NULL;
    envPtr->toolData = (ClientData)toolDataPtr;

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
    RpOptimToolData *toolDataPtr;
    int n;
    ClientData paramdata;

    if (envPtr->toolData) {
        toolDataPtr = (RpOptimToolData*)envPtr->toolData;
        if (toolDataPtr->toolPtr) {
            Tcl_DecrRefCount(toolDataPtr->toolPtr);
        }
        if (toolDataPtr->updateCmdPtr) {
            Tcl_DecrRefCount(toolDataPtr->updateCmdPtr);
        }
        free(toolDataPtr);
        envPtr->toolData = NULL;
    }

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
 *      <name> perform ?-tool <tool>? ?-updatecommand <varName>?
 *      <name> using
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
    RpOptimToolData* toolDataPtr = (RpOptimToolData*)envPtr->toolData;

    int n, j, nvals, nmatches;
    char *option, *type, *path;
    RpOptimParam *paramPtr;
    RpOptimParamString *strPtr;
    RpOptimStatus status;
    RpTclOption *optSpecPtr;
    Tcl_Obj *rval, *rrval, *toolPtr, *updateCmdPtr;

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

            /* list of values just changed -- patch up the count */
            strPtr = (RpOptimParamString*)paramPtr;
            for (nvals=0; strPtr->values[nvals]; nvals++)
                ; /* count the values */
            strPtr->numValues = nvals;
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
     * OPTION:  perform ?-tool name? ?-updatecommand name?
     */
    else if (*option == 'p' && strcmp(option,"perform") == 0) {
        /* use this tool by default */
        toolPtr = toolDataPtr->toolPtr;

        /* no -updatecommand by default */
        updateCmdPtr = NULL;

        n = 2;
        while (n < objc) {
            option = Tcl_GetStringFromObj(objv[n], (int*)NULL);
            if (n+1 >= objc) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "missing value for option \"", option, "\"",
                    (char*)NULL);
                return TCL_ERROR;
            }
            if (strcmp(option,"-tool") == 0) {
                toolPtr = objv[n+1];
                n += 2;
            }
            else if (strcmp(option,"-updatecommand") == 0) {
                updateCmdPtr = objv[n+1];
                n += 2;
            }
            else {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "bad option \"", option, "\": should be -tool,"
                    " -updatecommand", (char*)NULL);
                return TCL_ERROR;
            }
        }

        /*
         * Must have a tool object at this point, or else we
         * don't know what to optimize.
         */
        if (toolPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "tool being optimized not specified via -tool option",
                (char*)NULL);
            return TCL_ERROR;
        }

        Tcl_IncrRefCount(toolPtr);
        if (updateCmdPtr) {
            Tcl_IncrRefCount(updateCmdPtr);
            toolDataPtr->updateCmdPtr = updateCmdPtr;
        }

        /* call the main optimization routine here */
        status = (*pluginDataPtr->pluginDefn->runProc)(envPtr,
            RpOptimizerPerformInTcl);

        Tcl_DecrRefCount(toolPtr);
        if (updateCmdPtr) {
            Tcl_DecrRefCount(updateCmdPtr);
            toolDataPtr->updateCmdPtr = NULL;
        }

        switch (status) {
        case RP_OPTIM_SUCCESS:
            Tcl_SetResult(interp, "success", TCL_STATIC);
            break;
        case RP_OPTIM_FAILURE:
            Tcl_SetResult(interp, "failure", TCL_STATIC);
            break;
        case RP_OPTIM_ABORTED:
            Tcl_SetResult(interp, "aborted", TCL_STATIC);
            break;
        case RP_OPTIM_UNKNOWN:
        default:
            Tcl_SetResult(interp, "???", TCL_STATIC);
            break;
        }
        return TCL_OK;
    }

    /*
     * OPTION:  using
     */
    else if (*option == 'u' && strcmp(option,"using") == 0) {
        if (objc > 2) {
            Tcl_WrongNumArgs(interp, 1, objv, "using");
            return TCL_ERROR;
        }
        Tcl_SetResult(interp, pluginDataPtr->pluginDefn->name, TCL_STATIC);
        return TCL_OK;
    }

    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad option \"", option, "\": should be add, configure, "
            "get, perform, using", (char*)NULL);
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
    if (pluginDataPtr->pluginDefn->cleanupProc) {
        (*pluginDataPtr->pluginDefn->cleanupProc)(pluginDataPtr->clientData);
    }
    pluginDataPtr->clientData = NULL;

    /* free the container */
    free((char*)pluginDataPtr);
}

/*
 * ------------------------------------------------------------------------
 *  RpOptimizerPerformInTcl()
 *
 *  Invoked as a call-back within RpOptimPerform() to handle each
 *  optimization run.  Launches a run of a Rappture-based tool using
 *  the given values and computes the value for the fitness function.
 *
 *  Returns RP_OPTIM_SUCCESS if the run was successful, along with
 *  the value in the fitness function in fitnessPtr.  If something
 *  goes wrong with the run, it returns RP_OPTIM_FAILURE.
 * ------------------------------------------------------------------------
 */
static RpOptimStatus
RpOptimizerPerformInTcl(envPtr, values, numValues, fitnessPtr)
    RpOptimEnv *envPtr;       /* optimization environment */
    RpOptimParam *values;     /* incoming values for the simulation */
    int numValues;            /* number of incoming values */
    double *fitnessPtr;       /* returns: computed value of fitness func */
{
    printf("running...\n");
    *fitnessPtr = 0.0;
    return RP_OPTIM_SUCCESS;
}

/*
 * ------------------------------------------------------------------------
 *  RpOptimizerUpdateInTcl()
 *
 *  Invoked as a call-back within RpOptimPerform() to update the
 *  application and look for an "abort" signal.  Evaluates a bit of
 *  optional code stored in the optimization environment.  Returns 0
 *  if everything is okay, and non-zero if the user wants to abort.
 * ------------------------------------------------------------------------
 */
static int
RpOptimizerUpdateInTcl(envPtr)
    RpOptimEnv *envPtr;       /* optimization environment */
{
    RpOptimToolData *toolDataPtr = (RpOptimToolData*)envPtr->toolData;
    int status;

    if (toolDataPtr->updateCmdPtr) {
        status = Tcl_GlobalEvalObj(toolDataPtr->interp,
            toolDataPtr->updateCmdPtr);

        if (status == TCL_ERROR) {
            Tcl_BackgroundError(toolDataPtr->interp);
            return 0;
        }
        if (status == TCL_BREAK || status == TCL_RETURN) {
            return 1;  /* abort! */
        }
    }
    return 0;  /* keep going... */
}
