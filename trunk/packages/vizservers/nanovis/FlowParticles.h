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
#ifndef NV_FLOWPARTICLES_H
#define NV_FLOWPARTICLES_H

#include <cassert>
#include <string>

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "FlowTypes.h"
#include "Switch.h"
#include "ParticleRenderer.h"
#include "Volume.h"

namespace nv {

struct FlowParticlesValues {
    FlowPosition position;   ///< Position on axis of particle plane
    FlowColor color;         ///< Color of particles
    int isHidden;            ///< Is particle injection plane active
    float particleSize;      ///< Size of the particles
};

class FlowParticles
{
public:
    FlowParticles(const char *name);

    ~FlowParticles();

    const char *name() const
    {
        return _name.c_str();
    }

    bool visible() const
    {
        return !_sv.isHidden;
    }

    int parseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (nv::ParseSwitches(interp, _switches, objc, objv, &_sv, 
                              SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    void advect()
    {
        assert(_renderer->active());
        _renderer->advect();
    }

    void render();

    void reset()
    {
        _renderer->reset();
    }

    void initialize()
    {
        _renderer->initialize();
    }

    void setVectorField(Volume *volume)
    {
        _volume = volume;
        _renderer->setVectorField(volume);
    }

    float getRelativePosition(FlowPosition *position);

    bool configure();

private:
    /**
     * Name of particle injection plane. Actual character string is
     * stored in hash table.
     */
    std::string _name;
    nv::ParticleRenderer *_renderer;        ///< Particle renderer
    Volume *_volume;
    FlowParticlesValues _sv;

    static SwitchSpec _switches[];
};

}

#endif
