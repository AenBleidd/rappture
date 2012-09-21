/*
 * ----------------------------------------------------------------------
 *  rp_optimizer_tcl
 *
 *  This is the Tcl API for the functions in rp_optimizer.  This code
 *  allows you to call all of the core optimization functions from Tcl.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "rp_optimizer.h"

extern int pgapack_abort;
extern int pgapack_restart_user_action;

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
 
RpCustomTclOptionGet RpOption_GetRandDist;
RpCustomTclOptionParse RpOption_ParseRandDist;
RpTclOptionType RpOption_RandDist = {
	"pga_randdist", RpOption_ParseRandDist,RpOption_GetRandDist,NULL
}; 
RpTclOption rpOptimNumberOpts[] = {
  {"-min", RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,min)},
  {"-max", RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,max)},
  {"-mutnrate",RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,mutnrate)},
  {"-mutnValue",RP_OPTION_DOUBLE, Rp_Offset(RpOptimParamNumber,mutnValue)},
  {"-randdist",&RpOption_RandDist,Rp_Offset(RpOptimParamNumber,randdist)},
  {"-strictmin",RP_OPTION_BOOLEAN,Rp_Offset(RpOptimParamNumber,strictmin)},
  {"-strictmax",RP_OPTION_BOOLEAN,Rp_Offset(RpOptimParamNumber,strictmax)},
  {"-stddev",RP_OPTION_DOUBLE,Rp_Offset(RpOptimParamNumber,stddev)},
  {"-mean",RP_OPTION_DOUBLE,Rp_Offset(RpOptimParamNumber,mean)},
  {"-units",RP_OPTION_STRING,Rp_Offset(RpOptimParamNumber,units)},
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
static RpOptimStatus RpOptimizerPerformInTcl _ANSI_ARGS_((RpOptimEnv *envPtr,
    RpOptimParam *values, int numValues, double *fitnessPtr));

#ifdef BUILD_Rappture
__declspec( dllexport )
#endif


extern void PGARuntimeDataTableInit();
extern void PGARuntimeDataTableDeInit();
extern void GetSampleInformation();

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
    envPtr = RpOptimCreate(usingPluginPtr);

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

    PGARuntimeDataTableDeInit();/*Free space allocated to data table here*/
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
 *      <name> perform ?-tool <tool>? ?-fitness <expr>? \
 *                     ?-updatecommand <varName>?
 *      <name> using
 *      <name> samples ?number?
 *
 *  The "add" command is used to add various parameter types to the
 *  optimizer context.  The "perform" command kicks off an optimization
 *  run. The "samples" command displays sample info during an optimization run.
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
    RpOptimToolData* toolDataPtr = (RpOptimToolData*)envPtr->toolData;

    int n, j, nvals, nmatches;
    char *option, *type, *path, *fitnessExpr;
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
    } else if (*option == 'a' && strcmp(option,"abort") == 0) {
	int value;

        if (objc < 3) {
            Tcl_WrongNumArgs(interp, 1, objv, "abort bool");
            return TCL_ERROR;
        }
	if (Tcl_GetBooleanFromObj(interp, objv[2], &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	pgapack_abort = value;
	return TCL_OK;
    }else if (*option == 'r' && strcmp(option,"restart") == 0){
		int value;
        if (objc < 3) {
            Tcl_WrongNumArgs(interp, 1, objv, "restart bool");
            return TCL_ERROR;
        }
		if (Tcl_GetBooleanFromObj(interp, objv[2], &value) != TCL_OK) {
	    	return TCL_ERROR;
		}
		pgapack_restart_user_action = value;
		return TCL_OK;    
	}else if (*option == 'g' && strcmp(option,"get") == 0) {
	/*
	 * OPTION:  get ?globPattern? ?-option?
	 */
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
                    if (option == NULL) {
                        /* no particular option value? then include type */
                        if (Tcl_ListObjAppendElement(interp, rrval,
                              Tcl_NewStringObj("number",-1)) != TCL_OK) {
                            Tcl_DecrRefCount(rrval);
                            Tcl_DecrRefCount(rval);
                            return TCL_ERROR;
                        }
                    }
                    break;
                case RP_OPTIMPARAM_STRING:
                    optSpecPtr = rpOptimStringOpts;
                    if (option == NULL) {
                        /* no particular option value? then include type */
                        if (Tcl_ListObjAppendElement(interp, rrval,
                              Tcl_NewStringObj("string",-1)) != TCL_OK) {
                            Tcl_DecrRefCount(rrval);
                            Tcl_DecrRefCount(rval);
                            return TCL_ERROR;
                        }
                    }
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
        optSpecPtr = envPtr->pluginDefn->optionSpec;
        if (objc == 2) {
            /* report all values: -option val -option val ... */

            rval = Tcl_NewListObj(0,NULL);
            Tcl_IncrRefCount(rval);

            for (n=0; optSpecPtr[n].optname; n++) {
                if (RpTclOptionGet(interp, optSpecPtr,
                    (ClientData)envPtr->pluginData,
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
                (ClientData)envPtr->pluginData, option);
        }
        else {
            return RpTclOptionsProcess(interp, objc-2, objv+2,
                optSpecPtr, envPtr->pluginData);
        }
    }

    /*
     * OPTION:  perform ?-tool name? ?-fitness expr? ?-updatecommand name?
     */
    else if (*option == 'p' && strcmp(option,"perform") == 0) {
        /* use this tool by default */
        toolPtr = toolDataPtr->toolPtr;

        /* no -fitness function by default */
        fitnessExpr = NULL;

        /* no -updatecommand by default */
        updateCmdPtr = NULL;
		
		PGARuntimeDataTableInit(envPtr);/*Initialize Data table here....*/
		
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
            else if (strcmp(option,"-fitness") == 0) {
                fitnessExpr = Tcl_GetStringFromObj(objv[n+1], (int*)NULL);
                n += 2;
            }
            else if (strcmp(option,"-updatecommand") == 0) {
                updateCmdPtr = objv[n+1];
                n += 2;
            }
            else {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "bad option \"", option, "\": should be -fitness, -tool,"
                    " -updatecommand", (char*)NULL);
                return TCL_ERROR;
            }
        }

        /*
         * Must have a tool object and a fitness function at this point,
         * or else we don't know what to optimize.
         */
        if (toolPtr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "tool being optimized not specified via -tool option",
                (char*)NULL);
            return TCL_ERROR;
        }
        if (fitnessExpr == NULL) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "missing -fitness function for optimization",
                (char*)NULL);
            return TCL_ERROR;
        }

        Tcl_IncrRefCount(toolPtr);
        if (updateCmdPtr) {
            Tcl_IncrRefCount(updateCmdPtr);
            toolDataPtr->updateCmdPtr = updateCmdPtr;
        }
		
        /* call the main optimization routine here */
        status = (*envPtr->pluginDefn->runProc)(envPtr,
            RpOptimizerPerformInTcl, fitnessExpr);
		
		fprintf(stderr, ">>>status=%d\n", status);

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
	    fprintf(stderr, "Got abort status=%d\n", status);
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
        Tcl_SetResult(interp, envPtr->pluginDefn->name, TCL_STATIC);

        /* if the -tool was specified, then add it as a second element */
        toolDataPtr = (RpOptimToolData*)envPtr->toolData;
        if (toolDataPtr->toolPtr) {
            Tcl_AppendElement(interp,
                Tcl_GetStringFromObj(toolDataPtr->toolPtr, (int*)NULL));
        }
        return TCL_OK;
    }
    
    else if(*option == 's' && strcmp(option,"samples") == 0){
    	int sampleNumber = -1; /*initing sampnum to -1, use it when no sample number is specified*/
    	char *sampleDataBuffer;
    	if(objc>3){
    		Tcl_WrongNumArgs(interp, 2, objv, "?sampleNumber?");
            return TCL_ERROR;
    	}
    	
    	if(objc == 3){
	    	if(Tcl_GetIntFromObj(interp,objv[2],&sampleNumber) != TCL_OK){
	    		return TCL_ERROR; 
	    	}
    		sampleDataBuffer = malloc(sizeof(char)*SINGLE_SAMPLE_DATA_BUFFER_DEFAULT_SIZE);
    	}else{
    		sampleDataBuffer = malloc(sizeof(char)*50);
    	}
    	
    	if(sampleDataBuffer == NULL){
    		panic("Error: Could not allocate memory for sample data buffer.");
    	}
    	GetSampleInformation(sampleDataBuffer,sampleNumber);
		fprintf(stdout,sampleDataBuffer);/**TODO GTG check if this should be fprintf or something else*/
		free(sampleDataBuffer);
    	return TCL_OK;
    	
    }

    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad option \"", option, "\": should be add, configure, "
            "get, perform, using, samples", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
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
    RpOptimStatus result = RP_OPTIM_SUCCESS;
    Tcl_Obj *xmlObj = NULL;
    RpOptimToolData *toolDataPtr = (RpOptimToolData*)envPtr->toolData;
    Tcl_Interp *interp = toolDataPtr->interp;

    int n, status;
