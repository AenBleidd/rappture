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

#include <vrmath/Color4f.h>

#include "nanovis.h"        // For NMESH
#include "FlowParticles.h"
#include "Flow.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

FlowParticles::FlowParticles(const char *name) :
    _name(name),
    _renderer(new ParticleRenderer(NMESH, NMESH))
{
    _sv.position.value = 0.0f;
    _sv.position.flags = RELPOS;
    _sv.position.axis = AXIS_Z;
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
    FreeSwitches(_switches, &_sv, 0);
}

float
FlowParticles::getRelativePosition(FlowPosition *position)
{
    if (position->flags == RELPOS) {
        return position->value;
    }
    switch (position->axis) {
    case AXIS_X:  
        return (position->value - _volume->xAxis.min()) / 
            (_volume->xAxis.max() - _volume->xAxis.min()); 
    case AXIS_Y:  
        return (position->value - _volume->yAxis.min()) / 
            (_volume->yAxis.max() - _volume->yAxis.min()); 
    case AXIS_Z:  
        return (position->value - _volume->zAxis.min()) / 
            (_volume->zAxis.max() - _volume->zAxis.min()); 
    }
    return 0.0;
}

void
FlowParticles::render() 
{
    TRACE("Particles '%s' axis: %d pos: %g rel pos: %g",
          _name.c_str(), _sv.position.axis, _sv.position.value,
          getRelativePosition(&_sv.position));

    _renderer->setSlicePosition(getRelativePosition(&_sv.position));
    _renderer->setSliceAxis(_sv.position.axis);
    assert(_renderer->active());
    _renderer->render();
}

bool
FlowParticles::configure() 
{
    bool needReset = false;

    _renderer->setColor(Color4f(_sv.color.r,
                                _sv.color.g,
                                _sv.color.b, 
                                _sv.color.a));
    _renderer->particleSize(_sv.particleSize);
    if (_renderer->getSliceAxis() != _sv.position.axis) {
        needReset = true;
        _renderer->setSliceAxis(_sv.position.axis);
    }
    float pos = getRelativePosition(&_sv.position);
    if (_renderer->getSlicePosition() != pos) {
        needReset = true;
        _renderer->setSlicePosition(pos);
    }
    _renderer->active(!_sv.isHidden);

    return needReset;
}
