/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Renderable.cpp: abstract class, a drawable thing 
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "Renderable.h"

Renderable::Renderable()
{
}

Renderable::Renderable(const Vector3& loc) :
    _location(loc),
    _enabled(true)
{ 
}

Renderable::~Renderable()
{}

void Renderable::move(const Vector3& new_loc)
{
    _location = new_loc;
}

void Renderable::enable()
{
    _enabled = true;
}

void Renderable::disable()
{
    _enabled = false;
}

bool Renderable::isEnabled() const
{
    return _enabled;
}

