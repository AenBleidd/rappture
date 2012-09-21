/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Outcome Interface Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpOutcome.h"
#include "RpOutcomeCInterface.h"
#include "RpOutcomeCHelper.h"


#ifdef __cplusplus
extern "C" {
#endif

int
RapptureOutcomeInit(RapptureOutcome* status)
{
    if (status == NULL) {
        return -1;
    }

    status->_status = NULL;

    return 0;
}

int
RapptureOutcomeNew(RapptureOutcome* status)
{
    if (status == NULL) {
        return -1;
    }

    RapptureOutcomeFree(status);
    status->_status = (void*) new Rappture::Outcome();

    return 0;
}

int
RapptureOutcomeFree(RapptureOutcome* status)
{
    if (status->_status != NULL) {
        delete ((Rappture::Outcome*)status->_status);
        status->_status = NULL;
    }
    return 0;
}

int
RpOutcomeToCOutcome(Rappture::Outcome* rpoutcome, RapptureOutcome* status)
{
    if (rpoutcome == NULL) {
        return -1;
    }

    if (status == NULL) {
        return -1;
    }

    RapptureOutcomeFree(status);
    status->_status = new Rappture::Outcome(*rpoutcome);
    return 0;
}

int
RapptureOutcomeError(  RapptureOutcome* outcome,
                       const char* errmsg,
                       int status  )
{
    if (outcome == NULL) {
        return -1;
    }

    if (outcome->_status == NULL) {
        return -1;
    }

    ((Rappture::Outcome*)outcome->_status)->error(errmsg,status);
    return 0;
}

int
RapptureOutcomeClear(RapptureOutcome* status)
{
    if (status == NULL) {
        return -1;
    }

    if (status->_status == NULL) {
        return -1;
    }

    ((Rappture::Outcome*)status->_status)->clear();
    return 0;
}

int
RapptureOutcomeAddContext(RapptureOutcome* status, const char* msg)
{
    if (status == NULL) {
        return -1;
    }

    if (status->_status == NULL) {
        return -1;
    }

    ((Rappture::Outcome*)status->_status)->addContext(msg);
    return 0;
}

const char*
RapptureOutcomeRemark(RapptureOutcome* status)
{
    return NULL;
}

const char*
RapptureOutcomeContext(RapptureOutcome* status)
{
    return NULL;
}

int
RapptureOutcomeCheck(RapptureOutcome* status)
{
  return (Rappture::Outcome*)status->_status != NULL;
}

#ifdef __cplusplus
}
#endif // ifdef __cplusplus
