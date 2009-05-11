
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

unsigned int FlowCmd::flags = 0;
Tcl_HashTable FlowCmd::_flowTable;
float FlowCmd::_magMin, FlowCmd::_magMax;
float FlowCmd::_xMin,   FlowCmd::_xMax;
float FlowCmd::_yMin,   FlowCmd::_yMax;
float FlowCmd::_zMin,   FlowCmd::_zMax;
float FlowCmd::_wMin,   FlowCmd::_wMax;
float FlowCmd::_xOrigin, FlowCmd::_yOrigin, FlowCmd::_zOrigin;

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
    fprintf(stderr, "rendering particles, position=%g\n",
	    FlowCmd::GetPosition(&_sv.position));
    _rendererPtr->setPos(FlowCmd::GetPosition(&_sv.position));
    _rendererPtr->setAxis(_sv.position.axis);
    assert(_rendererPtr->isActivated());
    _rendererPtr->render();
}

void 
FlowParticles::Configure(void) 
{
    _rendererPtr->setPos(_sv.position.value);
    _rendererPtr->setColor(Vector4(_sv.color.r, _sv.color.g, _sv.color.b, 
		_sv.color.a));
    _rendererPtr->setAxis(_sv.position.axis);
    if ((_sv.isHidden) && (_rendererPtr->isActivated())) {
	_rendererPtr->deactivate();
    } else if ((!_sv.isHidden) && (!_rendererPtr->isActivated())) {
	_rendererPtr->activate();
    }
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
    if ((_sv.corner1.x == _sv.corner2.x) ||
	(_sv.corner1.y == _sv.corner2.y) ||
	(_sv.corner1.z == _sv.corner2.z)) {
	return;				
    }
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
    if (!fieldPtr->isActivated()) {
	fieldPtr->activate();
    }
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
    if (!_fieldPtr->isActivated()) {
	_fieldPtr->activate();
    }
    _fieldPtr->render();
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = FirstParticles(&iter); particlesPtr != NULL; 
	 particlesPtr = NextParticles(&iter)) {
	if (particlesPtr->visible()) {
	    particlesPtr->Render();
	}
    }
    RenderBoxes();
}

void
FlowCmd::Init(void) 
{
    Tcl_InitHashTable(&_flowTable, TCL_STRING_KEYS);
}

FlowCmd *
FlowCmd::FirstFlow(FlowIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&_flowTable, &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
	return NULL;
    }
    return (FlowCmd *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowCmd *
FlowCmd::NextFlow(FlowIterator *iterPtr) 
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
FlowCmd::GetFlow(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowCmd **flowPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&_flowTable, Tcl_GetString(objPtr));
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
FlowCmd::CreateFlow(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    const char *name;
    name = Tcl_GetString(objPtr);
    hPtr = Tcl_CreateHashEntry(&_flowTable, name, &isNew);
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
    name = Tcl_GetHashKey(&_flowTable, hPtr);
    flowPtr = new FlowCmd(interp, name, hPtr);
    if (flowPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate a flow object \"", name, 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, flowPtr);
    EventuallyRedraw(MAP_PENDING);
    return TCL_OK;
}

void
FlowCmd::DeleteFlows(Tcl_Interp *interp)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	flowPtr->disconnect();		/* Don't disrupt the hash walk */
	Tcl_DeleteCommand(interp, flowPtr->name());
    }
    Tcl_DeleteHashTable(&_flowTable);
}

