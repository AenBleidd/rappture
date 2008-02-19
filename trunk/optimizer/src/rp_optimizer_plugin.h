/*
 * ----------------------------------------------------------------------
 *  rp_optimizer_plugin
 *
 *  This file defines the API for various optimization packages to
 *  plug into the Rappture Optimization module.  Each plug-in defines
 *  the following:
 *
 *     initialization procedure ... malloc's a ball of data
 *     cleanup procedure .......... frees the ball of data
 *     configuration options ...... says how to configure the data
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef RP_OPTIMIZER_PLUGIN
#define RP_OPTIMIZER_PLUGIN

#include "rp_optimizer.h"
#include "rp_tcloptions.h"

/*
 * Each optimization package is plugged in to this infrastructure
 * by defining the following data at the top of rp_optimizer_tcl.c 
 */
typedef struct RpOptimPlugin {
    char *name;                    /* name of this package for -using */
    RpOptimInit *initPtr;          /* initialization routine */
    RpOptimCleanup *cleanupPtr;    /* cleanup routine */
    RpTclOption *optionSpec;       /* specs for config options */
} RpOptimPlugin;

#endif
