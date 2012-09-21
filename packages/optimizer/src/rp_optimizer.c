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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "rp_optimizer.h"

static void RpOptimCleanupParam _ANSI_ARGS_((RpOptimParam *paramPtr));

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
RpOptimCreate(pluginDefn)
    RpOptimPlugin *pluginDefn;    /* plug-in handling this optimization */
{
    RpOptimEnv *envPtr;
    envPtr = (RpOptimEnv*)malloc(sizeof(RpOptimEnv));

    envPtr->pluginDefn = pluginDefn;
    envPtr->pluginData = NULL;
    envPtr->toolData   = NULL;

    if (pluginDefn->initProc) {
        envPtr->pluginData = (*pluginDefn->initProc)();
    }

    envPtr->numParams = 0;
    envPtr->maxParams = 5;
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
RpOptimParam*
RpOptimAddParamNumber(envPtr, name)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
{
    RpOptimParamNumber *numPtr;
    numPtr = (RpOptimParamNumber*)malloc(sizeof(RpOptimParamNumber));
    numPtr->base.name = strdup(name);
    numPtr->base.type = RP_OPTIMPARAM_NUMBER;
    numPtr->base.value.dval = 0.0;
    numPtr->min = -DBL_MAX;
    numPtr->max = DBL_MAX;
    numPtr->mutnrate = PARAM_NUM_UNSPEC_MUTN_RATE; /*Unspecified by default, should result in global mutn rate being applied*/
    numPtr->mutnValue = 0.01; /* value of this number will be changed by plus/minus hundredth of its current value*/
    numPtr->randdist = RAND_NUMBER_DIST_GAUSSIAN ;/*Gaussian by default*/
    numPtr->stddev = 1; 
    numPtr->mean = 0;
    numPtr->strictmax = 0; /*no strictmax truncation by default*/
    numPtr->strictmin = 0; /*no strict minimum truncation by default*/
	numPtr->units = ""; /* Blank Units by default*/
    RpOptimAddParam(envPtr, (RpOptimParam*)numPtr);

    return (RpOptimParam*)numPtr;
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
RpOptimParam*
RpOptimAddParamString(envPtr, name)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
{
    RpOptimParamString *strPtr;
    strPtr = (RpOptimParamString*)malloc(sizeof(RpOptimParamString));
    strPtr->base.name = strdup(name);
    strPtr->base.type = RP_OPTIMPARAM_STRING;
    strPtr->base.value.sval.num = -1;
    strPtr->base.value.sval.str = NULL;
    strPtr->values = NULL;
    strPtr->numValues = 0;

    RpOptimAddParam(envPtr, (RpOptimParam*)strPtr);

    return (RpOptimParam*)strPtr;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimFindParam()
 *
 * Used to look for an existing parameter with the specified name.
 * Returns a pointer to the parameter, or NULL if not found.
 * ----------------------------------------------------------------------
 */
RpOptimParam*
RpOptimFindParam(envPtr, name)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
{
    int n;
    for (n=0; envPtr->numParams; n++) {
        if (strcmp(name, envPtr->paramList[n]->name) == 0) {
            return envPtr->paramList[n];
        }
    }
    return NULL;
}

/*
 * ----------------------------------------------------------------------
 * RpOptimDeleteParam()
 *
 * Used to delete a parameter from the environment.  This is especially
 * useful when an error is found while configuring the parameter, just
 * after it was created.  If the name is not recognized, this function
 * does nothing.
 * ----------------------------------------------------------------------
 */
void
RpOptimDeleteParam(envPtr, name)
    RpOptimEnv *envPtr;   /* context for this optimization */
    char *name;           /* name of this parameter */
{
    int n, j;
    for (n=0; envPtr->numParams; n++) {
        if (strcmp(name, envPtr->paramList[n]->name) == 0) {
            RpOptimCleanupParam(envPtr->paramList[n]);
            for (j=n+1; j < envPtr->numParams; j++) {
                envPtr->paramList[j-1] = envPtr->paramList[j];
            }
            envPtr->numParams--;
            break;
        }
    }
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

    if (envPtr->pluginDefn && envPtr->pluginDefn->cleanupProc) {
        (*envPtr->pluginDefn->cleanupProc)(envPtr->pluginData);
    }
    for (n=0; n < envPtr->numParams; n++) {
        RpOptimCleanupParam(envPtr->paramList[n]);
    }
    free(envPtr->paramList);
    free(envPtr);
}

/*
 * ----------------------------------------------------------------------
 * RpOptimCleanupParam()
 *
 * Used internally to free up the data associated with an optimization
 * parameter.  Looks at the parameter type and frees up the appropriate
 * data.
 * ----------------------------------------------------------------------
 */
static void
RpOptimCleanupParam(paramPtr)
    RpOptimParam *paramPtr;  /* data to be freed */
{
    int n;
    RpOptimParamNumber *numPtr;
    RpOptimParamString *strPtr;

    free(paramPtr->name);
    switch (paramPtr->type) {
        case RP_OPTIMPARAM_NUMBER:
            numPtr = (RpOptimParamNumber*)paramPtr;
            /* nothing special to free here */
            break;
        case RP_OPTIMPARAM_STRING:
            strPtr = (RpOptimParamString*)paramPtr;
            if (strPtr->values) {
                for (n=0; strPtr->values[n]; n++) {
                    free(strPtr->values[n]);
                }
            }
            break;
    }
    free(paramPtr);
}