bool
FlowCmd::MapFlows(void)
{
    flags &= ~FlowCmd::MAP_PENDING;

    _xMin = _yMin = _zMin = _wMin = _magMin = FLT_MAX;
    _xMax = _yMax = _zMax = _wMax = _magMax = -FLT_MAX;

    /* 
     * Step 1.  Get the overall min and max magnitudes of all the 
     *		flow vectors.
     */
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	double minMag, maxMag;
	if (!flowPtr->isDataLoaded()) {
	    continue;
	}
	flowPtr->GetMagRange(minMag, maxMag);
	if (minMag < _magMin) {
	    _magMin = minMag;
	} 
	if (maxMag > _magMax) {
	    _magMax = maxMag;
	}
	Rappture::Unirect3d *dataPtr;
	dataPtr = flowPtr->GetData();
	if (dataPtr->xMin() < _xMin) {
	    _xMin = dataPtr->xMin();
	}
	if (dataPtr->yMin() < _yMin) {
	    _yMin = dataPtr->yMin();
	}
	if (dataPtr->zMin() < _zMin) {
	    _zMin = dataPtr->zMin();
	}
	if (dataPtr->xMax() > _xMax) {
	    _xMax = dataPtr->xMax();
	}
	if (dataPtr->yMax() > _yMax) {
	    _yMax = dataPtr->yMax();
	}
	if (dataPtr->zMax() > _zMax) {
	    _zMax = dataPtr->zMax();
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
	if (!flowPtr->ScaleVectorField()) {
	    return false;
	}
	flowPtr->InitializeParticles();
	NanoVis::licRenderer->set_offset(flowPtr->GetPosition());
    }
    return true;
}

void
FlowCmd::RenderFlows(void)
{
    if (!NanoVis::licRenderer->isActivated()) {
	NanoVis::licRenderer->activate();
    }
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
	    flowPtr->Render();
	}
    }
    flags &= ~FlowCmd::REDRAW_PENDING;
}

void
FlowCmd::ResetFlows(void)
{
    if (!NanoVis::licRenderer->isActivated()) {
	NanoVis::licRenderer->activate();
    }
    /*NanoVis::licRenderer->convolve();*/

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
FlowCmd::AdvectFlows(void)
{
    if (!NanoVis::licRenderer->isActivated()) {
	NanoVis::licRenderer->activate();
    }
    /*NanoVis::licRenderer->convolve();*/

    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = NextFlow(&iter)) {
	if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
	    flowPtr->Advect();
	}
    }
}    


