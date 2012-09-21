/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Outcome Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#ifndef _RAPPTURE_OUTCOME_C_H
#define _RAPPTURE_OUTCOME_C_H

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct {
    void* _status;
} RapptureOutcome;

int RapptureOutcomeInit(RapptureOutcome* status);
int RapptureOutcomeNew(RapptureOutcome* status);
int RapptureOutcomeFree(RapptureOutcome* status);
int RapptureOutcomeError(  RapptureOutcome* outcome,
                           const char* errmsg,
                           int status  );
int RapptureOutcomeClear(RapptureOutcome* status);
int RapptureOutcomeAddContext(RapptureOutcome* status, const char* msg);
const char* RapptureOutcomeRemark(RapptureOutcome* status);
const char* RapptureOutcomeContext(RapptureOutcome* status);
int RapptureOutcomeCheck(RapptureOutcome* status);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_OUTCOME_C_H
