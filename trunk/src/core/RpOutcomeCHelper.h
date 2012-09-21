/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Rappture Outcome Helper Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RAPPTURE_OUTCOME_C_HELPER_H
#define _RAPPTURE_OUTCOME_C_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

int RpOutcomeToCOutcome(Rappture::Outcome* rpoutcome, RapptureOutcome* status);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_OUTCOME_C_HELPER_H
