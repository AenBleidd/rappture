/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Event.cpp: user event class for statistics purpose
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <memory.h>

#include "Event.h"

Event::Event()
{
}

Event::~Event()
{
}

Event::Event(int _type, float _param[3], float _time) :
    type(_type),
    msec(_time)
{
    memcpy(parameter, _param, 3*sizeof(float));
}

void Event::write(FILE *fd)
{
    fprintf(fd, "%d %f %f %f %g\n", type, parameter[0], parameter[1], parameter[2], msec);
}


