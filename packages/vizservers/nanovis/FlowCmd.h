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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef FLOWCMD_H
#define FLOWCMD_H

#include <tcl.h>

#include "Switch.h"
#include "NvLIC.h"
#include "NvParticleRenderer.h"
#include "NvVectorField.h"
#include "Unirect.h"
#include "Volume.h"

struct FlowColor {
    float r, g, b, a;
};

struct FlowPosition {
    float value;
    unsigned int flags;
    int axis;
};

struct FlowPoint {
    float x, y, z;
};

struct FlowParticlesValues {
    FlowPosition position;	///< Position on axis of particle plane
    FlowColor color;		///< Color of particles
    /// Indicates if particle injection plane is active or not
    int isHidden;
    float particleSize;		///< Size of the particles
};

struct FlowParticlesIterator {
    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch hashSearch;
};

class FlowParticles
{
public:
    FlowParticles(const char *name, Tcl_HashEntry *hPtr);

    ~FlowParticles();

    const char *name()
    {
        return _name;
    }

    void disconnect()
    {
        _hashPtr = NULL;
    }

    bool visible()
    {
        return !_sv.isHidden;
    }

    int ParseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv, 
                                    SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    void Advect()
    {
        assert(_rendererPtr->active());
        _rendererPtr->advect();
    }

    void Render();

    void Reset()
    {
        _rendererPtr->reset();
    }

    void Initialize()
    {
        _rendererPtr->initialize();
    }

    void SetVectorField(Volume *volPtr)
    {
        _rendererPtr->
            setVectorField(volPtr->id, 
                           volPtr->location(),
                           1.0f,
                           volPtr->height / (float)volPtr->width,
                           volPtr->depth  / (float)volPtr->width,
                           volPtr->wAxis.max());
    }

    void Configure();

private:
    /**
     * Name of particle injection plane. Actual character string is
     * stored in hash table.
     */
    const char *_name;
    Tcl_HashEntry *_hashPtr;
    NvParticleRenderer *_rendererPtr;	///< Particle renderer
    FlowParticlesValues _sv;

    static Rappture::SwitchSpec _switches[];
};

struct FlowBoxIterator {
    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch hashSearch;
};

struct FlowBoxValues {
    float position;		///< Position on axis of particle plane
    FlowPoint corner1, corner2;	///< Coordinates of the box.
    FlowColor color;		///< Color of particles
    float lineWidth;
    /// Indicates if particle injection plane is active or not
    int isHidden;
};

class FlowBox
{
public:
    FlowBox(const char *name, Tcl_HashEntry *hPtr);

    ~FlowBox()
    {
        Rappture::FreeSwitches(_switches, &_sv, 0);
        if (_hashPtr != NULL) {
            Tcl_DeleteHashEntry(_hashPtr);
        }
    }

    const char *name()
    {
        return _name;
    }

    bool visible()
    {
        return !_sv.isHidden;
    }

    void disconnect()
    {
        _hashPtr = NULL;
    }

    int ParseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv, 
                                    SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    void Render(Volume *volPtr);

private:
    const char *_name;			/* Name of this box in the hash
					 * table. */
    Tcl_HashEntry *_hashPtr;		/* Pointer to this entry in the hash
					 * table of boxes. */
    FlowBoxValues _sv;
    static Rappture::SwitchSpec _switches[];

};

struct FlowValues {
    TransferFunction *tfPtr;
    FlowPosition slicePos;
    int showArrows;
    int sliceVisible;
    int showVolume;
    int showOutline;
    int isHidden;
    /* The following are settings for the volume.*/
    float diffuse;
    float specular;
    float opacity;
};

struct FlowIterator {
    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch hashSearch;
};

class FlowCmd
{
public:
    enum SliceAxis { AXIS_X, AXIS_Y, AXIS_Z };

    FlowCmd(Tcl_Interp *interp, const char *name, Tcl_HashEntry *hPtr);

    ~FlowCmd();

    int CreateParticles(Tcl_Interp *interp, Tcl_Obj *objPtr);

    int GetParticles(Tcl_Interp *interp, Tcl_Obj *objPtr,
		     FlowParticles **particlePtrPtr);

    void Render();

    void Advect();

    void ResetParticles();

    void InitializeParticles();

    FlowParticles *FirstParticles(FlowParticlesIterator *iterPtr);