#define MAXBUILTIN 10
    int objc; Tcl_Obj **objv, *storage[MAXBUILTIN], *getcmd[3];
    int rc; Tcl_Obj **rv;
    Tcl_Obj *dataPtr;
    Tcl_DString buffer;
    RpOptimParamNumber *numPtr;
    char dvalBuffer[50];

    /*
     * Set up the arguments for a Tcl evaluation.
     */
    objc = 2*numValues + 2;  /* "tool run" + (name value)*numValues */
    if (objc > MAXBUILTIN) {
        objv = (Tcl_Obj**)malloc(objc*sizeof(Tcl_Obj));
    } else {
        objv = storage;
    }
    objv[0] = toolDataPtr->toolPtr;
    objv[1] = Tcl_NewStringObj("run",-1); Tcl_IncrRefCount(objv[1]);
    for (n=0; n < numValues; n++) {
        objv[2*n+2] = Tcl_NewStringObj(values[n].name, -1);
        Tcl_IncrRefCount(objv[2*n+2]);

        switch (values[n].type) {
        case RP_OPTIMPARAM_NUMBER:
        	numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
        	status = sprintf(dvalBuffer,"%lf%s",values[n].value.dval,numPtr->units);
        	if(status<0){
        		panic("Could not convert number into number+units format");
        	}
            objv[2*n+3] = Tcl_NewStringObj(dvalBuffer,-1);
            Tcl_IncrRefCount(objv[2*n+3]);
            break;
        case RP_OPTIMPARAM_STRING:
            objv[2*n+3] = Tcl_NewStringObj(values[n].value.sval.str,-1);
            Tcl_IncrRefCount(objv[2*n+3]);
            break;
        default:
            panic("bad parameter type in RpOptimizerPerformInTcl()");
        }
    }

    /*
     *  Invoke the tool and pick apart its results.
     */
    status = Tcl_EvalObjv(interp, objc, objv, TCL_EVAL_GLOBAL);

    if (status != TCL_OK) {
        result = RP_OPTIM_FAILURE;
        fprintf(stderr, "== JOB FAILED: %s\n", Tcl_GetStringResult(interp));
    } else {
        dataPtr = Tcl_GetObjResult(interp);
        /* hang on to this while we pick it apart into rv[] */
        Tcl_IncrRefCount(dataPtr);

        if (Tcl_ListObjGetElements(interp, dataPtr, &rc, &rv) != TCL_OK) {
            result = RP_OPTIM_FAILURE;
            fprintf(stderr, "== JOB FAILED: %s\n", Tcl_GetStringResult(interp));
        } else if (rc != 2
                    || Tcl_GetIntFromObj(interp, rv[0], &status) != TCL_OK) {
            result = RP_OPTIM_FAILURE;
            fprintf(stderr, "== JOB FAILED: malformed result: expected {status output}\n");
        } else {
            if (status != 0) {
                result = RP_OPTIM_FAILURE;
                fprintf(stderr, "== JOB FAILED with status code %d:\n%s\n",
                    status, Tcl_GetStringFromObj(rv[1], (int*)NULL));
            } else {
                /*
                 *  Get the output value from the tool output in the
                 *  result we just parsed above:  {status xmlobj}
                 *
                 *  Eventually, we should write a whole parser to
                 *  handle arbitrary fitness functions.  For now,
                 *  just query a single output value by calling:
                 *    xmlobj get fitnessExpr
                 */
                xmlObj = rv[1];
                /* hang onto this for -updatecommand below */
                Tcl_IncrRefCount(xmlObj);

                getcmd[0] = xmlObj;
                getcmd[1] = Tcl_NewStringObj("get",-1);
                getcmd[2] = Tcl_NewStringObj(envPtr->fitnessExpr,-1);
                for (n=0; n < 3; n++) {
                    Tcl_IncrRefCount(getcmd[n]);
                }

                status = Tcl_EvalObjv(interp, 3, getcmd, TCL_EVAL_GLOBAL);

                if (status != TCL_OK) {
                    result = RP_OPTIM_FAILURE;
                    fprintf(stderr, "==UNEXPECTED ERROR while extracting output value:%s\n", Tcl_GetStringResult(interp));
                } else if (Tcl_GetDoubleFromObj(interp,
                      Tcl_GetObjResult(interp), fitnessPtr) != TCL_OK) {
                    result = RP_OPTIM_FAILURE;
                    fprintf(stderr, "==ERROR while extracting output value:%s\n", Tcl_GetStringResult(interp));
                }
                for (n=0; n < 3; n++) {
                    Tcl_DecrRefCount(getcmd[n]);
                }
            }
        }
        Tcl_DecrRefCount(dataPtr);
    }

    /*
     * Clean up objects created for command invocation.
     */
    for (n=1; n < objc; n++) {
        Tcl_DecrRefCount(objv[n]);
    }
    if (objv != storage) {
        free(objv);
    }

    /*
     * If there's the -updatecommand was specified, execute it here
     * to bring the application up-to-date and see if the user wants
     * to abort.
     */
    if (toolDataPtr->updateCmdPtr) {
        Tcl_DStringInit(&buffer);
        Tcl_DStringAppend(&buffer,
            Tcl_GetStringFromObj(toolDataPtr->updateCmdPtr, (int*)NULL), -1);
        Tcl_DStringAppendElement(&buffer,
            (xmlObj != NULL) ? Tcl_GetStringFromObj(xmlObj, (int*)NULL): "");

        status = Tcl_GlobalEval(toolDataPtr->interp,
            Tcl_DStringValue(&buffer));
        if (status == TCL_ERROR) {
            Tcl_BackgroundError(toolDataPtr->interp);
        } 
        Tcl_DStringFree(&buffer);
    }

    if (xmlObj) {
        Tcl_DecrRefCount(xmlObj);  /* done with this now */
    }
    return result;
}

/*
 * ======================================================================
 *  OPTION:  -randdist <=> RAND_NUMBER_DIST_GAUSSIAN / RAND_NUMBER_DIST_UNIFORM 
 * ======================================================================
 */
int
RpOption_ParseRandDist(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (strcmp(val,"gaussian") == 0) {
        *ptr = RAND_NUMBER_DIST_GAUSSIAN;
    }
    else if (strcmp(val,"uniform") == 0) {
        *ptr = RAND_NUMBER_DIST_UNIFORM;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be gaussian or uniform",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetRandDist(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case RAND_NUMBER_DIST_GAUSSIAN:
        Tcl_SetResult(interp, "gaussian", TCL_STATIC);
        break;
    case RAND_NUMBER_DIST_UNIFORM:
        Tcl_SetResult(interp, "uniform", TCL_STATIC);
        break;
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}
