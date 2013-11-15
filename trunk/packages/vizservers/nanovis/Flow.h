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
#ifndef NV_FLOW_H
#define NV_FLOW_H

#include <tr1/unordered_map>
#include <vector>
#include <string>

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "Switch.h"
#include "FlowTypes.h"
#include "FlowParticles.h"
#include "FlowBox.h"
#include "LIC.h"
#include "VelocityArrowsSlice.h"
#include "Unirect.h"
#include "Volume.h"
#include "TransferFunction.h"

namespace nv {

struct FlowValues {
    TransferFunction *transferFunction;
    FlowPosition slicePos;
    int showArrows;
    int sliceVisible;
    int showVolume;
    int showOutline;
    int isHidden;
    int twoSidedLighting;
    float ambient;     ///< Ambient volume shading
    float diffuse;     ///< Diffuse volume shading
    float specular;    ///< Specular level volume shading
    float specularExp; ///< Specular exponent volume shading
    float opacity;     ///< Volume opacity scaling
};

class Flow
{
public:
    Flow(Tcl_Interp *interp, const char *name);

    ~Flow();

    void getBounds(vrmath::Vector3f& min,
                   vrmath::Vector3f& max,
                   bool onlyVisible);

    FlowParticles *createParticles(const char *particlesName);

    FlowParticles *getParticles(const char *particlesName);

    void deleteParticles(const char *particlesName);

    void getParticlesNames(std::vector<std::string>& names);

    void render();

    void advect();

    void resetParticles();

    void initializeParticles();

    FlowBox *createBox(const char *boxName);

    FlowBox *getBox(const char *boxName);

    void deleteBox(const char *boxName);

    void getBoxNames(std::vector<std::string>& names);

    bool visible()
    {
        return !_sv.isHidden;
    }

    const char *name() const
    {
        return _name.c_str();
    }

    bool isDataLoaded()
    {
        return (_volume != NULL);
    }

    void getVectorRange(double range[])
    {
        range[0] = _volume->wAxis.min();
        range[1] = _volume->wAxis.max();
    }

    void data(Unirect3d *unirect)
    {
        float *vdata = getScaledVector(unirect);
        _volume = makeVolume(unirect, vdata);
        delete [] vdata;
        delete unirect;
        initVolume();
        scaleVectorField();
    }

    void data(Volume *volume)
    {
        _volume = volume;
        initVolume();
        scaleVectorField();
    }

    FlowSliceAxis getAxis()
    {
        return _sv.slicePos.axis;
    }

    TransferFunction *getTransferFunction()
    {
        return _sv.transferFunction;
    }
#if 0
    void setAxis(FlowSliceAxis axis)
    {
        _sv.slicePos.axis = axis;
        if (NanoVis::licRenderer != NULL) {
            NanoVis::licRenderer->setSliceAxis(_sv.slicePos.axis);
        }
        if (NanoVis::velocityArrowsSlice != NULL) {
            NanoVis::velocityArrowsSlice->setSliceAxis(_sv.slicePos.axis);
        }
    }

    void setCurrentPosition(float position)
    {
        _sv.slicePos.value = position;
        if (NanoVis::licRenderer != NULL) {
            NanoVis::licRenderer->setSlicePosition(_sv.slicePos.value);
        }
        if (NanoVis::velocityArrowsSlice != NULL) {
            NanoVis::velocityArrowsSlice->setSlicePosition(_sv.slicePos.value);
        }
    }

    void setLICActive(bool state)
    {
        _sv.sliceVisible = state;
        if (NanoVis::licRenderer != NULL) {
            NanoVis::licRenderer->setVectorField(_volume);
            NanoVis::licRenderer->setSliceAxis(_sv.slicePos.axis);
            NanoVis::licRenderer->setSlicePosition(_sv.slicePos.value);
            NanoVis::licRenderer->visible(state);
        }
    }

    void setArrowsActive(bool state)
        _sv.showArrows = state;
        if (NanoVis::velocityArrowsSlice != NULL) {
            NanoVis::velocityArrowsSlice->setVectorField(_volume);
            NanoVis::velocityArrowsSlice->setSliceAxis(_sv.slicePos.axis);
            NanoVis::velocityArrowsSlice->setSlicePosition(_sv.slicePos.value);
            NanoVis::velocityArrowsSlice->visible(_sv.showArrows);
        }
    }
#endif
    const Volume *getVolume() const
    {
        return _volume;
    }

    int parseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (nv::ParseSwitches(interp, _switches, objc, objv, &_sv,
                              SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    bool configure();

    Tcl_Command getCommandToken()
    {
        return _cmdToken;
    }

    float getRelativePosition(FlowPosition *pos);

    static bool updatePending;
    static double magMin, magMax;

private:
    typedef std::string ParticlesId;
    typedef std::string BoxId;
    typedef std::tr1::unordered_map<ParticlesId, FlowParticles *> ParticlesHashmap;
    typedef std::tr1::unordered_map<BoxId, FlowBox *> BoxHashmap;

    void initVolume();

    bool scaleVectorField();

    float *getScaledVector(Unirect3d *unirect);

    Volume *makeVolume(Unirect3d *unirect, float *data);

    void renderBoxes();

    Tcl_Interp *_interp;
    /**
     * Name of the flow.  This may differ
     * from the name of the command
     * associated with the flow, if the
     * command was renamed. */
    std::string _name;

    /**
     * Command associated with the flow.
     * When the command is deleted, so is
     * the flow. */
    Tcl_Command _cmdToken;

    /**
     * The volume associated with the
     * flow.  This isn't the same thing as
     * a normal volume displayed. */
    Volume *_volume;

    /**
     * For each field there can be one or
     * more particle injection planes
     * where the particles are injected
     * into the flow. */
    ParticlesHashmap _particlesTable;

    /**
     * A table of boxes.  There maybe
     * zero or more boxes associated
     * with each field. */
    BoxHashmap _boxTable;

    FlowValues _sv;

    static SwitchSpec _switches[];
};

}

#endif
