/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * FlowCmd.h
 *
 *      This modules creates the Tcl interface to the nanovis server.  The
 *      communication protocol of the server is the Tcl language.  Commands
 *      given to the server by clients are executed in a safe interpreter and
 *      the resulting image rendered offscreen is returned as BMP-formatted
 *      image data.
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *	     Insoo Woo <iwoo@purdue.edu>
 *           Michael McLennan <mmclennan@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef FLOWCMD_H
#define FLOWCMD_H

#include <tr1/unordered_map>
#include <vector>
#include <string>

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "Switch.h"
#include "FlowTypes.h"
#include "FlowParticles.h"
#include "FlowBox.h"
#include "NvLIC.h"
#include "NvParticleRenderer.h"
#include "NvVectorField.h"
#include "Unirect.h"
#include "Volume.h"
#include "TransferFunction.h"

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

class FlowCmd
{
public:
    enum SliceAxis { AXIS_X, AXIS_Y, AXIS_Z };

    FlowCmd(Tcl_Interp *interp, const char *name);

    ~FlowCmd();

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
    
    void initVectorField();

    NvVectorField *getVectorField()
    {
        return _field;
    }

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

    void activateSlice()
    {
        /* Must set axis before offset or position goes to wrong axis. */
        NanoVis::licRenderer->setAxis(_sv.slicePos.axis);
        NanoVis::licRenderer->setOffset(_sv.slicePos.value);
        NanoVis::licRenderer->active(true);
    }

    void ceactivateSlice()
    {
        NanoVis::licRenderer->active(false);
    }

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

    void setAxis(FlowCmd::SliceAxis axis)
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

    void setVectorField(NvVectorField *field)
    {
        deleteVectorField();
        _field = field;
    }

    void deleteVectorField()
    {
        if (_field != NULL) {
            delete _field;
            _field = NULL;
        }
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

    static Rappture::SwitchSpec videoSwitches[];

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
     * Vector field generated from the 
     * above volume */
    NvVectorField *_field;

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

extern int GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes);

extern int GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                             bool *boolVal);

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           float *floatVal);

extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          int *axisVal);

extern int GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                            Volume **volume);

#endif
