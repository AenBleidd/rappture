
/*
 * ----------------------------------------------------------------------
 * Command.cpp
 *
 *      This modules creates the Tcl interface to the nanovis server.  The
 *      communication protocol of the server is the Tcl language.  Commands
 *      given to the server by clients are executed in a safe interpreter and
 *      the resulting image rendered offscreen is returned as BMP-formatted
 *      image data.
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Michael McLennan <mmclennan@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * TODO:  In no particular order...
 *        x Convert to Tcl_CmdObj interface. (done)
 *        o Use Tcl command option parser to reduce size of procedures, remove
 *          lots of extra error checking code. (almost there)
 *        o Convert GetVolumeIndices to GetVolumes.  Goal is to remove
 *          all references of Nanovis::volume[] from this file.  Don't
 *          want to know how volumes are stored. Same for heightmaps.
 *        o Rationalize volume id scheme. Right now it's the index in
 *          the vector. 1) Use a list instead of a vector. 2) carry
 *          an id field that's a number that gets incremented each new volume.
 *        x Create R2, matrix, etc. libraries. (done)
 *        o Add bookkeeping for volumes, heightmaps, flows, etc. to track
 *          1) id #  2) simulation # 3) include/exclude.  The include/exclude
 *          is to indicate whether the item should contribute to the overall
 *          limits of the axes.
 */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
/*#include <tcl.h>*/
#include "Vector4.h"
#include "Switch.h"
#include "Unirect.h"
#include "FlowCmd.h"

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpEncode.h>
#include <RpOutcome.h>
#include <RpBuffer.h>
#include <RpAVTranslate.h>

#include "Trace.h"
#include "Command.h"
#include "nanovis.h"
#include "CmdProc.h"
#include "Nv.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "ZincBlendeVolume.h"
#include "NvLoadFile.h"
#include "NvColorTableRenderer.h"
#include "NvEventLog.h"
#include "NvZincBlendeReconstructor.h"
#include "VolumeInterpolator.h"
#include "HeightMap.h"
#include "Grid.h"
#include "NvCamera.h"
#include "RenderContext.h"
#include "NvLIC.h"

static Tcl_HashTable flowTable;

extern bool load_vector_stream2(Rappture::Outcome &result, int index, 
	size_t length, Rappture::Unirect3d *dataPtr);

/*
    physicalMin.set(dataPtr->xMin(), dataPtr->yMin(), dataPtr->zMin());
    physicalMax.set(dataPtr->xMax(), dataPtr->yMax(), dataPtr->zMax());
*/
static float *
MakeFlowVector(Rappture::Unirect3d *dataPtr, double minMag, double maxMag)
{
    float *data = new float[4* dataPtr->nValues()];
    if (data == NULL) {
	return NULLL;
    }
    memset(data, 0, sizeof(float) * 4 * dataPtr->nValues());
    float *destPtr = data;
    float *srcPtr = dataPtr->values();
    for (size_t iz=0; iz < dataPtr->zNum(); iz++) {
	for (size_t iy=0; iy < dataPtr->yNum(); iy++) {
	    for (size_t ix=0; ix < dataPtr->xNum(); ix++) {
		double vx, vy, vz, vm;
		vx = values[0];
		vy = values[1];
		vz = values[2];
		vm = sqrt(vx*vx + vy*vy + vz*vz);
		destPtr[0] = vm / maxMag; 
		destPtr[1] = vx /(2.0*maxMag) + 0.5; 
		destPtr[2] = vy /(2.0*maxMag) + 0.5; 
		destPtr[3] = vz /(2.0*maxMag) + 0.5; 
		srcPtr += 3;
		destPtr += 4;
	    }
	}
    }
    return data;
}

