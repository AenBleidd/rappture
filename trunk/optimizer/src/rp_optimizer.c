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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "rp_optimizer.h"

/*
 * ----------------------------------------------------------------------
 * RpOptimCreate()
 *
 * Used to create the context for an optimization.  Creates an empty
 * context and returns a pointer to it.  The context can be updated
 * by calling functions like RpOptimAddParamNumber to define various
 * input parameters.
 * ----------------------------------------------------------------------
 */
RpOptimEnv*
RpOptimCreate()
{
    RpOptimEnv *envPtr;
    envPtr = (RpOptimEnv*)malloc(sizeof(RpOptimEnv));
    envPtr->numParams = 0;
    envPtr->maxParams = 2;
    envPtr->paramList = (RpOptimParam**)malloc(
        (size_t)(envPtr->maxParams*sizeof(RpOptimParam*))
    );

    return envPtr;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimAddParam()
 *
 * Used internally to add a new parameter into the given optimization
 * context.  The internal list of parameters is resized, if necessary,
 * to accommodate the new parameter.
 * ----------------------------------------------------------------------
 */
void
RpOptimAddParam(envPtr, paramPtr)
    RpOptimEnv *envPtr;      /* context for this optimization */
    RpOptimParam *paramPtr;  /* parameter being added */
{
    RpOptimParam **newParamList;

    /*
     * Add the new parameter at the end of the list.
     */
    envPtr->paramList[envPtr->numParams++] = paramPtr;

    /*
     * Out of space?  Then double the space available for params.
     */
    if (envPtr->numParams >= envPtr->maxParams) {
        envPtr->maxParams *= 2;
        newParamList = (RpOptimParam**)malloc(
            (size_t)(envPtr->maxParams*sizeof(RpOptimParam*))
        );
        memcpy(newParamList, envPtr->paramList,
            (size_t)(envPtr->numParams*sizeof(RpOptimParam*)));
        free(envPtr->paramList);
        envPtr->paramList = newParamList;
    }
}

/*
 * ----------------------------------------------------------------------
 * RpOptimAddParamNumber()
 *
 * Used to add a number parameter as an input to an optimization.
 * Each number has a name and a double precision value that can be
 * constrained between min/max values.  Adds this number to the end
 * of the parameter list in the given optimization context.
 * ----------------------------------------------------------------------
 */
void
RpOptimAddParamNumber(envPtr, name, min, max)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
    double min;           /* minimum value for this parameter */
    double max;           /* minimum value for this parameter */
{
    RpOptimParamNumber *numPtr;
    numPtr = (RpOptimParamNumber*)malloc(sizeof(RpOptimParamNumber));
    numPtr->base.name = strdup(name);
    numPtr->base.type = RP_OPTIMPARAM_NUMBER;
    numPtr->base.value.num = 0.0;
    numPtr->min = min;
    numPtr->max = max;

    RpOptimAddParam(envPtr, (RpOptimParam*)numPtr);
}

/*
 * ----------------------------------------------------------------------
 * RpOptimAddParamString()
 *
 * Used to add a string parameter as an input to an optimization.
 * Each string has a name and a list of allowed values terminated
 * by a NULL.  Adds this string to the end of the parameter list
 * in the given optimization context.
 * ----------------------------------------------------------------------
 */
void
RpOptimAddParamString(envPtr, name, allowedValues)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
    char **allowedValues; /* null-term list of allowed values */
{
    int n;
    RpOptimParam **endPtrPtr;
    RpOptimParamString *strPtr;
    strPtr = (RpOptimParamString*)malloc(sizeof(RpOptimParamString));
    strPtr->base.name = strdup(name);
    strPtr->base.type = RP_OPTIMPARAM_STRING;
    strPtr->base.value.str = NULL;

    /* count the number of allowed values */
    if (allowedValues) {
        for (n=0; allowedValues[n] != NULL; n++)
            ;
    } else {
        n = 0;
    }
    strPtr->numValues = n;
    strPtr->values = (char**)malloc(n*sizeof(char*));

    /* build a null-terminated list of copies of allowed values */
    for (n=0; n < strPtr->numValues; n++) {
        strPtr->values[n] = strdup(allowedValues[n]);
    }

    RpOptimAddParam(envPtr, (RpOptimParam*)strPtr);
}

/*
 * ----------------------------------------------------------------------
 * RpOptimPerform()
 *
 * Used to perform an optimization in the given context.  Each run is
 * performed by calling an evaluation function represented by a
 * function pointer.  If an optimum value is found within the limit
 * on the number of runs, then this procedure returns RP_OPTIM_SUCCESS.
 * Values for the optimum input parameters are returned through the
 * paramList within the context.  If the optimization fails, this
 * function returns RP_OPTIM_FAILURE.
 * ----------------------------------------------------------------------
 */
RpOptimStatus
RpOptimPerform(envPtr, evalFuncPtr, maxRuns)
    RpOptimEnv *envPtr;               /* context for this optimization */
    RpOptimEvaluator *evalFuncPtr;    /* function called to handle run */
    int maxRuns;                      /* limit on number of runs,
                                       * or 0 for no limit */
{
    RpOptimStatus status = RP_OPTIM_UNKNOWN;

    int n, nruns, ival;
    double dval, fitness;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;
    RpOptimStatus runStatus;

    if (envPtr->numParams == 0) {    /* no input parameters? */
        return RP_OPTIM_FAILURE;     /* then we can't optimize! */
    }

    /*
     * Call the evaluation function a number of times with different
     * values and perform the optimization.
     */
    nruns = 0;
    while (status == RP_OPTIM_UNKNOWN) {
        /*
         * Pick random values for all inputs.
         */
        for (n=0; n < envPtr->numParams; n++) {
            switch (envPtr->paramList[n]->type) {
                case RP_OPTIMPARAM_NUMBER:
                    numPtr = (RpOptimParamNumber*)envPtr->paramList[n];
                    dval = drand48();
                    envPtr->paramList[n]->value.num =
                        (numPtr->max - numPtr->min)*dval + numPtr->min;
                    break;
                case RP_OPTIMPARAM_STRING:
                    strPtr = (RpOptimParamString*)envPtr->paramList[n];
                    ival = (int)floor(drand48() * strPtr->numValues);
                    envPtr->paramList[n]->value.str = strPtr->values[ival];
                    break;
            }
        }

        /*
         * Call the evaluation function to get the fitness value.
         */
        runStatus = (*evalFuncPtr)(envPtr->paramList, envPtr->numParams,
            &fitness);

        if (runStatus == RP_OPTIM_SUCCESS) {
            /*
             * Is the fitness function any better?
             * Change the input values here based on fitness.
             *  ...
             */
        }

        if (++nruns >= maxRuns && maxRuns > 0) {
            status = RP_OPTIM_FAILURE;  /* reached the limit of runs */
        }
    }

    return status;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimDelete()
 *
 * Used to delete the context for an optimization once it is finished
 * or no longer needed.  Frees up the memory needed to store the
 * context and all input parameters.  After this call, the context
 * should not be used again.
 * ----------------------------------------------------------------------
 */
void
RpOptimDelete(envPtr)
    RpOptimEnv *envPtr;   /* context for this optimization */
{
    int n;
    RpOptimParam *paramPtr;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    for (n=0; n < envPtr->numParams; n++) {
        paramPtr = envPtr->paramList[n];
        free(paramPtr->name);
        switch (paramPtr->type) {
            case RP_OPTIMPARAM_NUMBER:
                numPtr = (RpOptimParamNumber*)paramPtr;
                /* nothing special to free here */
                break;
            case RP_OPTIMPARAM_STRING:
                strPtr = (RpOptimParamString*)paramPtr;
                for (n=0; n < strPtr->numValues; n++) {
                    free(strPtr->values[n]);
                }
                break;
        }
        free(paramPtr);
    }
    free(envPtr->paramList);
    free(envPtr);
}
