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
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#undef _ANSI_ARGS_
#undef CONST

#ifndef NO_CONST
#  define CONST const
#else
#  define CONST
#endif

#ifndef NO_PROTOTYPES
#  define _ANSI_ARGS_(x)  x
#else
#  define _ANSI_ARGS_(x)  ()
#endif

#ifdef EXTERN
#  undef EXTERN
#endif
#ifdef __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

/*
 * This is the basic definition for each parameter within an optimization.
 * Each parameter has a name, type, and value.  The value is important
 * at the end of an optimization.  It is either the final value, or the
 * value where we left off when the optimization was terminated.
 */
typedef enum {
    RP_OPTIMPARAM_NUMBER, RP_OPTIMPARAM_STRING
} RpOptimParamType;

typedef struct RpOptimParam {
    char *name;                     /* name of this parameter */
    RpOptimParamType type;          /* paramter type -- number, string, etc */
    union {
        double num;                 /* slot for number value */
        char *str;                  /* slot for string value */
    } value;
} RpOptimParam;

/*
 * NUMBER PARAMETERS have additional min/max values as constraints.
 */
typedef struct RpOptimParamNumber {
    RpOptimParam base;              /* basic parameter info */
    double min;                     /* optimization constraint: min value */
    double max;                     /* optimization constraint: max value */
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
    RpOptimParam **paramList;       /* list of input parameters to vary */
    int numParams;                  /* current number of parameters */
    int maxParams;                  /* storage for this many paramters */
} RpOptimEnv;

/*
 * During each optimization, a function of the following type is
 * called againa and again to evaluate input parameters and compute
 * the fitness function.
 */
typedef enum {
    RP_OPTIM_SUCCESS=0, RP_OPTIM_UNKNOWN, RP_OPTIM_FAILURE
} RpOptimStatus;

typedef RpOptimStatus (RpOptimEvaluator)_ANSI_ARGS_((RpOptimParam **values,
    int numValues, double *fitnessPtr));


/*
 *  Here are the functions in the API:
 */
EXTERN RpOptimEnv* RpOptimCreate _ANSI_ARGS_(());

EXTERN void RpOptimAddParamNumber _ANSI_ARGS_((RpOptimEnv *envPtr,
    char *name, double min, double max));

EXTERN void RpOptimAddParamString _ANSI_ARGS_((RpOptimEnv *envPtr,
    char *name, char **allowedValues));

EXTERN RpOptimStatus RpOptimPerform _ANSI_ARGS_((RpOptimEnv *envPtr,
    RpOptimEvaluator *evalFuncPtr, int maxRuns));

EXTERN void RpOptimDelete _ANSI_ARGS_((RpOptimEnv *envPtr));