#ifdef notdef
int
FlowCmd::InitVectorField(Tcl_Interp *interp) 
{
    if (NanoVis::flowVisRenderer == NULL) {
	Tcl_AppendResult(interp, "flowvis renderer is NULL", (char *)NULL);
	return TCL_ERROR;
    }
    if (NanoVis::licRenderer == NULL) {
	Tcl_AppendResult(interp, "LIC renderer is NULL", (char *)NULL);
	return TCL_ERROR;
    }
    _fieldPtr->setVectorField(_volPtr->id, *(_volPtr->get_location()),
	1.0f, _volPtr->height / (float)_volPtr->width,
	_volPtr->depth  / (float)_volPtr->width,
	_volPtr->wAxis.max());

    NanoVis::initParticle();
    NanoVis::licRenderer->setVectorField(
	_volPtr->id,			/* Texture ID */
	*(_volPtr->get_location()),	/* Origin */
	1.0f / volPtr->aspect_ratio_width, /* X-axis scale. */
	1.0f / volPtr->aspect_ratio_height, /* Y-axis scale. */
	1.0f / volPtr->aspect_ratio_depth, /* Z-axis scale. */ 
	volPtr->wAxis.max());		/* Maximum ???? */

    NanoVis::licRenderer->set_offset(NanoVis::lic_slice_z);
    return TCL_OK;
}
#endif

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
FlowCmd::GetMagRange(double &min_mag, double &max_mag)
{
    const float *values = _dataPtr->values();
    size_t nValues = _dataPtr->nValues();
    max_mag = -1e21, min_mag = 1e21;
    for (size_t i = 0; i < nValues; i += 3) {
	double vx, vy, vz, vm;

	vx = values[i];
	vy = values[i+1];
	vz = values[i+2];
		    
	vm = sqrt(vx*vx + vy*vy + vz*vz);
	if (vm > max_mag) {
	    max_mag = vm;
	}
	if (vm < min_mag) {
	    min_mag = vm;
	}
    }
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
    width  = _xMax - _xMin;
    height = _yMax - _yMin;
    depth  = _zMax - _zMin;

    Vector3 *locationPtr = _volPtr->get_location();
    /*This is wrong. Need to compute origin. */
    _xOrigin = locationPtr->x;
    _yOrigin = locationPtr->y;
    _zOrigin = locationPtr->z;

    _fieldPtr->setVectorField(_volPtr, *locationPtr, 
	1.0f, height / width, depth  / width, _magMax);

    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->setVectorField(_volPtr->id, 
		*locationPtr,
		1.0f / _volPtr->aspect_ratio_width,
		1.0f / _volPtr->aspect_ratio_height,
		1.0f / _volPtr->aspect_ratio_depth,
		_volPtr->wAxis.max());
	SetCurrentPosition();
	SetAxis();
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
		destPtr[0] = vm / _magMax; 
		destPtr[1] = vx /(2.0*_magMax) + 0.5; 
		destPtr[2] = vy /(2.0*_magMax) + 0.5; 
		destPtr[3] = vz /(2.0*_magMax) + 0.5; 
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
    }
    Volume *volPtr;
    volPtr = NanoVis::load_volume(_volIndex, _dataPtr->xNum(), 
	_dataPtr->yNum(), _dataPtr->zNum(), 4, data, _magMin, _magMax, 0);
    volPtr->xAxis.SetRange(_dataPtr->xMin(), _dataPtr->xMax());
    volPtr->yAxis.SetRange(_dataPtr->yMin(), _dataPtr->yMax());
    volPtr->zAxis.SetRange(_dataPtr->zMin(), _dataPtr->zMax());
    volPtr->wAxis.SetRange(_magMin, _magMax);

    /*volPtr->update_pending = false;*/
    Vector3 physicalMin(_xMin, _yMin, _zMin);
    Vector3 physicalMax(_xMax, _yMax, _zMax);
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

    int extents;
    if (Tcl_GetIntFromObj(interp, objv[4], &extents) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((extents < 1) || (extents > 4)) {
	Tcl_AppendResult(interp, "bad extents value \"", Tcl_GetString(objv[4]),
			 "\"", (char *)NULL);
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

	dataPtr = new Rappture::Unirect3d(extents);
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if (strncmp(buf.bytes(), "<unirect3d>", 4) == 0) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
	dataPtr = new Rappture::Unirect3d(extents);
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
	flowPtr->SetData(dataPtr);
    } else {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(extents);
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
					   (char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    }
    FlowCmd::EventuallyRedraw(FlowCmd::MAP_PENDING);
    return TCL_OK;
}

static int
FlowDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

    Trace("Flow Data Loading\n");

    int nBytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    if (nBytes <= 0) {
	Tcl_AppendResult(interp, "bad # bytes request \"", 
		Tcl_GetString(objv[3]), "\" for \"data follows\"", (char *)NULL);
	return TCL_ERROR;
    }
    int extents;
    if (Tcl_GetIntFromObj(interp, objv[4], &extents) != TCL_OK) {
        return TCL_ERROR;
    }
    Rappture::Buffer buf;
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    if ((buf.size() > 4) && (strncmp(buf.bytes(), "<DX>", 4) == 0)) {
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(extents);
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
		(char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    } else if ((buf.size() > 4) && 
	       (strncmp(buf.bytes(), "<unirect3d>", 4) == 0)) {
	Rappture::Unirect3d *dataPtr;
	Tcl_CmdInfo cmdInfo;

	/* Set the clientdata field of the unirect3d command to contain
	 * the local data structure. */
	dataPtr = new Rappture::Unirect3d(extents);
	if (!Tcl_GetCommandInfo(interp, "unirect3d", &cmdInfo)) {
	    return TCL_ERROR;
	}
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
	flowPtr->SetData(dataPtr);
    } else {
	Tcl_AppendResult(interp, "unknown data header \"", /*buf.bytes(), "\"",*/
			 (char *)NULL);
	Rappture::Unirect3d *dataPtr;

	dataPtr = new Rappture::Unirect3d(extents);
	if (!dataPtr->ReadVectorDataFromDx(result, buf.size(), 
					   (char *)buf.bytes())) {
	    Tcl_AppendResult(interp, result.remark(), (char *)NULL);
	    delete dataPtr;
	    return TCL_ERROR;
	}
	flowPtr->SetData(dataPtr);
    }
    FlowCmd::EventuallyRedraw(FlowCmd::MAP_PENDING);
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


static int
FlowVectorIdOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
#ifdef notdef
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
#endif
    return TCL_OK;
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
	fprintf(stderr, "got absolute value %s\n", string);
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
    FlowCmd::EventuallyRedraw();
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
    FlowCmd::EventuallyRedraw(FlowCmd::MAP_PENDING);
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
    FlowCmd::EventuallyRedraw();
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
    FlowCmd::EventuallyRedraw(FlowCmd::MAP_PENDING);
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
    FlowCmd::EventuallyRedraw();
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
    FlowCmd::EventuallyRedraw();
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
    FlowCmd::EventuallyRedraw();
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
    if (FlowCmd::CreateFlow(interp, objv[2]) != TCL_OK) {
	return TCL_ERROR;
    }
    FlowCmd *flowPtr;
    if (FlowCmd::GetFlow(interp, objv[2], &flowPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (flowPtr->ParseSwitches(interp, objc - 3, objv + 3) != TCL_OK) {
	Tcl_DeleteCommand(interp, flowPtr->name());
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[2]);
    FlowCmd::EventuallyRedraw();
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

	if (FlowCmd::GetFlow(interp, objv[i], &flowPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DeleteCommand(interp, flowPtr->name());
    }
    FlowCmd::EventuallyRedraw(FlowCmd::MAP_PENDING);
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
    for (flowPtr = FlowCmd::FirstFlow(&iter); flowPtr != NULL; 
	 flowPtr = FlowCmd::NextFlow(&iter)) {
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
    if (!NanoVis::licRenderer->isActivated()) {
        NanoVis::licRenderer->activate();
    }
    Trace("sending flow playback frame\n");
    // Generate the latest frame and send it back to the client
    if (FlowCmd::flags & FlowCmd::MAP_PENDING) {
	FlowCmd::MapFlows();
    }
    FlowCmd::EventuallyRedraw();
    NanoVis::licRenderer->convolve();
    FlowCmd::AdvectFlows();
    NanoVis::offscreen_buffer_capture();  //enable offscreen render
    NanoVis::display();
    NanoVis::read_screen();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    static int frame_count = 0;
    NanoVis::bmp_write_to_file(++frame_count, ".");
    Trace("FLOW end\n");
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    FlowCmd::ResetFlows();
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
	FlowCmd::RenderFlows();
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
    {"add",      1, FlowAddOp,     3, 0, "name ?option value...?",},
    {"delete",   1, FlowDeleteOp,  2, 0, "name...",},
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
    FlowCmd::Init();
    return TCL_OK;
}


float
FlowCmd::GetPosition(FlowPosition *posPtr)
{
    if (posPtr->flags == RELPOS) {
	return posPtr->value;
    }
    switch (posPtr->axis) {
    case AXIS_X:  
	return (posPtr->value - _xMin) / (_xMax - _xMin); 
    case AXIS_Y:  
	return (posPtr->value - _yMin) / (_yMax - _yMin); 
    case AXIS_Z:  
	return (posPtr->value - _zMin) / (_zMax - _zMin); 
    }
    return 0.0;
}

float
FlowCmd::GetPosition(void) 
{
    return FlowCmd::GetPosition(&_sv.slicePos);
}
