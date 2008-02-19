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
#include "rp_optimizer_plugin.h"

typedef struct PgapackData {
    int foo;                        /* data used by Pgapack... */
} PgapackData;

RpTclOption PgapackOptions[] = {
  {"-foo", RP_OPTION_INT, NULL, Rp_Offset(PgapackData,foo)},
  {NULL, NULL, NULL, 0}
};

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
    dataPtr->foo = 1;
    return (ClientData)dataPtr;
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
    dataPtr->foo = 0;
    free(dataPtr);
}