static Volume *
MakeFlowVolume(Rappture::Unirect3d *dataPtr, double minMag, double maxMag, 
	       float *data)
{
    Volume *volPtr;

    ivol = NanoVis::n_volumes;
    volPtr = NanoVis::load_volume(ivol, dataPtr->xNum(), dataPtr->yNum(), 
	dataPtr->zNum(), 4, data, minMag, maxMag, 0);
    volPtr->xAxis.SetRange(dataPtr->xMin(), dataPtr->xMax());
    volPtr->yAxis.SetRange(dataPtr->yMin(), dataPtr->yMax());
    volPtr->zAxis.SetRange(dataPtr->zMin(), dataPtr->zMax());
    volPtr->wAxis.SetRange(minMag, maxMag);
    volPtr->update_pending = true;
    volPtr->setPhysicalBBox(physicalMin, physicalMax);
    //
    // BE CAREFUL:  Set the number of slices to something
    //   slightly different for each volume.  If we have
    //   identical volumes at exactly the same position
    //   with exactly the same number of slices, the second
    //   volume will overwrite the first, so the first won't
    //   appear at all.
    //
    volPtr->set_n_slice(256-n);
    // volPtr->set_n_slice(512-n);
    volPtr->disable_cutplane(0);
    volPtr->disable_cutplane(1);
    volPtr->disable_cutplane(2);
    
    NanoVis::vol_renderer->add_volume(volPtr, 
	NanoVis::get_transfunc("default"));
    
    float dx0 = -0.5;
    float dy0 = -0.5*volPtr->height/volPtr->width;
    float dz0 = -0.5*volPtr->depth/volPtr->width;
    volPtr->move(Vector3(dx0, dy0, dz0));
    return volPtr;
}


static int
FlowDataFileOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

    const char *fileName;
    fileName = Tcl_GetString(objv[3]);
    Trace("Flow loading data from file %s\n", fileName);

    int extents;
    if (Tcl_GetIntFromObj(interp, objv[4], &extents) != TCL_OK) {
        return TCL_ERROR;
    }
    Rappture::Buffer buf;
    if (!buf.load(result, fileName)) {
	Tcl_AppendResult(interp, "can't load data from \"", fileName, "\": ",
			 result.remark(), (char *)NULL);
	return TCL_ERROR;
    }

    FlowCmd *flowPtr = (FlowCmd *)clientData;
    if (strncmp(buf.bytes(), "<DX>", 4) == 0) {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d();
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->dataPtr = dataPtr;
    } else if (strncmp(buf.bytes(), "<unirect3d>", 4) == 0) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	dataPtr = new Rappture::Unirect3d();
	dataPtr->extents(extents);
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect3d", &cmdInfo);
	if (Tcl_Eval(interp, (const char *)buf.bytes()) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->dataPtr = dataPtr;
    }

#ifdef notdef
    flowPtr->volPtr = NanoVis::volume[NanoVis::n_volumes];
    //
    // BE CAREFUL:  Set the number of slices to something
    //   slightly different for each volume.  If we have
    //   identical volumes at exactly the same position
    //   with exactly the same number of slices, the second
    //   volume will overwrite the first, so the first won't
    //   appear at all.
    //
    flowPtr->volPtr->set_n_slice(256-n);
    // volPtr->set_n_slice(512-n);
    flowPtr->volPtr->disable_cutplane(0);
    flowPtr->volPtr->disable_cutplane(1);
    flowPtr->volPtr->disable_cutplane(2);
    
    NanoVis::vol_renderer->add_volume(flowPtr->volPtr, 
	NanoVis::get_transfunc("default"));
    
    Trace("Flow Data move\n");
    float dx0 = -0.5;
    float dy0 = -0.5*flowPtr->volPtr->height/flowPtr->volPtr->width;
    float dz0 = -0.5*flowPtr->volPtr->depth/flowPtr->volPtr->width;
    flowPtr->volPtr->move(Vector3(dx0, dy0, dz0));
#endif
    return TCL_OK;
}

static int
FlowDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

    Trace("Flow Data Loading\n");

    int nbytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    int extents;
    if (Tcl_GetIntFromObj(interp, objv[4], &extents) != TCL_OK) {
        return TCL_ERROR;
    }
    Rappture::Buffer buf;
    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    if (strncmp(buf.bytes(), "<DX>", 4) == 0) {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d();
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->dataPtr = dataPtr;
    } else if (strncmp(buf.bytes(), "<unirect3d>", 4) == 0) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect3d();
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	dataPtr->extents(extents);
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect3d", &cmdInfo);
	if (Tcl_Eval(interp, (const char *)buf.bytes()) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->dataPtr = dataPtr;
    }
