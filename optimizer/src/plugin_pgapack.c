/*
 * ----------------------------------------------------------------------
 *  OPTIMIZER PLUG-IN:  Pgapack
 *
 *  This code connects Pgapack into the Rappture Optimization
 *  infrastructure.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "pgapack.h"
#include "rp_optimizer_plugin.h"

typedef struct PgapackData {
    int operation;       /* operation <=> PGA_MINIMIZE/PGA_MAXIMIZE */
    int maxRuns;         /* maximum runs <=> PGASetMaxGAIterValue() */
    int popSize;         /* population size <=> PGASetPopSize() */
    int popRepl;         /* replacement <=> PGASetPopReplacementType() */
} PgapackData;

RpCustomTclOptionParse RpOption_ParseOper;
RpCustomTclOptionGet RpOption_GetOper;
RpTclOptionType RpOption_Oper = {
    "pga_operation", RpOption_ParseOper, RpOption_GetOper, NULL
};

RpCustomTclOptionParse RpOption_ParsePopRepl;
RpCustomTclOptionGet RpOption_GetPopRepl;
RpTclOptionType RpOption_PopRepl = {
    "pga_poprepl", RpOption_ParsePopRepl, RpOption_GetPopRepl, NULL
};

RpTclOption PgapackOptions[] = {
  {"-maxruns", RP_OPTION_INT, Rp_Offset(PgapackData,maxRuns)},
  {"-operation", &RpOption_Oper, Rp_Offset(PgapackData,operation)},
  {"-poprepl", &RpOption_PopRepl, Rp_Offset(PgapackData,popRepl)},
  {"-popsize", RP_OPTION_INT, Rp_Offset(PgapackData,popSize)},
  {NULL, NULL, 0}
};

static double PgapEvaluate _ANSI_ARGS_((PGAContext *ctx, int p, int pop));
static void PgapCreateString _ANSI_ARGS_((PGAContext *ctx, int, int, int));
static int PgapMutation _ANSI_ARGS_((PGAContext *ctx, int, int, double));
static void PgapCrossover _ANSI_ARGS_((PGAContext *ctx, int, int, int,
    int, int, int));
static void PgapPrintString _ANSI_ARGS_((PGAContext *ctx, FILE*, int, int));
static void PgapCopyString _ANSI_ARGS_((PGAContext *ctx, int, int, int, int));
static int PgapDuplicateString _ANSI_ARGS_((PGAContext *ctx, int, int, int, int));
static MPI_Datatype PgapBuildDT _ANSI_ARGS_((PGAContext *ctx, int, int));

static void PgapLinkContext2Env _ANSI_ARGS_((PGAContext *ctx,
    RpOptimEnv *envPtr));
static RpOptimEnv* PgapGetEnvForContext _ANSI_ARGS_((PGAContext *ctx));
static void PgapUnlinkContext2Env _ANSI_ARGS_((PGAContext *ctx));


/*
 * ----------------------------------------------------------------------
 * PgapackInit()
 *
 * This routine is called whenever a new optimization object is created
 * to initialize Pgapack.  Returns a pointer to PgapackData that is
 * used in later routines.
 * ----------------------------------------------------------------------
 */
ClientData
PgapackInit()
{
    PgapackData *dataPtr;

    dataPtr = (PgapackData*)malloc(sizeof(PgapackData));
    dataPtr->operation = PGA_MINIMIZE;
    dataPtr->maxRuns = 10000;
    dataPtr->popRepl = PGA_POPREPL_BEST;
    dataPtr->popSize = 200;

    return (ClientData)dataPtr;
}

/*
 * ----------------------------------------------------------------------
 * PgapackRun()
 *
 * This routine is called to kick off an optimization run.  Sets up
 * a PGApack context and starts invoking runs.
 * ----------------------------------------------------------------------
 */
