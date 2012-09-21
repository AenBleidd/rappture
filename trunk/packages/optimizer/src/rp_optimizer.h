/*
 * ----------------------------------------------------------------------
 *  rp_optimizer
 *
 *  This is the C language API for the optimization package in
 *  Rappture.  It lets you set up an optimization of some fitness
 *  function with respect to a set of inputs.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RP_OPTIMIZER
#define RP_OPTIMIZER

#include <tcl.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <malloc.h>
#include "rp_tcloptions.h"

#define PGAPACK_RUNTIME_TABLE_DEFAULT_SIZE 5000 /*Approx Number of Samples in a run*/
#define SINGLE_SAMPLE_DATA_BUFFER_DEFAULT_SIZE 5000

/*User defined Random Number Distributions*/
#define RAND_NUMBER_DIST_GAUSSIAN 1
#define RAND_NUMBER_DIST_UNIFORM 2

/* Used to indicate unspecified mutation rate for a param, will result in global mutn rate being applied*/
#define PARAM_NUM_UNSPEC_MUTN_RATE -1.0

/*
 * General-purpose allocation/cleanup routines:
 * These are used, for example, for the plug-in architecture.
 */
typedef ClientData (RpOptimInit)_ANSI_ARGS_(());
typedef void (RpOptimCleanup)_ANSI_ARGS_((ClientData cdata));

/*
 * During each optimization, a function of the following type is
 * called again and again to evaluate input parameters and compute
 * the fitness function.
 */
typedef enum {
    RP_OPTIM_SUCCESS=0, RP_OPTIM_UNKNOWN, RP_OPTIM_FAILURE, RP_OPTIM_ABORTED, RP_OPTIM_RESTARTED
} RpOptimStatus;

struct RpOptimEnv;   /* defined below */
struct RpOptimParam;

typedef RpOptimStatus (RpOptimEvaluator) _ANSI_ARGS_((
    struct RpOptimEnv *envPtr, struct RpOptimParam *values, int numValues,
    double *fitnessPtr));

typedef RpOptimStatus (RpOptimHandler) _ANSI_ARGS_((
    struct RpOptimEnv *envPtr, RpOptimEvaluator *evalProc,
    char *fitnessExpr));

/*
 * Each optimization package is plugged in to this infrastructure
 * by defining the following data at the top of rp_optimizer_tcl.c
 */
typedef struct RpOptimPlugin {
    char *name;                   /* name of this package for -using */
    RpOptimInit *initProc;        /* initialization routine */
    RpOptimHandler *runProc;      /* handles the core optimization */
    RpOptimCleanup *cleanupProc;  /* cleanup routine */
    RpTclOption *optionSpec;      /* specs for config options */
} RpOptimPlugin;

/*
 * This is the basic definition for each parameter within an optimization.
 * Each parameter has a name, type, and value.  The value is important
 * at the end of an optimization.  It is either the final value, or the
 * value where we left off when the optimization was terminated.
 */
typedef enum {
    RP_OPTIMPARAM_NUMBER, RP_OPTIMPARAM_STRING
} RpOptimParamType;

typedef struct RpDiscreteString {
    int num;                        /* integer representing this string */
    char *str;                      /* actual string selected */
} RpDiscreteString;

typedef struct RpOptimParam {
    char *name;                     /* name of this parameter */
    RpOptimParamType type;          /* paramter type -- number, string, etc */
    union {
        double dval;                /* slot for number value */
        int ival;                   /* slot for integer value */
        RpDiscreteString sval;      /* slot for string value */
    } value;
} RpOptimParam;

/*
 * NUMBER PARAMETERS have additional constraints.
 */
typedef struct RpOptimParamNumber {
    RpOptimParam base;              /* basic parameter info */
    double min;                     /* optimization constraint: min value */
    double max;                     /* optimization constraint: max value */
    double mutnrate;            	/* independently sets mutation rate for each parameter*/
    double mutnValue;               /* on mutation bumps up/down the value by val (+-) mutnValue*val */
    int randdist;             		/* gaussian or uniform distribution*/
    int strictmin;					/* whether a strict min is to be applied for gauss. rand. numbers*/
    int strictmax;					/* whether a strict max is to be applied for gauss. rand. numbers*/
    double stddev;					/* std deviaton for gaussian profile*/
    double mean;					/* mean for gaussian profile*/
    char *units;
} RpOptimParamNumber;

/*
 * STRING PARAMETERS have a set of allowed values.
 */
typedef struct RpOptimParamString {
    RpOptimParam base;              /* basic parameter info */
    char **values;                  /* array of allowed values */
    int numValues;                  /* number of allowed values */
} RpOptimParamString;

/*
 * Each optimization problem is represented by a context that includes
 * the parameters that will be varied.
 */
typedef struct RpOptimEnv {
    RpOptimPlugin *pluginDefn;      /* plug-in handling this optimization */
    ClientData pluginData;          /* data created by plugin init routine */
    RpOptimEvaluator *evalProc;     /* called during optimization to do run */
    char *fitnessExpr;              /* fitness function in string form */
    ClientData toolData;            /* data used during tool execution */
    RpOptimParam **paramList;       /* list of input parameters to vary */
    int numParams;                  /* current number of parameters */
    int maxParams;                  /* storage for this many paramters */
} RpOptimEnv;

/*
 *  Here are the functions in the API:
 */
EXTERN RpOptimEnv* RpOptimCreate _ANSI_ARGS_((RpOptimPlugin *pluginDefn));

EXTERN RpOptimParam* RpOptimAddParamNumber _ANSI_ARGS_((RpOptimEnv *envPtr,
    char *name));

EXTERN RpOptimParam* RpOptimAddParamString _ANSI_ARGS_((RpOptimEnv *envPtr,
    char *name));

EXTERN RpOptimParam* RpOptimFindParam _ANSI_ARGS_((RpOptimEnv *envPtr,
    char *name));

EXTERN void RpOptimDeleteParam _ANSI_ARGS_((RpOptimEnv *envPtr, char *name));

EXTERN void RpOptimDelete _ANSI_ARGS_((RpOptimEnv *envPtr));


#endif