#ifdef notdef
    // FIXME: Move this to flow pending routine.

    flowPtr->volPtr = NanoVis::volume[NanoVis::n_volumes];
    //
    // BE CAREFUL:  Set the number of slices to something
    //   slightly different for each volume.  If we have
    //   identical volumes at exactly the same position
    //   with exactly the same number of slices, the second
    //   volume will overwrite the first, so the first won't
    //   appear at all.
    //
    flowPtr->volPtr->set_n_slice(256-n);
    // volPtr->set_n_slice(512-n);
    flowPtr->volPtr->disable_cutplane(0);
    flowPtr->volPtr->disable_cutplane(1);
    flowPtr->volPtr->disable_cutplane(2);
    
    NanoVis::vol_renderer->add_volume(flowPtr->volPtr, 
	NanoVis::get_transfunc("default"));
    
    Trace("Flow Data move\n");
    float dx0 = -0.5;
    float dy0 = -0.5*flowPtr->volPtr->height/flowPtr->volPtr->width;
    float dz0 = -0.5*flowPtr->volPtr->depth/flowPtr->volPtr->width;
    flowPtr->volPtr->move(Vector3(dx0, dy0, dz0));
#endif
    return TCL_OK;
}

static Rappture::CmdSpec flowDataOps[] = {
    {"file",    2, FlowDataFileOp,    5, 5, "fileName extents",},
    {"follows", 2, FlowDataFollowsOp, 5, 5, "size extents",},
};
static int nFlowDataOps = NumCmdSpecs(flowDataOps);

static int
FlowDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
	   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nFlowDataOps, flowDataOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *	Can configure axis of stream plane, the position on the axis, and if
 *	the stream plane is visible.
 *
 */
static Rappture::SwitchSpec streamSwitches[] = 
{
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
	offsetof(Stream, hidden), 0},
    {Rappture::SWITCH_FLOAT, "-position", "number",
	offsetof(Stream, position), 0},
    {Rappture::SWITCH_END}
};

static int
FlowSliceConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    int axis;
    if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Stream *streamPtr;
    streamPtr = flowPtr->streams + axis;

    if (flowPtr->ParseRappture::ParseSwitches(interp, streamSwitches, objc - 3, objv + 3, 
	streamPtr, SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    // Save the values in the stream
    streamPtr->axis;
    NanoVis::lic_slice_x = streamPtr->position;
    NanoVis::licRenderer->set_axis(axis);
    NanoVis::licRenderer->set_offset(streamPtr->position);
    return TCL_OK;
}

static Rappture::CmdSpec flowStreamOps[] = {
    {"configure", 1, FlowStreamConfigureOp,  5, 5, "axis ?switches?",},
};

static int nFlowStreamOps = NumCmdSpecs(flowStreamOps);

static int
FlowStreamOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nFlowStreamOps, flowStreamOps,
        Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
FlowVectorIdOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Volume *volPtr;
    if (GetVolumeFromObj(interp, objv[2], &volPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (NanoVis::flowVisRenderer != NULL) {
        NanoVis::flowVisRenderer->setVectorField(volPtr->id, 
            *(volPtr->get_location()),
            1.0f,
            volPtr->height / (float)volPtr->width,
            volPtr->depth  / (float)volPtr->width,
            volPtr->wAxis.max());
        NanoVis::initParticle();
    }
    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->setVectorField(volPtr->id,
            *(volPtr->get_location()),
            1.0f / volPtr->aspect_ratio_width,
            1.0f / volPtr->aspect_ratio_height,
            1.0f / volPtr->aspect_ratio_depth,
            volPtr->wAxis.max());
        NanoVis::licRenderer->set_offset(NanoVis::lic_slice_z);
    }
    return TCL_OK;
}

static Rappture::SwitchSpec streamSwitches[] = 
{
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
	offsetof(Stream, hidden), 0},
    {Rappture::SWITCH_FLOAT, "-position", "number",
	offsetof(Stream, position), 0},
    {Rappture::SWITCH_END}
};

static Rappture::SwitchParseProc AxisSwitchProc;

static Rappture:;SwitchCustom axisSwitch = {
    AxisSwitchProc, NULL, 0,
};

