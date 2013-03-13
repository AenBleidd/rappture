/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Insoo Woo <iwoo@purdue.edu>
 */

#include <vrmath/Color4f.h>

using namespace vrmath;

Color4f::Color4f() :
    r(0.0f), g(0.0f), b(0.0f), a(1.0f)
{
}

Color4f::Color4f(float r1, float g1, float b1, float a1) :
    r(r1), g(g1), b(b1), a(a1)
{
}