    FlowParticles *NextParticles(FlowParticlesIterator *iterPtr);

    int CreateBox(Tcl_Interp *interp, Tcl_Obj *objPtr);

    int GetBox(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowBox **boxPtrPtr);

    FlowBox *FirstBox(FlowBoxIterator *iterPtr);

    FlowBox *NextBox(FlowBoxIterator *iterPtr);

    float *GetScaledVector();

    Volume *MakeVolume(float *data);
    
    void InitVectorField();

    NvVectorField *VectorField()
    {
        return _fieldPtr;
    }

    bool ScaleVectorField();

    bool visible()
    {
        return !_sv.isHidden;
    }

    const char *name()
    {
        return _name;
    }

    void disconnect()
    {
        _hashPtr = NULL;
    }

    bool isDataLoaded()
    {
        return (_dataPtr != NULL);
    }

    Rappture::Unirect3d *data()
    {
        return _dataPtr;
    }

    void data(Rappture::Unirect3d *dataPtr)
    {
        if (_dataPtr != NULL) {
            delete _dataPtr;
        }
        _dataPtr = dataPtr;
    }

    void ActivateSlice()
    {
        /* Must set axis before offset or position goes to wrong axis. */
        NanoVis::licRenderer->set_axis(_sv.slicePos.axis);
        NanoVis::licRenderer->set_offset(_sv.slicePos.value);
        NanoVis::licRenderer->active(true);
    }

    void DeactivateSlice()
    {
        NanoVis::licRenderer->active(false);
    }

    SliceAxis GetAxis()
    {
        return (SliceAxis)_sv.slicePos.axis;
    }

    TransferFunction *GetTransferFunction()
    {
        return _sv.tfPtr;
    }

    float GetRelativePosition();

    void SetAxis()
    {
        NanoVis::licRenderer->set_axis(_sv.slicePos.axis);
    }

    void SetAxis(FlowCmd::SliceAxis axis)
    {
        _sv.slicePos.axis = axis;
        NanoVis::licRenderer->set_axis(_sv.slicePos.axis);
    }

    void SetCurrentPosition(float position)
    {
        _sv.slicePos.value = position;
        NanoVis::licRenderer->set_offset(_sv.slicePos.value);
    }

    void SetCurrentPosition()
    {
        NanoVis::licRenderer->set_offset(_sv.slicePos.value);
    }

    void SetActive(bool state)
    {
        _sv.sliceVisible = state;
        NanoVis::licRenderer->active(state);
    }

    void SetActive()
    {
        NanoVis::licRenderer->active(_sv.sliceVisible);
    }

    void SetVectorField(NvVectorField *fieldPtr)
    {
        DeleteVectorField();
        _fieldPtr = fieldPtr;
    }

    void DeleteVectorField()
    {
        if (_fieldPtr != NULL) {
            delete _fieldPtr;
            _fieldPtr = NULL;
        }
    }

    int ParseSwitches(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
    {
        if (Rappture::ParseSwitches(interp, _switches, objc, objv, &_sv,
                                    SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    static float GetRelativePosition(FlowPosition *posPtr);

    static Rappture::SwitchSpec videoSwitches[];

private:
    void Configure();

    void RenderBoxes();

    Tcl_Interp *_interp;
    Tcl_HashEntry *_hashPtr;
    /**
     * Name of the flow.  This may differ
     * from the name of the command
     * associated with the flow, if the
     * command was renamed. */
    const char *_name;

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
    Rappture::Unirect3d *_dataPtr;

    /**
     * The volume associated with the
     * flow.  This isn't the same thing as
     * a normal volume displayed. */
    Volume *_volPtr;

    /**
     * Vector field generated from the 
     * above volume */
    NvVectorField *_fieldPtr;

    /**
     * For each field there can be one or
     * more particle injection planes
     * where the particles are injected
     * into the flow. */
    Tcl_HashTable _particlesTable;

    /**
     * A table of boxes.  There maybe
     * zero or more boxes associated
     * with each field. */
    Tcl_HashTable _boxTable;

    FlowValues _sv;

    static Rappture::SwitchSpec _switches[];
};

extern int GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes);

extern int GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                             bool *boolPtr);

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           float *floatPtr);

extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                          int *axisPtr);

extern int GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                            Volume **volumePtrPtr);

#endif