/*
 *---------------------------------------------------------------------------
 *
 * AxisSwitch --
 *
 *	Convert a Tcl_Obj representing the label of a child node into its
 *	integer node id.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisSwitch(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    const char *string = Tcl_GetString(objPtr);
    if (string[1] == '\0') {
	FlowCmd::SliceAxis *axisPtr = (FlowCmd::SliceAxis *)(record + offset);
        char c;
        c = tolower((unsigned char)string[0]);
        if (c == 'x') {
            *axisPtr = FlowCmd::SliceAxis::AXIS_X;
            return TCL_OK;
        } else if (c == 'y') {
            *axisPtr = FlowCmd::SliceAxis::AXIS_Y;
            return TCL_OK;
        } else if (c == 'z') {
            *axisPtr = FlowCmd::SliceAxis::AXIS_Z;
            return TCL_OK;
        }
        /*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
                     "\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

static int
FlowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->ParseSwitches(interp, objc - 3, objv + 3) != TCL_OK) {
	return TCL_ERROR;
    }
    flowPtr->SetAxis();
    flowPtr->SetSlicePosition();
    flowPtr->SliceVisible();
    flowPtr->Visible();
    return TCL_OK;
}

static Rappture::SwitchSpec initStreamSwitches[] = 
{
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean", 
	offsetof(InitStream, hidden), 0},
    {Rappture::SWITCH_FLOAT, "-position", "number",
	offsetof(InitStream, position), 0},
    {Rappture::SWITCH_END}
};


static int
FlowParticlesAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->CreatePlane(interp, objv[3])) {
	return TCL_ERROR;
    }
    Particles *particlesPtr;
    if (flowPtr->GetPlane(interp, objv[3], &particlesPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (particlesPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	flowPtr->DestroyPlane(particlesPtr);
	return TCL_ERROR;
    }
    particlesPtr->SetPosition();
    particlesPtr->SetColor();
    particlesPtr->SetAxis();
    particlesPtr->Visible();
    return TCL_OK;
}

static int
FlowParticlesConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
			 Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    Particles *particlesPtr;
    if (flowPtr->GetPlane(interp, objv[3], &particlesPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (particlesPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	return TCL_ERROR;
    }
    particlesPtr->SetPosition();
    particlesPtr->SetColor();
    particlesPtr->SetAxis();
    particlesPtr->Visible();
    return TCL_OK;
}

static int
FlowParticlesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
		      Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    int i;
    for (i = 3; i < objc; i++) {
	Particles *particlesPtr;

	if (flowPtr->GetPlane(interp, objv[i], &particlesPtr)!=TCL_OK) {
	    return TCL_ERROR;
	}
	flowPtr->DestroyPlane(particlesPtr);
    }
    return TCL_OK;
}

static int
FlowParticlesNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    ParticlesIterator iter;
    Particles *particlesPtr;
    for (particlesPtr = flowPtr->FirstPlane(&iter); particlesPtr != NULL; 
	 particlesPtr = flowPtr->NextPlane(&iter)) {

	objPtr = Tcl_NewStringObj(particlesPtr->Name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowParticlesObjCmd --
 *
 * 	This procedure is invoked to process commands on behalf of the flow
 * 	object.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 * $flow particles oper $name 
 *---------------------------------------------------------------------------
 */
static Rappture::CmdSpec flowParticlesOps[] = {
    {"add",        1, FlowParticlesAddOp,        3, 0, "name ?switches?",},
    {"configure",  1, FlowParticlesConfigureOp,  3, 0, "name ?switches?",},
    {"delete",	   1, FlowParticlesDeleteOp,     3, 0, "?name...?"},
    {"names",      1, FlowParticlesNamesOp,      3, 4, "?pattern?"},
};

static int nFlowInjectorOps = NumCmdSpecs(flowInjectorOps);

static int
FlowInjectorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowInjectorOps, flowInjectorOps, 
	Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
    return result;
}