RpOptimStatus
PgapackRun(envPtr, evalProc)
    RpOptimEnv *envPtr;           /* optimization environment */
    RpOptimEvaluator *evalProc;   /* call this proc to run tool */
{
    PgapackData *dataPtr =(PgapackData*)envPtr->pluginData;
    PGAContext *ctx;

    /* pgapack requires at least one arg -- the executable name */
    /* fake it here by just saying something like "rappture" */
    int argc = 1; char *argv[] = {"rappture"};

    ctx = PGACreate(&argc, argv, PGA_DATATYPE_USER, envPtr->numParams,
        dataPtr->operation);

    PGASetMaxGAIterValue(ctx, dataPtr->maxRuns);
    PGASetPopSize(ctx, dataPtr->popSize);
    PGASetPopReplaceType(ctx, dataPtr->popRepl);

    PGASetUserFunction(ctx, PGA_USERFUNCTION_CREATESTRING, PgapCreateString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_MUTATION, PgapMutation);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_CROSSOVER, PgapCrossover);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_PRINTSTRING, PgapPrintString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_COPYSTRING, PgapCopyString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_DUPLICATE, PgapDuplicateString);
    PGASetUserFunction(ctx, PGA_USERFUNCTION_BUILDDATATYPE, PgapBuildDT);

    envPtr->evalProc = evalProc;  /* call this later for evaluations */

    /*
     * We need a way to convert from a PGAContext to our RpOptimEnv
     * data.  This happens when Pgapack calls routines like
     * PgapCreateString, passing in the PGAContext, but nothing else.
     * Call PgapLinkContext2Env() here, so later on we can figure
     * out how many parameters, names, types, etc.
     */
    PgapLinkContext2Env(ctx, envPtr);

    PGASetUp(ctx);
    PGARun(ctx, PgapEvaluate);
    PGADestroy(ctx);

    PgapUnlinkContext2Env(ctx);

    return RP_OPTIM_SUCCESS;
}

/*
 * ----------------------------------------------------------------------
 * PgapackEvaluate()
 *
 * Called by PGApack whenever a set of input values needs to be
 * evaluated.  Passes the values on to the underlying Rappture tool,
 * launches a run, and computes the value of the fitness function.
 * Returns the value for the fitness function.
 * ----------------------------------------------------------------------
 */
