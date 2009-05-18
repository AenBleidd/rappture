
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <tcl.h>
#include "Switch.h"
#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpOutcome.h>
#include <RpAVTranslate.h>
#include "Trace.h"
#include "TransferFunction.h"

#include "nanovis.h"
#include "Command.h"
#include "CmdProc.h"
#include "Nv.h"

#include "NvLIC.h"

#include "Unirect.h"
#include "FlowCmd.h"

#define RELPOS 0
#define ABSPOS 1

static Rappture::SwitchParseProc AxisSwitchProc;
static Rappture::SwitchCustom axisSwitch = {
    AxisSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc ColorSwitchProc;
static Rappture::SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc PointSwitchProc;
static Rappture::SwitchCustom pointSwitch = {
    PointSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc PositionSwitchProc;
static Rappture::SwitchCustom positionSwitch = {
    PositionSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc TransferFunctionSwitchProc;
static Rappture::SwitchCustom transferFunctionSwitch = {
    TransferFunctionSwitchProc, NULL, 0,
};

Rappture::SwitchSpec FlowCmd::_switches[] = {
    {Rappture::SWITCH_BOOLEAN, "-slice", "boolean",
	offsetof(FlowValues, sliceVisible), 0},
    {Rappture::SWITCH_CUSTOM, "-axis", "axis",
	offsetof(FlowValues, slicePos.axis), 0, 0, &axisSwitch},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
	offsetof(FlowValues, isHidden), 0},
    {Rappture::SWITCH_CUSTOM, "-position", "number",
	offsetof(FlowValues, slicePos), 0, 0, &positionSwitch},
    {Rappture::SWITCH_CUSTOM, "-transferfunction", "name",
        offsetof(FlowValues, tfPtr), 0, 0, &transferFunctionSwitch},
    {Rappture::SWITCH_BOOLEAN, "-volume", "boolean",
	offsetof(FlowValues, showVolume), 0},
    {Rappture::SWITCH_BOOLEAN, "-outline", "boolean",
	offsetof(FlowValues, showOutline), 0},
    {Rappture::SWITCH_END}
};

Rappture::SwitchSpec FlowParticles::_switches[] = {
    {Rappture::SWITCH_CUSTOM, "-axis", "string",
     offsetof(FlowParticlesValues, position.axis), 0, 0, &axisSwitch},
    {Rappture::SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowParticlesValues, color), 0, 0,  &colorSwitch},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowParticlesValues, isHidden), 0},
    {Rappture::SWITCH_CUSTOM, "-position", "number",
	offsetof(FlowValues, slicePos), 0, 0, &positionSwitch},
    {Rappture::SWITCH_END}
};

Rappture::SwitchSpec FlowBox::_switches[] = {
    {Rappture::SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowBoxValues, color), 0, 0,  &colorSwitch},
    {Rappture::SWITCH_CUSTOM, "-corner1", "{x y z}",
     offsetof(FlowBoxValues, corner1), 0, 0, &pointSwitch},
    {Rappture::SWITCH_CUSTOM, "-corner2", "{x y z}",
     offsetof(FlowBoxValues, corner2), 0, 0, &pointSwitch},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowBoxValues, isHidden), 0},
    {Rappture::SWITCH_FLOAT, "-linewidth", "number",
     offsetof(FlowBoxValues, lineWidth), 0},
    {Rappture::SWITCH_END}
};


static Tcl_ObjCmdProc FlowInstObjCmd;
static Tcl_CmdDeleteProc FlowInstDeleteProc;


FlowParticles::FlowParticles(const char *name, Tcl_HashEntry *hPtr) 
{
    memset(this, 0, sizeof(FlowParticles));
    _name = name;
    _hashPtr = hPtr;
    _sv.isHidden = false;
    _sv.position.axis = 0;		/* X_AXIS */
    _sv.position.value = 0.0f;	
    _sv.position.flags = RELPOS;
    _sv.color.r = _sv.color.b = _sv.color.g = _sv.color.a = 1.0f;
    _rendererPtr = new NvParticleRenderer(NMESH, NMESH, 
		/* Global nVidia Cg context */g_context);
}

void
FlowParticles::Render(void) 
{
    Trace("rendering particles %s\n", _name);
    _rendererPtr->setPos(FlowCmd::GetRelativePosition(&_sv.position));
    _rendererPtr->setAxis(_sv.position.axis);
    assert(_rendererPtr->active());
    _rendererPtr->render();
}

void 
FlowParticles::Configure(void) 
{
    _rendererPtr->setPos(FlowCmd::GetRelativePosition(&_sv.position));
    _rendererPtr->setColor(Vector4(_sv.color.r, _sv.color.g, _sv.color.b, 
		_sv.color.a));
    _rendererPtr->setAxis(_sv.position.axis);
    _rendererPtr->active(!_sv.isHidden);
}

