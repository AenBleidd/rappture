/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * PerfQuery.cpp: performance query class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <assert.h>

#include "PerfQuery.h"

PerfQuery::PerfQuery()
{
    glGenQueriesARB(1, &id);
    pixel = 0;
}

PerfQuery::~PerfQuery()
{
    glDeleteQueriesARB(1, &id);
}

//There can only be one active query at any given moment
void PerfQuery::enable()
{
    glBeginQueryARB(GL_SAMPLES_PASSED_ARB, id);
}

void PerfQuery::disable()
{
    glEndQueryARB(GL_SAMPLES_PASSED_ARB);

    GLuint count;
    glGetQueryObjectuivARB(id, GL_QUERY_RESULT_ARB, &count);

    //accumulate pixel count count
    pixel += count;
}
