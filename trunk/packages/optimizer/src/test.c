/*
 * ----------------------------------------------------------------------
 *  TEST PROGRAM for rp_optimizer
 *
 *  This is a simple test program used to exercise the rp_optimizer
 *  library in Rappture.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>
#include "rp_optimizer.h"

RpOptimStatus
GetValue(values, numValues, fitnessPtr)
    RpOptimParam **values;         /* array of input values */
    int numValues;                 /* number of values on the list */
    double *fitnessPtr;            /* returns: value of fitness */
{
    double a = 0.0;
    double b = 0.0;
    char* func = "unknown";

    int n;
    double fval;

    for (n=0; n < numValues; n++) {
        if (strcmp(values[n]->name,"a") == 0) {
            a = values[n]->value.num;
        }
        else if (strcmp(values[n]->name,"b") == 0) {
            b = values[n]->value.num;
        }
        else if (strcmp(values[n]->name,"func") == 0) {
            func = values[n]->value.str;
        }
        else {
            printf("ERROR: unrecognized parameter \"%s\"\n", values[n]->name);
            return RP_OPTIM_FAILURE;
        }
    }

    if (strcmp(func,"f1") == 0) {
        fval = (a == 0.0) ? 1.0 : b*sin(a)/a;
    }
    else if (strcmp(func,"f2") == 0) {
        fval = (b-a == 0.0) ? 1.0 : sin(b-a)/(b-a);
    }
    else if (strcmp(func,"another") == 0) {
        fval = -a*a + b;
    }
    else {
        printf("ERROR: unrecognized func value \"%s\"\n", func);
        return RP_OPTIM_FAILURE;
    }
    printf("a=%g, b=%g func=%s  =>  f=%g\n", a, b, func, fval);

    *fitnessPtr = fval;
    return RP_OPTIM_SUCCESS;
}

int
main(int argc, char** argv)
{
    RpOptimEnv *envPtr;
    RpOptimStatus status;
    char *funcValues[] = { "f1", "f2", "another", NULL };

    envPtr = RpOptimCreate();
    RpOptimAddParamNumber(envPtr, "a", 0.0, 1.0);
    RpOptimAddParamNumber(envPtr, "b", 1.0, 10.0);
    RpOptimAddParamString(envPtr, "func", funcValues);

    status = RpOptimPerform(envPtr, GetValue, 10);

    if (status == RP_OPTIM_SUCCESS) {
        printf ("success!\n");
    } else {
        printf ("failed :(\n");
    }

    RpOptimDelete(envPtr);
    return 0;
}