FlowBox::FlowBox(const char *name, Tcl_HashEntry *hPtr) 
{
    _name = name;
    _hashPtr = hPtr;
    _sv.isHidden = false;
    _sv.corner1.x = 0.0f;	
    _sv.corner1.y = 0.0f;	
    _sv.corner1.z = 0.0f;	
    _sv.corner2.x = 1.0f;	
    _sv.corner2.y = 1.0f;	
    _sv.corner2.z = 1.0f;	
    _sv.lineWidth = 1.2f;
    _sv.color.r = _sv.color.b = _sv.color.g = _sv.color.a = 1.0f;

}

void 
FlowBox::Render(Volume *volPtr)
{
    Trace("rendering boxes %s\n", _name);
    if ((_sv.corner1.x == _sv.corner2.x) ||
	(_sv.corner1.y == _sv.corner2.y) ||
	(_sv.corner1.z == _sv.corner2.z)) {
	return;				
    }
    Trace("rendering boxes %s\n", _name);
    glColor4d(_sv.color.r, _sv.color.g, _sv.color.b, _sv.color.a);

    glPushMatrix();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    glPushMatrix();
    Vector3 *originPtr = volPtr->get_location();
    glTranslatef(originPtr->x, originPtr->y, originPtr->z);

    double sx, sy, sz;
    sx = 1.0;
    sy = volPtr->height / (double)volPtr->width;
    sz = volPtr->depth  / (double)volPtr->width;
    glScaled(sx, sy, sz);

    Vector3 min, max;

    min = volPtr->getPhysicalBBoxMin();
    max = volPtr->getPhysicalBBoxMax();

    float x0, y0, z0, x1, y1, z1;
    x0 = (_sv.corner1.x - min.x) / (max.x - min.x);
    y0 = (_sv.corner1.y - min.y) / (max.y - min.y);
    z0 = (_sv.corner1.z - min.z) / (max.z - min.z);
    x1 = (_sv.corner2.x - min.x) / (max.x - min.x);
    y1 = (_sv.corner2.y - min.y) / (max.y - min.y);
    z1 = (_sv.corner2.z - min.z) / (max.z - min.z);
    
    Trace("rendering box %g,%g %g,%g %g,%g\n", x0, x1, y0, y1, z0, z1);

    glLineWidth(_sv.lineWidth);
    glBegin(GL_LINE_LOOP); 
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y1, z0);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z1);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x0, y0, z1);
	glVertex3d(x0, y1, z1);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x1, y1, z0);
    }
    glEnd();

    glPopMatrix();
    assert(CheckGL(AT));
    glPopMatrix();
    assert(CheckGL(AT));

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}


FlowCmd::FlowCmd(Tcl_Interp *interp, const char *name, Tcl_HashEntry *hPtr)
{
    memset(this, 0, sizeof(FlowCmd));
    _name = name;
    _interp = interp;
    _hashPtr = hPtr;
    _volIndex = -1;			/* Indicates that no volume slot has
					 * been allocated for this vector. */
    _sv.sliceVisible = 1;
    _volPtr = NULL;
    _cmdToken = Tcl_CreateObjCommand(interp, (char *)_name, 
	(Tcl_ObjCmdProc *)FlowInstObjCmd, this, FlowInstDeleteProc);
    Tcl_InitHashTable(&_particlesTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&_boxTable, TCL_STRING_KEYS);
}

FlowCmd::~FlowCmd(void)
{
    Rappture::FreeSwitches(_switches, &_sv, 0);
    if (_hashPtr != NULL) {
	Tcl_DeleteHashEntry(_hashPtr);
    }
    if (_fieldPtr != NULL) {
	delete _fieldPtr;
    }
    if (_dataPtr != NULL) {
	delete _dataPtr;
    }
     if (_volPtr != NULL) {
	delete _volPtr;
	_volPtr = NULL;
	NanoVis::volume[_volIndex] = NULL;
	NanoVis::vol_renderer->remove_volume(_volIndex);
    }

    FlowBox *boxPtr;
    FlowBoxIterator boxIter;
    for (boxPtr = FirstBox(&boxIter); boxPtr != NULL; 
	 boxPtr = NextBox(&boxIter)) {
	boxPtr->disconnect();
	delete boxPtr;
    }	
    FlowParticles *particlesPtr;
    FlowParticlesIterator partIter;
    for (particlesPtr = FirstParticles(&partIter); particlesPtr != NULL;
	 particlesPtr = NextParticles(&partIter)) {
	particlesPtr->disconnect();
	delete particlesPtr;
    }	
    Tcl_DeleteHashTable(&_particlesTable);
    Tcl_DeleteHashTable(&_boxTable);
}

