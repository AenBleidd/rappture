/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */
#include "RenderContext.h"

using namespace nv::graphics;

RenderContext::RenderContext() :
    _cullMode(NO_CULL),
    _fillMode(FILL),
    _shadingModel(SMOOTH)
{
}

RenderContext::~RenderContext()
{
}