double
PgapEvaluate(ctx, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
{
    double fit = 0.0;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;
    RpOptimStatus status;

    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;

    status = (*envPtr->evalProc)(envPtr, paramPtr, envPtr->numParams, &fit);

    if (status != RP_OPTIM_SUCCESS) {
        fprintf(stderr, "==WARNING: run failed!");
        PgapPrintString(ctx, stderr, p, pop);
    }

    return fit;
}

/*
 * ----------------------------------------------------------------------
 * PgapackCleanup()
 *
 * This routine is called whenever an optimization object is deleted
 * to clean up data associated with the object.  Frees the data
 * allocated in PgapackInit.
 * ----------------------------------------------------------------------
 */
void
PgapackCleanup(cdata)
    ClientData cdata;  /* data from to be cleaned up */
{
    PgapackData *dataPtr = (PgapackData*)cdata;
    free(dataPtr);
}

/*
 * ======================================================================
 *  ROUTINES FOR MANAGING DATA STRINGS
 * ======================================================================
 * PgapCreateString()
 *
 * Called by pgapack to create the so-called "string" of data used for
 * an evaluation.
 * ----------------------------------------------------------------------
 */
void
PgapCreateString(ctx, p, pop, initFlag)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
    int initFlag;     /* non-zero => fields should be initialized */
{
    int n, ival;
    double dval;
    RpOptimEnv *envPtr;
    RpOptimParam *oldParamPtr, *newParamPtr;
    PGAIndividual *newData;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    envPtr = PgapGetEnvForContext(ctx);

    newData = PGAGetIndividual(ctx, p, pop);
    newData->chrom = malloc(envPtr->numParams*sizeof(RpOptimParam));
    newParamPtr = (RpOptimParam*)newData->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        oldParamPtr = envPtr->paramList[n];
        newParamPtr[n].name = oldParamPtr->name;
        newParamPtr[n].type = oldParamPtr->type;
        switch (oldParamPtr->type) {
        case RP_OPTIMPARAM_NUMBER:
            newParamPtr[n].value.dval = 0.0;
            break;
        case RP_OPTIMPARAM_STRING:
            newParamPtr[n].value.sval.num = -1;
            newParamPtr[n].value.sval.str = NULL;
            break;
        default:
            panic("bad parameter type in PgapCreateString()");
        }
    }

    if (initFlag) {
        for (n=0; n < envPtr->numParams; n++) {
            switch (newParamPtr[n].type) {
            case RP_OPTIMPARAM_NUMBER:
                numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
                dval = PGARandom01(ctx,0);
                newParamPtr[n].value.dval =
                    (numPtr->max - numPtr->min)*dval + numPtr->min;
                break;
            case RP_OPTIMPARAM_STRING:
                strPtr = (RpOptimParamString*)envPtr->paramList[n];
                ival = (int)floor(PGARandom01(ctx,0) * strPtr->numValues);
                envPtr->paramList[n]->value.sval.num = ival;
                envPtr->paramList[n]->value.sval.str = strPtr->values[ival];
                break;
            default:
                panic("bad parameter type in PgapCreateString()");
            }
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapMutation()
 *
 * Called by pgapack to perform random mutations on the input data
 * used for evaluation.
 * ----------------------------------------------------------------------
 */
int
PgapMutation(ctx, p, pop, mr)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
    double mr;        /* probability of mutation for each gene */
{
    int count = 0;    /* number of mutations */

    int n, ival;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        if (PGARandomFlip(ctx, mr)) {
            /* won the coin toss -- change this parameter */
            count++;

            switch (paramPtr[n].type) {
            case RP_OPTIMPARAM_NUMBER:
                /* bump the value up/down a little, randomly */
                if (PGARandomFlip(ctx, 0.5)) {
                    paramPtr[n].value.dval += 0.1*paramPtr[n].value.dval;
                } else {
                    paramPtr[n].value.dval -= 0.1*paramPtr[n].value.dval;
                }
                /* make sure the resulting value is still in bounds */
                numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
                if (paramPtr[n].value.dval > numPtr->max) {
                    paramPtr[n].value.dval = numPtr->max;
                }
                if (paramPtr[n].value.dval < numPtr->min) {
                    paramPtr[n].value.dval = numPtr->min;
                }
                break;

            case RP_OPTIMPARAM_STRING:
                ival = paramPtr[n].value.sval.num;
                if (PGARandomFlip(ctx, 0.5)) {
                    ival += 1;
                } else {
                    ival -= 1;
                }
                strPtr = (RpOptimParamString*)envPtr->paramList[n];
                if (ival < 0) ival = 0;
                if (ival >= strPtr->numValues) ival = strPtr->numValues-1;
                paramPtr[n].value.sval.num = ival;
                paramPtr[n].value.sval.str = strPtr->values[ival];
                break;

            default:
                panic("bad parameter type in PgapMutation()");
            }
        }
    }
    return count;
}

/*
 * ----------------------------------------------------------------------
 * PgapCrossover()
 *
 * Called by pgapack to perform cross-over mutations on the input data
 * used for evaluation.
 * ----------------------------------------------------------------------
 */
void
PgapCrossover(ctx, p1, p2, pop1, c1, c2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* sample # for parent of input string1 */
    int p2;           /* sample # for parent of input string2 */
    int pop1;         /* population containing p1 and p2 */
    int c1;           /* sample # for child of input string1 */
    int c2;           /* sample # for child of input string2 */
    int pop2;         /* population containing c1 and c2 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *parent1, *parent2, *child1, *child2;
    double pu;

    envPtr = PgapGetEnvForContext(ctx);
    parent1 = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    parent2 = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop1)->chrom;
    child1  = (RpOptimParam*)PGAGetIndividual(ctx, c1, pop2)->chrom;
    child2  = (RpOptimParam*)PGAGetIndividual(ctx, c2, pop2)->chrom;

    pu = PGAGetUniformCrossoverProb(ctx);

    for (n=0; n < envPtr->numParams; n++) {
        if (PGARandomFlip(ctx, pu)) {
            /* child inherits from parent */
            memcpy(&child1[n], &parent1[n], sizeof(RpOptimParam));
            memcpy(&child2[n], &parent2[n], sizeof(RpOptimParam));
        } else {
            /* crossover */
            memcpy(&child1[n], &parent2[n], sizeof(RpOptimParam));
            memcpy(&child2[n], &parent1[n], sizeof(RpOptimParam));
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapPrintString()
 *
 * Called by pgapack to format the values for a particular string of
 * input data.
 * ----------------------------------------------------------------------
 */
void
PgapPrintString(ctx, fp, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    FILE *fp;         /* write to this file pointer */
    int p;            /* sample #p being run */
    int pop;          /* identifier for this population */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *paramPtr;

    envPtr = PgapGetEnvForContext(ctx);
    paramPtr = (RpOptimParam*)PGAGetIndividual(ctx, p, pop)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        fprintf(fp, "#%4d: ", n);
        switch (paramPtr[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            fprintf(fp, "[%11.7g] (%s)\n", paramPtr[n].value.dval,
                paramPtr[n].name);
            break;
        case RP_OPTIMPARAM_STRING:
            fprintf(fp, "[%d]=\"%s\" (%s)\n", paramPtr[n].value.sval.num,
                paramPtr[n].value.sval.str, paramPtr[n].name);
            break;
        default:
            panic("bad parameter type in PgapPrintString()");
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapCopyString()
 *
 * Called by pgapack to copy one input string to another.
 * ----------------------------------------------------------------------
 */
void
PgapCopyString(ctx, p1, pop1, p2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* source sample # being run */
    int pop1;         /* population containing p1 */
    int p2;           /* destination sample # being run */
    int pop2;         /* population containing p1 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *src, *dst;

    envPtr = PgapGetEnvForContext(ctx);
    src = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    dst = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop2)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        dst[n].type = src[n].type;
        switch (src[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            dst[n].value.dval = src[n].value.dval;
            break;
        case RP_OPTIMPARAM_STRING:
            dst[n].value.sval.num = src[n].value.sval.num;
            dst[n].value.sval.str = src[n].value.sval.str;
            break;
        default:
            panic("bad parameter type in PgapCopyString()");
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * PgapDuplicateString()
 *
 * Called by pgapack to compare two input strings.  Returns non-zero if
 * the two are duplicates and 0 otherwise.
 * ----------------------------------------------------------------------
 */
int
PgapDuplicateString(ctx, p1, pop1, p2, pop2)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p1;           /* sample #p being run */
    int pop1;         /* population containing p1 */
    int p2;           /* sample #p being run */
    int pop2;         /* population containing p1 */
{
    int n;
    RpOptimEnv *envPtr;
    RpOptimParam *param1, *param2;

    envPtr = PgapGetEnvForContext(ctx);
    param1 = (RpOptimParam*)PGAGetIndividual(ctx, p1, pop1)->chrom;
    param2 = (RpOptimParam*)PGAGetIndividual(ctx, p2, pop2)->chrom;

    for (n=0; n < envPtr->numParams; n++) {
        if (param1[n].type != param2[n].type) {
            return 0;  /* different! */
        }
        switch (param1[n].type) {
        case RP_OPTIMPARAM_NUMBER:
            if (param1[n].value.dval != param2[n].value.dval) {
                return 0;  /* different! */
            }
            break;
        case RP_OPTIMPARAM_STRING:
            if (param1[n].value.sval.num != param2[n].value.sval.num) {
                return 0;  /* different! */
            }
            break;
        default:
            panic("bad parameter type in PgapDuplicateString()");
        }
    }
    return 1;
}

/*
 * ----------------------------------------------------------------------
 * PgapCopyString()
 *
 * Called by pgapack to copy one input string to another.
 * ----------------------------------------------------------------------
 */
MPI_Datatype
PgapBuildDT(ctx, p, pop)
    PGAContext *ctx;  /* pgapack context for this optimization */
    int p;            /* sample # being run */
    int pop;          /* population containing sample */
{
    panic("MPI support not implemented!");
    return NULL;
}

/*
 * ======================================================================
 *  OPTION:  -operation <=> PGA_MINIMIZE / PGA_MAXIMIZE
 * ======================================================================
 */
int
RpOption_ParseOper(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (strcmp(val,"minimize") == 0) {
        *ptr = PGA_MINIMIZE;
    }
    else if (strcmp(val,"maximize") == 0) {
        *ptr = PGA_MAXIMIZE;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be minimize, maximize",
            (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetOper(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_MINIMIZE:
        Tcl_SetResult(interp, "minimize", TCL_STATIC);
        break;
    case PGA_MAXIMIZE:
        Tcl_SetResult(interp, "maximize", TCL_STATIC);
        break;
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}

/*
 * ======================================================================
 *  OPTION:  -poprepl <=> PGASetPopReplacementType()
 * ======================================================================
 */
int
RpOption_ParsePopRepl(interp, valObj, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    Tcl_Obj *valObj;     /* set option to this new value */
    ClientData cdata;    /* save in this data structure */
    int offset;          /* save at this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    char *val = Tcl_GetStringFromObj(valObj, (int*)NULL);
    if (*val == 'b' && strcmp(val,"best") == 0) {
        *ptr = PGA_POPREPL_BEST;
    }
    else if (*val == 'r' && strcmp(val,"random-repl") == 0) {
        *ptr = PGA_POPREPL_RANDOM_REP;
    }
    else if (*val == 'r' && strcmp(val,"random-norepl") == 0) {
        *ptr = PGA_POPREPL_RANDOM_NOREP;
    }
    else {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad value \"", val, "\": should be best, random-norepl,"
            " or random-repl", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
RpOption_GetPopRepl(interp, cdata, offset)
    Tcl_Interp *interp;  /* interpreter handling this request */
    ClientData cdata;    /* get from this data structure */
    int offset;          /* get from this offset in cdata */
{
    int *ptr = (int*)(cdata+offset);
    switch (*ptr) {
    case PGA_POPREPL_BEST:
        Tcl_SetResult(interp, "best", TCL_STATIC);
        break;
    case PGA_POPREPL_RANDOM_REP:
        Tcl_SetResult(interp, "random-repl", TCL_STATIC);
        break;
    case PGA_POPREPL_RANDOM_NOREP:
        Tcl_SetResult(interp, "random-norepl", TCL_STATIC);
        break;
    default:
        Tcl_SetResult(interp, "???", TCL_STATIC);
        break;
    }
    return TCL_OK;
}

/*
 * ======================================================================
 *  ROUTINES FOR CONNECTING PGACONTEXT <=> RPOPTIMENV
 * ======================================================================
 * PgapLinkContext2Env()
 *   This routine is used internally to establish a relationship between
 *   a PGAContext token and its corresponding RpOptimEnv data.  The
 *   PGA routines don't provide a way to pass the RpOptimEnv data along,
 *   so we use these routines to find the correspondence.
 *
 * PgapGetEnvForContext()
 *   Returns the RpOptimEnv associated with a given PGAContext.  If the
 *   link has not been established via PgapLinkContext2Env(), then this
 *   routine returns NULL.
 *
 * PgapUnlinkContext2Env()
 *   Breaks the link between a PGAContext and its RpOptimEnv.  Should
 *   be called when the PGAContext is destroyed and is no longer valid.
 * ----------------------------------------------------------------------
 */
static Tcl_HashTable *Pgacontext2Rpenv = NULL;

void
PgapLinkContext2Env(ctx, envPtr)
    PGAContext *ctx;      /* pgapack context for this optimization */
    RpOptimEnv *envPtr;   /* corresponding Rappture optimization data */
{
    Tcl_HashEntry *ctxEntry;
    int newEntry;

    if (Pgacontext2Rpenv == NULL) {
        Pgacontext2Rpenv = (Tcl_HashTable*)malloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(Pgacontext2Rpenv, TCL_ONE_WORD_KEYS);
    }
    ctxEntry = Tcl_CreateHashEntry(Pgacontext2Rpenv, (char*)ctx, &newEntry);
    Tcl_SetHashValue(ctxEntry, (ClientData)envPtr);
}

RpOptimEnv*
PgapGetEnvForContext(ctx)
    PGAContext *ctx;
{
    Tcl_HashEntry *entryPtr;

    if (Pgacontext2Rpenv) {
        entryPtr = Tcl_FindHashEntry(Pgacontext2Rpenv, (char*)ctx);
        if (entryPtr) {
            return (RpOptimEnv*)Tcl_GetHashValue(entryPtr);
        }
    }
    return NULL;
}

void
PgapUnlinkContext2Env(ctx)
    PGAContext *ctx;
{
    Tcl_HashEntry *entryPtr;

    if (Pgacontext2Rpenv) {
        entryPtr = Tcl_FindHashEntry(Pgacontext2Rpenv, (char*)ctx);
        if (entryPtr) {
            Tcl_DeleteHashEntry(entryPtr);
        }
    }
}
