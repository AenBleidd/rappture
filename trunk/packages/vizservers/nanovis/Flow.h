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
    enum SliceAxis { AXIS_X, AXIS_Y, AXIS_Z };

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

    float *getScaledVector();

    Volume *makeVolume(float *data);

    bool scaleVectorField();

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
        return (_data != NULL);
    }

    Rappture::Unirect3d *data()
    {
        return _data;
    }

    void data(Rappture::Unirect3d *data)
    {
        if (_data != NULL) {
            delete _data;
        }
        _data = data;
    }
#if 0
    void activateSlice()
    {
        /* Must set axis before offset or position goes to wrong axis. */
        NanoVis::licRenderer->setAxis(_sv.slicePos.axis);
        NanoVis::licRenderer->setOffset(_sv.slicePos.value);
        NanoVis::licRenderer->active(true);
    }

    void deactivateSlice()
    {
        NanoVis::licRenderer->active(false);
    }
#endif
    SliceAxis getAxis()
    {
        return (SliceAxis)_sv.slicePos.axis;
    }

    TransferFunction *getTransferFunction()
    {
        return _sv.transferFunction;
    }

    float getRelativePosition();

    void setAxis()
    {
        NanoVis::licRenderer->setAxis(_sv.slicePos.axis);
    }

    void setAxis(Flow::SliceAxis axis)
    {
        _sv.slicePos.axis = axis;
        NanoVis::licRenderer->setAxis(_sv.slicePos.axis);
    }

    void setCurrentPosition(float position)
    {
        _sv.slicePos.value = position;
        NanoVis::licRenderer->setOffset(_sv.slicePos.value);
    }

    void setCurrentPosition()
    {
        NanoVis::licRenderer->setOffset(_sv.slicePos.value);
    }

    void setActive(bool state)
    {
        _sv.sliceVisible = state;
        NanoVis::licRenderer->active(state);
    }

    void setActive()
    {
        NanoVis::licRenderer->active(_sv.sliceVisible);
    }

    const Volume *getVolume() const
    {
        return _volume;
    }

    int parseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv,
                                    SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    Tcl_Command getCommandToken()
    {
        return _cmdToken;
    }

    static float getRelativePosition(FlowPosition *pos);

private:
    typedef std::string ParticlesId;
    typedef std::string BoxId;
    typedef std::tr1::unordered_map<ParticlesId, FlowParticles *> ParticlesHashmap;
    typedef std::tr1::unordered_map<BoxId, FlowBox *> BoxHashmap;

    void configure();

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
     * Uniform rectangular data
     * representing the mesh and vector
     * field values.  These values are
     * kept to regenerate the volume
     * associated with the flow. */
    Rappture::Unirect3d *_data;

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

    static Rappture::SwitchSpec _switches[];
};

}

#endif
