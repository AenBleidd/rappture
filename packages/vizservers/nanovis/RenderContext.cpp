/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "RenderContext.h"

namespace graphics {

RenderContext::RenderContext()
: _cullMode(NO_CULL), _fillMode(FILL), _shadingModel(SMOOTH)
{
}

RenderContext::~RenderContext()
{
}

};

