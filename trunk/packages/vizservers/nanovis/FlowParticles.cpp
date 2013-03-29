/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */

#include <assert.h>

#include <vrmath/Vector4f.h>

#include "nanovis.h"        // For NMESH
#include "FlowParticles.h"
#include "Flow.h"
#include "Trace.h"

using namespace vrmath;

FlowParticles::FlowParticles(const char *name) :
    _name(name),
    _renderer(new NvParticleRenderer(NMESH, NMESH))
{
    _sv.position.value = 0.0f;
    _sv.position.flags = RELPOS;
    _sv.position.axis = 0; // X_AXIS
    _sv.color.r = _sv.color.g = _sv.color.b = _sv.color.a = 1.0f;
    _sv.isHidden = false;
    _sv.particleSize = 1.2;
}

FlowParticles::~FlowParticles() 
{
    TRACE("Deleting renderer");
    if (_renderer != NULL) {
        delete _renderer;
    }
    TRACE("Freeing switches");
    Rappture::FreeSwitches(_switches, &_sv, 0);
}

void
FlowParticles::render() 
{
    TRACE("Particles '%s' axis: %d pos: %g rel pos: %g",
          _name.c_str(), _sv.position.axis, _sv.position.value,
          Flow::getRelativePosition(&_sv.position));

    _renderer->setPos(Flow::getRelativePosition(&_sv.position));
    _renderer->setAxis(_sv.position.axis);
    assert(_renderer->active());
    _renderer->render();
}

void 
FlowParticles::configure() 
{
    _renderer->setPos(Flow::getRelativePosition(&_sv.position));
    _renderer->setColor(Vector4f(_sv.color.r,
                                 _sv.color.g,
                                 _sv.color.b, 
                                 _sv.color.a));
    _renderer->particleSize(_sv.particleSize);
    _renderer->setAxis(_sv.position.axis);
    _renderer->active(!_sv.isHidden);
}
