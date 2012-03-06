/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Renderable.cpp: abstract class, a drawable thing 
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


#include "Renderable.h"

Renderable::Renderable(){}
Renderable::Renderable(Vector3 loc):
    location(loc),
    enabled(true)
{ 
}

Renderable::~Renderable(){}
  
void Renderable::move(Vector3 new_loc) { location = new_loc; }

void Renderable::enable() { enabled = true; }
void Renderable::disable() { enabled = false; }
bool Renderable::is_enabled() { return enabled; }