void
FlowCmd::ResetParticles(void)
{
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = FirstParticles(&iter); particlesPtr != NULL;
	 particlesPtr = NextParticles(&iter)) {
	particlesPtr->Reset();
    }
}

void
FlowCmd::Advect(void)
{
    NvVectorField *fieldPtr;
    fieldPtr = VectorField();
    fieldPtr->active(true);
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = FirstParticles(&iter); particlesPtr != NULL;
	 particlesPtr = NextParticles(&iter)) {
	if (particlesPtr->visible()) {
	    particlesPtr->Advect();
	}
    }
}

void
FlowCmd::Render(void)
{
    _fieldPtr->active(true);
    _fieldPtr->render();
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = FirstParticles(&iter); particlesPtr != NULL; 
	 particlesPtr = NextParticles(&iter)) {
	if (particlesPtr->visible()) {
	    particlesPtr->Render();
	}
    }
    Trace("in Render before boxes %s\n", _name);
    RenderBoxes();
}

int
FlowCmd::CreateParticles(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    const char *particlesName = Tcl_GetString(objPtr);
    hPtr = Tcl_CreateHashEntry(&_particlesTable, particlesName, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "particle injection plane \"",
			 particlesName, "\" already exists.", (char *)NULL);
	return TCL_ERROR;
    }
    particlesName = Tcl_GetHashKey(&_particlesTable, hPtr);
    FlowParticles *particlesPtr;
    particlesPtr = new FlowParticles(particlesName, hPtr);
    if (particlesPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate particle injection plane",
		(char *)NULL);
	Tcl_DeleteHashEntry(hPtr);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, particlesPtr);
    return TCL_OK;
}