static int
FlowShapeAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    const char *name;
    name = Tcl_GetString(objv[3]);
    Tcl_HashEntry *hPtr;
    int isNew;
    hPtr = Tcl_CreateHashEntry(&flowPtr->injectTable, name, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "flow \"", Tcl_GetString(objv[0]), 
		"\" already has a injection plane named \"", name, "\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    InitStream *iStreamPtr;
    iStreamPtr = new InitStream;
    if (iStreamPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate a initstream \"", name, 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, iStreamPtr);
    iStreamPtr->hashPtr = hPtr;
    iStreamPtr->name = Tcl_GetHashKey(&flowPtr->injectTable, hPtr);
    if (Rappture::ParseSwitches(interp, initStreamSwitches, objc - 3, objv + 3, 
	iStreamPtr, SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
FlowShapeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    const char *name;
    name = Tcl_GetString(objv[3]);
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&flowPtr->injectTable, name);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "can't find a initstream \"", name, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    InitStream *iStreamPtr = (InitStream *)Tcl_GetHashValue(hPtr);
    Tcl_DeleteHashEntry(hPtr);
    delete iStreamPtr;
    return TCL_OK;
}

static int
FlowShapeNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Tcl_FirstHashEntry(&flowPtr->injectTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
	Tcl_Obj *objPtr;
	const char *name;

	name = Tcl_GetHashKey(&flowPtr->injectTable, hPtr);
	objPtr = Tcl_NewStringObj(name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowShapeConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
			Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    const char *name;
    name = Tcl_GetString(objv[3]);
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&flowPtr->injectTable, name);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "can't find a initstream \"", name, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    InitStream *iStreamPtr = (InitStream *)Tcl_GetHashValue(hPtr);
    if (Rappture::ParseSwitches(interp, initStreamSwitches, objc - 4, objv + 4, 
	iStreamPtr, SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowShapeOp--
 *
 * 	This procedure is invoked to process commands on behalf of the flow
 * 	object.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Rappture::CmdSpec flowShapeOps[] = {
    {"add",        1, FlowShapeAddOp,        3, 0, "name ?switches?",},
    {"configure",  1, FlowShapeConfigureOp,  3, 5, "name ?switches?",},
    {"delete",	   1, FlowShapeDeleteOp,     3, 0, "?name...?"},
    {"names",      1, FlowShapeNamesOp,      3, 4, "?pattern?"},
};

static int nFlowShapeOps = NumCmdSpecs(flowShapeOps);

static int
FlowShapeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowShapeOps, flowShapeOps, 
	Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowInstObjCmd --
 *
 * 	This procedure is invoked to process commands on behalf of the flow
 * 	object.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Rappture::CmdSpec flowInstOps[] = {
    {"configure",   1, FlowConfigureOp,  3, 5, "?switches?",},
    {"data",	    1, FlowDataOp,       4, 0, "oper ?args?"},
    {"particles",   1, FlowParticlesOp,  3, 0, "oper ?args?",},
    {"shape",       2, FlowShapeOp,      3, 0, "oper ?args?",},
};
static int nFlowInstOps = NumCmdSpecs(flowInstOps);

static int
FlowInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowInstOps, flowInstOps, 
	Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowInstDeleteProc --
 *
 *	Deletes the command associated with the tree.  This is called only
 *	when the command associated with the tree is destroyed.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FlowInstDeleteProc(ClientData clientData)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    delete flowPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowAddOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FlowAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
	  Tcl_Obj *const *objv)
{
    return FlowCmd::NewFlow(interp, Tcl_GetString(objv[2]));
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowDeleteOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FlowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
	     Tcl_Obj *const *objv)
{
    int i;

    for (i = 2; i < objc; i++) {
	FlowCmd *flowPtr;

	if (FlowCmd::GetFlowFromObj(interp, objv[i], &flowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	delete flowPtr;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowInitOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FlowInitOp(ClientData clientData, Tcl_Interp *interp, int objc,
	   Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);

    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&flowTable, &iter); hPtr != NULL; 
	hPtr = Tcl_NextHashEntry(&iter)) {
	FlowCmd *flowPtr;
	flowPtr = (FlowCmd *)Tcl_GetHashValue(hPtr);
	if (flowPtr->fieldPtr != NULL) {
	    delete flowPtr->fieldPtr; // Remove the vector field.
	    flowPtr->fieldPtr = NULL;
	}
    }
#ifdef notdef
    // For each flow vector enabled in the table add it to the renderer.
    for (hPtr = Tcl_FirstHashEntry(&flowTable, &iter); hPtr != NULL; 
	hPtr = Tcl_NextHashEntry(&iter)) {
	FlowCmd *flowPtr;
	flowPtr = (FlowCmd *)Tcl_GetHashValue(hPtr);
	if (flowPtr->hidden) {
	    continue;		// Flow is hidden.
	}
	if (flowPtr->volPtr == NULL) {
	    continue;
	}
	volPtr->set_n_slice(256-n);
	// volPtr->set_n_slice(512-n);
	volPtr->disable_cutplane(0);
	volPtr->disable_cutplane(1);
	volPtr->disable_cutplane(2);
	    
	NanoVis::vol_renderer->add_volume(volPtr,
		NanoVis::get_transfunc("default"));
	float dx0 = -0.5;
	float dy0 = -0.5*volPtr->height/volPtr->width;
	float dz0 = -0.5*volPtr->depth/volPtr->width;
	volPtr->move(Vector3(dx0, dy0, dz0));
	//volPtr->enable_data();
	volPtr->disable_data();
	NvVectorField *fieldPtr;
	fieldPtr = NanoVis::flowVisRenderer->addVectorField(volPtr, 
		*(volPtr->get_location()), 1.0f,
		volPtr->height / (float)volPtr->width,
		volPtr->depth  / (float)volPtr->width,
		1.0f);
	NanoVis::flowVisRenderer->activateVectorField(fieldPtr);
	    
	//////////////////////////////////
	// ADD Particle Injection Plane1
	NanoVis::flowVisRenderer->addPlane(fieldPtr, plane_name1);
	NanoVis::flowVisRenderer->setPlaneAxis(fieldPtr, plane_name1, 0);
	NanoVis::flowVisRenderer->setPlanePos(fieldPtr, plane_name1, 0.9);
	NanoVis::flowVisRenderer->setParticleColor(fieldPtr, plane_name1, 
						   color1);
	// ADD Particle Injection Plane2
	NanoVis::flowVisRenderer->addPlane(fieldPtr, plane_name2);
	NanoVis::flowVisRenderer->setPlaneAxis(fieldPtr, plane_name2, 0);
	NanoVis::flowVisRenderer->setPlanePos(fieldPtr, plane_name2, 0.2);
	NanoVis::flowVisRenderer->setParticleColor(fieldPtr, plane_name2, 
						   color2);
	NanoVis::flowVisRenderer->initialize(fieldPtr);
	    
	NanoVis::flowVisRenderer->activatePlane(fieldPtr, plane_name1);
	NanoVis::flowVisRenderer->activatePlane(fieldPtr, plane_name2);
	    
	NanoVis::licRenderer->setVectorField(volPtr->id,
				*(volPtr->get_location()),
				1.0f / volPtr->aspect_ratio_width,
				1.0f / volPtr->aspect_ratio_height,
				1.0f / volPtr->aspect_ratio_depth,
				volPtr->wAxis.max());
	}
    }
	const char *name;
	name = Tcl_GetCommandName(interp, flowPtr->cmdToken);
	Tcl_Obj *objPtr;
	objPtr = Tcl_NewStringObj(name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
#endif
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowNamesOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FlowNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);

    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&flowTable, &iter); hPtr != NULL; 
	hPtr = Tcl_NextHashEntry(&iter)) {
	FlowCmd *flowPtr;
	flowPtr = (FlowCmd *)Tcl_GetHashValue(hPtr);
	const char *name;
	name = Tcl_GetCommandName(interp, flowPtr->cmdToken);
	Tcl_Obj *objPtr;
	objPtr = Tcl_NewStringObj(name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowNextOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    if (!NanoVis::licRenderer->isActivated()) {
        NanoVis::licRenderer->activate();
    }
    if (!NanoVis::flowVisRenderer->isActivated()) {
        NanoVis::flowVisRenderer->activate();
    }

    Trace("sending flow playback frame\n");

    // Generate the latest frame and send it back to the client
    NanoVis::licRenderer->convolve();
    FlowCmd::AdvectFlows();
    NanoVis::offscreen_buffer_capture();  //enable offscreen render
    NanoVis::display();
    NanoVis::read_screen();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    // NanoVis::bmp_write_to_file(frame_count, fileName);
    Trace("FLOW end\n");
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    NanoVis::initParticle();
    return TCL_OK;
}

static int
FlowVideoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    int width, height;		// Resolution of video.
    int numFrames;		// Total number of frames.
    float frameRate;		// Frame rate of the video.
    float bitRate;		// Bit rate of the vide.

    if ((Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &numFrames) != TCL_OK) ||
	(GetFloatFromObj(interp, objv[5], &frameRate) != TCL_OK) ||
	(GetFloatFromObj(interp, objv[6], &bitRate) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((width<0) || (width>SHRT_MAX) || (height<0) || (height>SHRT_MAX)) {
	Tcl_AppendResult(interp, "bad dimensions for video", (char *)NULL);
	return TCL_ERROR;
    }
    if ((frameRate < 0.0f) || (frameRate > 30.0f)) {
	Tcl_AppendResult(interp, "bad frame rate \"", Tcl_GetString(objv[5]),
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((bitRate < 0.0f) || (frameRate > 30.0f)) {
	Tcl_AppendResult(interp, "bad bit rate \"", Tcl_GetString(objv[6]),
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (NanoVis::licRenderer == NULL) {
	Tcl_AppendResult(interp, "no lic renderer.", (char *)NULL);
	return TCL_ERROR;
    }
    if (NanoVis::flowVisRenderer == NULL) {
	Tcl_AppendResult(interp, "no flow renderer.", (char *)NULL);
	return TCL_ERROR;
    }
    NanoVis::licRenderer->activate();
    NanoVis::flowVisRenderer->activate();

    // Save the old dimensions of the offscreen buffer.
    int oldWidth, oldHeight;
    oldWidth = NanoVis::win_width;
    oldHeight = NanoVis::win_height;

    if ((width != oldWidth) || (height != oldHeight)) {
	// Resize to the requested size.
	NanoVis::resize_offscreen_buffer(width, height);
    }

    char fileName[128];
    sprintf(fileName,"/tmp/flow%d.mpeg", getpid());

    Trace("FLOW started\n");

    Rappture::Outcome result;
    Rappture::AVTranslate movie(width, height, frameRate, bitRate);

    int pad = 0;
    if ((3*NanoVis::win_width) % 4 > 0) {
        pad = 4 - ((3*NanoVis::win_width) % 4);
    }

    movie.init(result, fileName);

    for (int i = 0; i < numFrames; i++) {
        // Generate the latest frame and send it back to the client
	NanoVis::licRenderer->convolve();
	FlowCmd::AdvectFlows();
        NanoVis::offscreen_buffer_capture();  //enable offscreen render
        NanoVis::display();

        NanoVis::read_screen();
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        // This is done before bmp_write_to_file because bmp_write_to_file
        // turns rgb data to bgr
        movie.append(result, NanoVis::screen_buffer, pad);
        // NanoVis::bmp_write_to_file(frame_count, fileName);
    }

    movie.done(result);
    Trace("FLOW end\n");

    if (NanoVis::licRenderer) {
        NanoVis::licRenderer->deactivate();
    }
    if (NanoVis::flowVisRenderer) {
        NanoVis::flowVisRenderer->deactivate();
    }
    NanoVis::initParticle();

    // FIXME: find a way to get the data from the movie object as a void*
    Rappture::Buffer data;
    if (!data.load(result, fileName)) {
        Tcl_AppendResult(interp, "can't load data from temporary movie file \"",
		fileName, "\": ", result.remark(), (char *)NULL);
	return TCL_ERROR;
    }
    // Build the command string for the client.
    char command[200];
    sprintf(command,"nv>image -bytes %lu -type movie -token token\n", 
	    (unsigned long)data.size());

    NanoVis::sendDataToClient(command, data.bytes(), data.size());
    if (unlink(fileName) != 0) {
        Tcl_AppendResult(interp, "can't unlink temporary movie file \"",
		fileName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Rappture::CmdSpec flowCmdOps[] = {
    {"add",      1, FlowAddOp,     2, 3, "?name? ?option value...?",},
    {"delete",   1, FlowDeleteOp,  2, 0, "name...",},
    {"init",	 1, FlowInitOp,    2, 2, "",},
    {"names",    1, FlowNamesOp,   2, 3, "?pattern?",},
    {"next",     2, FlowNextOp,    2, 2, "",},
    {"reset",    1, FlowResetOp,   2, 2, "",},
    {"video",    1, FlowVideoOp,   7, 7, 	
	"width height numFrames frameRate bitRate ",},
};
static int nFlowCmdOps = NumCmdSpecs(flowCmdOps);

/*ARGSUSED*/
static int
FlowCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nFlowCmdOps, flowCmdOps, 
	Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowCmdInitProc --
 *
 *	This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
FlowCmdInitProc(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "flow", FlowCmdProc, NULL, NULL);
    Tcl_InitHashTable(&flowTable, TCL_STRING_KEYS);
    return TCL_OK;
}

