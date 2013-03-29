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
#ifndef FLOWPARTICLES_H
#define FLOWPARTICLES_H

#include <cassert>
#include <string>

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "FlowTypes.h"
#include "Switch.h"
#include "NvParticleRenderer.h"
#include "Volume.h"

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
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv, 
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

    void setVectorField(Volume *volume, const vrmath::Vector3f& location,
                        float scaleX, float scaleY, float scaleZ,
                        float max)
    {
        _renderer->
            setVectorField(volume->textureID(),
                           location,
                           scaleX,
                           scaleY,
                           scaleZ,
                           max);
    }

    void configure();

private:
    /**
     * Name of particle injection plane. Actual character string is
     * stored in hash table.
     */
    std::string _name;
    NvParticleRenderer *_renderer;        ///< Particle renderer
    FlowParticlesValues _sv;

    static Rappture::SwitchSpec _switches[];
};

#endif