int
FlowCmd::GetParticles(Tcl_Interp *interp, Tcl_Obj *objPtr, 
		      FlowParticles **particlesPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&_particlesTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a particle injection plane \"",
			 Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *particlesPtrPtr = (FlowParticles *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

FlowParticles *
FlowCmd::FirstParticles(FlowParticlesIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&_particlesTable, 
	&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowParticles *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowParticles *
FlowCmd::NextParticles(FlowParticlesIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowParticles *)Tcl_GetHashValue(iterPtr->hashPtr);
}

int
FlowCmd::CreateBox(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    hPtr = Tcl_CreateHashEntry(&_boxTable, Tcl_GetString(objPtr), &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "box \"", Tcl_GetString(objPtr),
		"\" already exists in flow \"", name(), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    const char *boxName;
    boxName = Tcl_GetHashKey(&_boxTable, hPtr);
    FlowBox *boxPtr;
    boxPtr = new FlowBox(boxName, hPtr);
    if (boxPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate box \"", boxName, "\"",
			 (char *)NULL);
	Tcl_DeleteHashEntry(hPtr);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, boxPtr);
    return TCL_OK;
}

int
FlowCmd::GetBox(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowBox **boxPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&_boxTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a box \"", 
		Tcl_GetString(objPtr), "\" in flow \"", name(), "\"", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    *boxPtrPtr = (FlowBox *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

FlowBox *
FlowCmd::FirstBox(FlowBoxIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&_boxTable, &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowBox *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowBox *
FlowCmd::NextBox(FlowBoxIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowBox *)Tcl_GetHashValue(iterPtr->hashPtr);
}


void
FlowCmd::InitVectorField(void)
{
    if (_volPtr != NULL) {
	delete _volPtr;
	_volPtr = NULL;
	NanoVis::volume[_volIndex] = NULL;
	NanoVis::vol_renderer->remove_volume(_volIndex);
    }
    // Remove the associated vector field.
    if (_fieldPtr != NULL) {
	delete _fieldPtr;
	_fieldPtr = NULL;
    }
    
}

void
FlowCmd::InitializeParticles(void)
{
    FlowParticles *particlesPtr;
    FlowParticlesIterator iter;
    for (particlesPtr = FirstParticles(&iter); particlesPtr != NULL;
	 particlesPtr = NextParticles(&iter)) {
	particlesPtr->Initialize();
    }	
}

bool
FlowCmd::ScaleVectorField()
{
    if (_volPtr != NULL) {
	delete _volPtr;
	_volPtr = NULL;
	NanoVis::volume[_volIndex] = NULL;
	NanoVis::vol_renderer->remove_volume(_volIndex);
    }
    float *vdata;
    vdata = GetScaledVector();
    if (vdata == NULL) {
	return false;
    }
    Volume *volPtr;
    volPtr = MakeVolume(vdata);
    delete [] vdata;
    if (volPtr == NULL) {
	return false;
    }
    _volPtr = volPtr;

    _fieldPtr = new NvVectorField();
    if (_fieldPtr == NULL) {
	return false;
    }

    double width, height, depth;
    width  = NanoVis::xMax - NanoVis::xMin;
    height = NanoVis::yMax - NanoVis::yMin;
    depth  = NanoVis::zMax - NanoVis::zMin;

    Vector3 *locationPtr = _volPtr->get_location();
    /*This is wrong. Need to compute origin. */
    NanoVis::xOrigin = locationPtr->x;
    NanoVis::yOrigin = locationPtr->y;
    NanoVis::zOrigin = locationPtr->z;

    _fieldPtr->setVectorField(_volPtr, *locationPtr, 
	1.0f, height / width, depth  / width, NanoVis::magMax);

    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->setVectorField(_volPtr->id, 
		*locationPtr,
		1.0f / _volPtr->aspect_ratio_width,
		1.0f / _volPtr->aspect_ratio_height,
		1.0f / _volPtr->aspect_ratio_depth,
		_volPtr->wAxis.max());
	SetCurrentPosition();
	SetAxis();
	SetActive();
    }
    FlowParticles *particlesPtr;
    FlowParticlesIterator partIter;
    for (particlesPtr = FirstParticles(&partIter); particlesPtr != NULL;
	 particlesPtr = NextParticles(&partIter)) {
	particlesPtr->SetVectorField(_volPtr);
    }	
    return true;
}

void
FlowCmd::RenderBoxes(void)
{
    FlowBoxIterator iter;
    FlowBox *boxPtr;
    for (boxPtr = FirstBox(&iter); boxPtr != NULL; boxPtr = NextBox(&iter)) {
	if (boxPtr->visible()) {
	    boxPtr->Render(_volPtr);
	}
    }
}

float *
FlowCmd::GetScaledVector(void)
{
    assert(_dataPtr->nComponents() == 3);
    size_t n = _dataPtr->nValues() / _dataPtr->nComponents() * 4;
    float *data = new float[n];
    if (data == NULL) {
	return NULL;
    }
    memset(data, 0, sizeof(float) * n);
    float *destPtr = data;
    const float *values = _dataPtr->values();
    for (size_t iz=0; iz < _dataPtr->zNum(); iz++) {
	for (size_t iy=0; iy < _dataPtr->yNum(); iy++) {
	    for (size_t ix=0; ix < _dataPtr->xNum(); ix++) {
		double vx, vy, vz, vm;
		vx = values[0];
		vy = values[1];
		vz = values[2];
		vm = sqrt(vx*vx + vy*vy + vz*vz);
		destPtr[0] = vm / NanoVis::magMax; 
		destPtr[1] = vx /(2.0*NanoVis::magMax) + 0.5; 
		destPtr[2] = vy /(2.0*NanoVis::magMax) + 0.5; 
		destPtr[3] = vz /(2.0*NanoVis::magMax) + 0.5; 
		values += 3;
		destPtr += 4;
	    }
	}
    }
    return data;
}

Volume *
FlowCmd::MakeVolume(float *data)
{
    if (_volIndex < 0) {
	_volIndex = NanoVis::n_volumes;
	Trace("VolumeIndex is %d\n", _volIndex);
    }
    Volume *volPtr;
    volPtr = NanoVis::load_volume(_volIndex, _dataPtr->xNum(), 
	_dataPtr->yNum(), _dataPtr->zNum(), 4, data, 
	NanoVis::magMin, NanoVis::magMax, 0);
    volPtr->xAxis.SetRange(_dataPtr->xMin(), _dataPtr->xMax());
    volPtr->yAxis.SetRange(_dataPtr->yMin(), _dataPtr->yMax());
    volPtr->zAxis.SetRange(_dataPtr->zMin(), _dataPtr->zMax());
    volPtr->wAxis.SetRange(NanoVis::magMin, NanoVis::magMax);

    /*volPtr->update_pending = false;*/
    Vector3 physicalMin(NanoVis::xMin, NanoVis::yMin, NanoVis::zMin);
    Vector3 physicalMax(NanoVis::xMax, NanoVis::yMax, NanoVis::zMax);
    volPtr->setPhysicalBBox(physicalMin, physicalMax);

    volPtr->set_n_slice(256 - _volIndex);
    // volPtr->set_n_slice(512- _volIndex);
    volPtr->disable_cutplane(0);
    volPtr->disable_cutplane(1);
    volPtr->disable_cutplane(2);

    TransferFunction *tfPtr;
    tfPtr = _sv.tfPtr;
    if (tfPtr == NULL) {
	tfPtr = NanoVis::get_transfunc("default");
    }
    NanoVis::vol_renderer->add_volume(volPtr, tfPtr);
    if (_sv.showVolume) {
	volPtr->enable_data();
    } else {
	volPtr->disable_data();
    }
    if (_sv.showOutline) {
	volPtr->enable_outline();
    } else {
	volPtr->disable_outline();
    }
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

    int nComponents;
    if (Tcl_GetIntFromObj(interp, objv[4], &nComponents) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((nComponents < 1) || (nComponents > 4)) {
	Tcl_AppendResult(interp, "bad # of components \"", 
			 Tcl_GetString(objv[4]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Rappture::Buffer buf;
    if (!buf.load(result, fileName)) {
	Tcl_AppendResult(interp, "can't load data from \"", fileName, "\": ",
			 result.remark(), (char *)NULL);
	return TCL_ERROR;
    }

    FlowCmd *flowPtr = (FlowCmd *)clientData;
    size_t length = buf.size();
    char *bytes = (char *)buf.bytes();
    if ((length > 4) && (strncmp(bytes, "<DX>", 4) == 0)) {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!dataPtr->ImportDx(result, nComponents, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if ((length > 11) && (strncmp(bytes, "<unirect3d>", 11) == 0)) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect3d", &cmdInfo);
	if (Tcl_Eval(interp, (const char *)buf.bytes()+11) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if ((length > 11) && (strncmp(bytes, "<unirect2d>", 11) == 0)) {
	Rappture::Unirect2d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect2d();
	if (!Tcl_GetCommandInfo(interp, "unirect2d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect2d", &cmdInfo);
	if (Tcl_Eval(interp, (const char *)buf.bytes()+11) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	Rappture::Unirect3d *d3Ptr = new Rappture::Unirect3d(nComponents);
	d3Ptr->Convert(dataPtr);
	flowPtr->SetData(d3Ptr);
	delete dataPtr;
    } else {
	Rappture::Unirect3d *dataPtr;

	fprintf(stderr, "header is %.14s\n", buf.bytes());
	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!dataPtr->ImportDx(result, nComponents, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

/*
 * $flow data follows nbytes nComponents
 */
static int
FlowDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

    Trace("Flow Data Loading\n");

    int nBytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nBytes) != TCL_OK) {
	Trace("Bad nBytes \"%s\"\n", Tcl_GetString(objv[3]));
        return TCL_ERROR;
    }
    if (nBytes <= 0) {
	Tcl_AppendResult(interp, "bad # bytes request \"", 
		Tcl_GetString(objv[3]), "\" for \"data follows\"", (char *)NULL);
	Trace("Bad nbytes %d\n", nBytes);
	return TCL_ERROR;
    }
    int nComponents;
    if (Tcl_GetIntFromObj(interp, objv[4], &nComponents) != TCL_OK) {
	Trace("Bad # of components \"%s\"\n", Tcl_GetString(objv[4]));
        return TCL_ERROR;
    }
    if (nComponents <= 0) {
	Tcl_AppendResult(interp, "bad # of components request \"", 
		Tcl_GetString(objv[4]), "\" for \"data follows\"", (char *)NULL);
	Trace("Bad # of components %d\n", nComponents);
	return TCL_ERROR;
    }
    Rappture::Buffer buf;
    Trace("Flow Data Loading %d %d\n", nBytes, nComponents);
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    size_t length = buf.size();
    char *bytes = (char *)buf.bytes();
    if ((length > 4) && (strncmp(bytes, "<DX>", 4) == 0)) {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!dataPtr->ImportDx(result, nComponents, length - 4, bytes + 4)) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if ((length > 11) && (strncmp(bytes, "<unirect3d>", 11) == 0)) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect3d", &cmdInfo);
	if (Tcl_Eval(interp, bytes+11) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if ((length > 11) && (strncmp(bytes, "<unirect2d>", 11) == 0)) {
	Rappture::Unirect2d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect2d();
	if (!Tcl_GetCommandInfo(interp, "unirect2d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	cmdInfo.objClientData = (ClientData)dataPtr;	
	Tcl_SetCommandInfo(interp, "unirect2d", &cmdInfo);
	if (Tcl_Eval(interp, bytes+11) != TCL_OK) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	if (!dataPtr->isInitialized()) {
	    delete dataPtr;
	    return TCL_ERROR;
	}
	Rappture::Unirect3d *d3Ptr = new Rappture::Unirect3d(nComponents);
	d3Ptr->Convert(dataPtr);
	flowPtr->SetData(d3Ptr);
	delete dataPtr;
    } else {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(nComponents);
	if (!dataPtr->ImportDx(result, nComponents, length, bytes)) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    }
    {
        char info[1024];
	ssize_t nWritten;
	size_t length;

        length = sprintf(info, "nv>data tag %s id 0 min %g max %g vmin %g vmax %g\n",
			 flowPtr->name(), NanoVis::magMin,
			 NanoVis::magMax, NanoVis::xMin, NanoVis::xMax);
        nWritten  = write(0, info, length);
	assert(nWritten == (ssize_t)strlen(info));
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static Rappture::CmdSpec flowDataOps[] = {
    {"file",    2, FlowDataFileOp,    5, 5, "fileName nComponents",},
    {"follows", 2, FlowDataFollowsOp, 5, 5, "size nComponents",},
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

float
FlowCmd::GetRelativePosition(FlowPosition *posPtr)
{
    if (posPtr->flags == RELPOS) {
	return posPtr->value;
    }
    switch (posPtr->axis) {
    case AXIS_X:  
	return (posPtr->value - NanoVis::xMin) / 
	    (NanoVis::xMax - NanoVis::xMin); 
    case AXIS_Y:  
	return (posPtr->value - NanoVis::yMin) / 
	    (NanoVis::yMax - NanoVis::yMin); 
    case AXIS_Z:  
	return (posPtr->value - NanoVis::zMin) / 
	    (NanoVis::zMax - NanoVis::zMin); 
    }
    return 0.0;
}

float
FlowCmd::GetRelativePosition(void) 
{
    return FlowCmd::GetRelativePosition(&_sv.slicePos);
}

/* Static NanoVis class commands. */

void
NanoVis::InitFlows(void) 
{
    Tcl_InitHashTable(&flowTable, TCL_STRING_KEYS);
}

FlowCmd *
NanoVis::FirstFlow(FlowIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&flowTable, &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowCmd *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowCmd *
NanoVis::NextFlow(FlowIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowCmd *)Tcl_GetHashValue(iterPtr->hashPtr);
}

int
NanoVis::GetFlow(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowCmd **flowPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&flowTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a flow \"", 
			     Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *flowPtrPtr = (FlowCmd *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

int
NanoVis::CreateFlow(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    const char *name;
    name = Tcl_GetString(objPtr);
    hPtr = Tcl_CreateHashEntry(&flowTable, name, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "flow \"", name, "\" already exists.",
			 (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_CmdInfo cmdInfo;
    if (Tcl_GetCommandInfo(interp, name, &cmdInfo)) {
	Tcl_AppendResult(interp, "an another command \"", name, 
			 "\" already exists.", (char *)NULL);
	return TCL_ERROR;
    }	
    FlowCmd *flowPtr;
    name = Tcl_GetHashKey(&flowTable, hPtr);
    flowPtr = new FlowCmd(interp, name, hPtr);
    if (flowPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate a flow object \"", name, 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, flowPtr);
    EventuallyRedraw(MAP_FLOWS);
    return TCL_OK;
}

void
NanoVis::DeleteFlows(Tcl_Interp *interp)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	flowPtr->disconnect();		/* Don't disrupt the hash walk */
	Tcl_DeleteCommand(interp, flowPtr->name());
    }
    Tcl_DeleteHashTable(&flowTable);
}

bool
NanoVis::MapFlows(void)
{
    flags &= ~MAP_FLOWS;


    /* 
     * Step 1.  Get the overall min and max magnitudes of all the 
     *		flow vectors.
     */
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	double min, max;
	if (!flowPtr->isDataLoaded()) {
	    continue;
	}
	Rappture::Unirect3d *dataPtr;
	dataPtr = flowPtr->GetData();
	min = dataPtr->magMin();
	max = dataPtr->magMax();
	if (min < magMin) {
	    magMin = min;
	} 
	if (max > magMax) {
	    magMax = max;
	}
	if (dataPtr->xMin() < xMin) {
	    xMin = dataPtr->xMin();
	}
	if (dataPtr->yMin() < yMin) {
	    yMin = dataPtr->yMin();
	}
	if (dataPtr->zMin() < zMin) {
	    zMin = dataPtr->zMin();
	}
	if (dataPtr->xMax() > xMax) {
	    xMax = dataPtr->xMax();
	}
	if (dataPtr->yMax() > yMax) {
	    yMax = dataPtr->yMax();
	}
	if (dataPtr->zMax() > zMax) {
	    zMax = dataPtr->zMax();
	}
    }

    /* 
     * Step 2.  Generate the vector field from each data set. 
     *		Delete the currently generated fields. 
     */
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if (!flowPtr->isDataLoaded()) {
	    continue;
	}
	flowPtr->InitVectorField();
	if (!flowPtr->visible()) {
	    continue;
	}
	flowPtr->InitializeParticles();
	if (!flowPtr->ScaleVectorField()) {
	    return false;
	}
	licRenderer->set_offset(flowPtr->GetRelativePosition());
    }
    AdvectFlows();
    return true;
}

void
NanoVis::RenderFlows(void)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
	    flowPtr->Render();
	}
    }
    flags &= ~REDRAW_PENDING;
}

void
NanoVis::ResetFlows(void)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
	    flowPtr->ResetParticles();
	}
    }
}    

void
NanoVis::AdvectFlows(void)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
	    flowPtr->Advect();
	}
    }
}    

/*
 *---------------------------------------------------------------------------
 *
 * AxisSwitchProc --
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
AxisSwitchProc(
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
            *axisPtr = FlowCmd::AXIS_X;
            return TCL_OK;
        } else if (c == 'y') {
            *axisPtr = FlowCmd::AXIS_Y;
            return TCL_OK;
        } else if (c == 'z') {
            *axisPtr = FlowCmd::AXIS_Z;
            return TCL_OK;
        }
        /*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
                     "\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *	Convert a Tcl_Obj representing the label of a list of four color 
 *	components in to a RGBA color value.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    Tcl_Obj **objv;
    int objc;
    FlowColor *colorPtr = (FlowColor *)(record + offset);

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc < 3) || (objc > 4)) {
	Tcl_AppendResult(interp, "wrong # of elements in color definition",
			 (char *)NULL);
	return TCL_ERROR;
    }
    float values[4];
    int i;
    values[3] = 1.0f;
    for (i = 0; i < objc; i++) {
	float value;

	if (GetFloatFromObj(interp, objv[i], &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((value < 0.0) || (value > 1.0)) {
	    Tcl_AppendResult(interp, "bad component value in \"",
		Tcl_GetString(objPtr), "\": color values must be [0..1]",
		(char *)NULL);
	    return TCL_ERROR;
	}
	values[i] = value;
    }	    
    colorPtr->r = values[0];
    colorPtr->g = values[1];
    colorPtr->b = values[2];
    colorPtr->a = values[3];
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PointSwitchProc --
 *
 *	Convert a Tcl_Obj representing the a 3-D coordinate into
 *	a point.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PointSwitchProc(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    FlowPoint *pointPtr = (FlowPoint *)(record + offset);
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc != 3) {
	Tcl_AppendResult(interp, "wrong # of elements for box coordinates: "
			 " should be \"x y z\"", (char *)NULL);
	return TCL_ERROR;
    }
    float values[3];
    int i;
    for (i = 0; i < objc; i++) {
	float value;

	if (GetFloatFromObj(interp, objv[i], &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	values[i] = value;
    }	    
    pointPtr->x = values[0];
    pointPtr->y = values[1];
    pointPtr->z = values[2];
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PositionSwitchProc --
 *
 *	Convert a Tcl_Obj representing the a 3-D coordinate into
 *	a point.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PositionSwitchProc(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    FlowPosition *posPtr = (FlowPosition *)(record + offset);
    const char *string;
    char *p;

    string = Tcl_GetString(objPtr);
    p = strrchr(string, '%');
    if (p == NULL) {
	float value;

	if (GetFloatFromObj(interp, objPtr, &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	posPtr->value = value;
	posPtr->flags = ABSPOS;
    } else {
	double value;

	*p = '\0';
	if (Tcl_GetDouble(interp, string, &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	posPtr->value = (float)value * 0.01;
	posPtr->flags = RELPOS;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TransferFunctionSwitchProc --
 *
 *	Convert a Tcl_Obj representing the transfer function into a
 *	TransferFunction pointer.  The transfer function must have been
 *	previously defined.
 *
 * Results:
 *	The return value is a standard Tcl result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TransferFunctionSwitchProc(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    TransferFunction **funcPtrPtr = (TransferFunction **)(record + offset);
    TransferFunction *funcPtr;
    funcPtr = NanoVis::get_transfunc(Tcl_GetString(objPtr));
    if (funcPtr == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", Tcl_GetString(objPtr),
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    *funcPtrPtr = funcPtr;
    return TCL_OK;
}

static int
FlowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->ParseSwitches(interp, objc - 2, objv + 2) != TCL_OK) {
	return TCL_ERROR;
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->CreateParticles(interp, objv[3]) != TCL_OK) {
	return TCL_ERROR;
    }
    FlowParticles *particlesPtr;
    if (flowPtr->GetParticles(interp, objv[3], &particlesPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (particlesPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	delete particlesPtr;
	return TCL_ERROR;
    }
    particlesPtr->Configure();
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowParticlesConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
			 Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    FlowParticles *particlesPtr;
    if (flowPtr->GetParticles(interp, objv[3], &particlesPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (particlesPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	return TCL_ERROR;
    }
    particlesPtr->Configure();
    NanoVis::EventuallyRedraw();
    return TCL_OK;
}

static int
FlowParticlesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
		      Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    int i;
    for (i = 3; i < objc; i++) {
	FlowParticles *particlesPtr;

	if (flowPtr->GetParticles(NULL, objv[i], &particlesPtr) == TCL_OK) {
	    delete particlesPtr;
	}
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = flowPtr->FirstParticles(&iter); particlesPtr != NULL; 
	 particlesPtr = flowPtr->NextParticles(&iter)) {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewStringObj(particlesPtr->name(), -1);
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
    {"add",        1, FlowParticlesAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowParticlesConfigureOp,  4, 0, "name ?switches?",},
    {"delete",	   1, FlowParticlesDeleteOp,     4, 0, "?name...?"},
    {"names",      1, FlowParticlesNamesOp,      3, 4, "?pattern?"},
};

static int nFlowParticlesOps = NumCmdSpecs(flowParticlesOps);

static int
FlowParticlesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowParticlesOps, flowParticlesOps, 
	Rappture::CMDSPEC_ARG2, objc, objv, 0);
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
FlowBoxAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->CreateBox(interp, objv[3]) != TCL_OK) {
	return TCL_ERROR;
    }
    FlowBox *boxPtr;
    if (flowPtr->GetBox(interp, objv[3], &boxPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (boxPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	delete boxPtr;
	return TCL_ERROR;
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowBoxDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    int i;
    for (i = 3; i < objc; i++) {
	FlowBox *boxPtr;

	if (flowPtr->GetBox(NULL, objv[i], &boxPtr) == TCL_OK) {
	    delete boxPtr;
	}
    }
    NanoVis::EventuallyRedraw();
    return TCL_OK;
}

static int
FlowBoxNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    FlowBoxIterator iter;
    FlowBox *boxPtr;
    for (boxPtr = flowPtr->FirstBox(&iter); boxPtr != NULL; 
	 boxPtr = flowPtr->NextBox(&iter)) {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewStringObj(boxPtr->name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowBoxConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
			Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    FlowBox *boxPtr;
    if (flowPtr->GetBox(interp, objv[3], &boxPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (boxPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
	return TCL_ERROR;
    }
    NanoVis::EventuallyRedraw();
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowBoxOp--
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
static Rappture::CmdSpec flowBoxOps[] = {
    {"add",        1, FlowBoxAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowBoxConfigureOp,  4, 0, "name ?switches?",},
    {"delete",	   1, FlowBoxDeleteOp,     3, 0, "?name...?"},
    {"names",      1, FlowBoxNamesOp,      3, 0, "?pattern?"},
};

static int nFlowBoxOps = NumCmdSpecs(flowBoxOps);

static int
FlowBoxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowBoxOps, flowBoxOps, 
	Rappture::CMDSPEC_ARG2, objc, objv, 0);
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
    {"box",         1, FlowBoxOp,        2, 0, "oper ?args?"},
    {"configure",   1, FlowConfigureOp,  2, 0, "?switches?"},
    {"data",	    1, FlowDataOp,       2, 0, "oper ?args?"},
    {"particles",   1, FlowParticlesOp,  2, 0, "oper ?args?"}
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
    assert(CheckGL(AT));
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
    if (NanoVis::CreateFlow(interp, objv[2]) != TCL_OK) {
	return TCL_ERROR;
    }
    FlowCmd *flowPtr;
    if (NanoVis::GetFlow(interp, objv[2], &flowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (flowPtr->ParseSwitches(interp, objc - 3, objv + 3) != TCL_OK) {
	Tcl_DeleteCommand(interp, flowPtr->name());
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[2]);
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
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

	if (NanoVis::GetFlow(interp, objv[i], &flowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DeleteCommand(interp, flowPtr->name());
    }
    NanoVis::EventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlowExistsOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FlowExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
	     Tcl_Obj *const *objv)
{
    bool value;
    FlowCmd *flowPtr;

    value = false;
    if (NanoVis::GetFlow(NULL, objv[2], &flowPtr) == TCL_OK) {
	value = true;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (int)value);
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
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = NanoVis::FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NanoVis::NextFlow(&iter)) {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewStringObj(flowPtr->name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowNextOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    assert(NanoVis::licRenderer != NULL);
    if (NanoVis::flags & NanoVis::MAP_FLOWS) {
	NanoVis::MapFlows();
    }
    NanoVis::EventuallyRedraw();
    NanoVis::licRenderer->convolve();
    NanoVis::AdvectFlows();
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    NanoVis::ResetFlows();
    NanoVis::licRenderer->reset();
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
	NanoVis::AdvectFlows();
	NanoVis::RenderFlows();
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

#ifdef notdef
    if (NanoVis::licRenderer) {
        NanoVis::licRenderer->active(false);
    }
#endif
    NanoVis::licRenderer->make_patterns();

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
    {"add",      1, FlowAddOp,     3, 0, "name ?option value...?",},
    {"delete",   1, FlowDeleteOp,  2, 0, "name...",},
    {"exists",   1, FlowExistsOp,  3, 3, "name",},
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
    NanoVis::InitFlows();
    return TCL_OK;
}

