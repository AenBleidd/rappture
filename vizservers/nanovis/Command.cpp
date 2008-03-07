
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

#include <tcl.h>
#include "Trace.h"
#include "Command.h"
#include "nanovis.h"
#include "CmdProc.h"

#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"
#include "RpEncode.h"

#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"

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
#include <RenderContext.h>
#include <NvLIC.h>

#define ISO_TEST                1
#define PLANE_CMD               0
#define __TEST_CODE__           0
// FOR testing new functions
#define _LOCAL_ZINC_TEST_       0

#if _LOCAL_ZINC_TEST_
#include "Test.h"
#endif

// EXTERN DECLARATIONS
// in Nv.cpp

extern Grid* NanoVis::grid;

// in nanovis.cpp
extern vector<PointSet*> g_pointSet;

extern PlaneRenderer* plane_render;
extern Texture2D* plane[10];

extern Rappture::Outcome load_volume_stream(int index, std::iostream& fin);
extern Rappture::Outcome load_volume_stream_odx(int index, const char *buf, 
						int nBytes);
extern Rappture::Outcome load_volume_stream2(int index, std::iostream& fin);
extern void load_volume(int index, int width, int height, int depth, 
			int n_component, float* data, double vmin, double vmax, 
			double nzero_min);
extern void load_vector_stream(int index, std::istream& fin);

// Tcl interpreter for incoming messages
static Tcl_Interp *interp;
static Tcl_DString cmdbuffer;

// default transfer function
static const char def_transfunc[] = 
    "transfunc define default {\n\
  0.0  1 1 1\n\
  0.2  1 1 0\n\
  0.4  0 1 0\n\
  0.6  0 1 1\n\
  0.8  0 0 1\n\
  1.0  1 0 1\n\
} {\n\
  0.00  1.0\n\
  0.05  0.0\n\
  0.15  0.0\n\
  0.20  1.0\n\
  0.25  0.0\n\
  0.35  0.0\n\
  0.40  1.0\n\
  0.45  0.0\n\
  0.55  0.0\n\
  0.60  1.0\n\
  0.65  0.0\n\
  0.75  0.0\n\
  0.80  1.0\n\
  0.85  0.0\n\
  0.95  0.0\n\
  1.00  1.0\n\
}";

static Tcl_ObjCmdProc AxisCmd;
static Tcl_ObjCmdProc CameraCmd;
static Tcl_ObjCmdProc CutplaneCmd;
static Tcl_ObjCmdProc FlowCmd;
static Tcl_ObjCmdProc GridCmd;
static Tcl_ObjCmdProc LegendCmd;
#if PLANE_CMD
static Tcl_ObjCmdProc PlaneCmd;
#endif
static Tcl_ObjCmdProc ScreenCmd;
static Tcl_ObjCmdProc ScreenShotCmd;
static Tcl_ObjCmdProc TransfuncCmd;
static Tcl_ObjCmdProc UniRect2dCmd;
static Tcl_ObjCmdProc UpCmd;
static Tcl_ObjCmdProc VolumeCmd;

static bool
GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, bool *boolPtr) 
{
    int value;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *boolPtr = (bool)value;
    return TCL_OK;
}

static int
GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, float *valuePtr)
{
    double value;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *valuePtr = (float)value;
    return TCL_OK;
}

static int
GetCullMode(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	    graphics::RenderContext::CullMode *modePtr)
{
    char *string = Tcl_GetString(objPtr);
    if (strcmp(string, "none") == 0) {
        *modePtr = graphics::RenderContext::NO_CULL;
    } else if (strcmp(string, "front") == 0) {
        *modePtr = graphics::RenderContext::FRONT;
    } else if (strcmp(string, "back") == 0) {
        *modePtr = graphics::RenderContext::BACK;
    } else {
        Tcl_AppendResult(interp, "invalid cull mode \"", string, 
			 "\": should be front, back, or none\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetShadingModel(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                graphics::RenderContext::ShadingModel *modelPtr)
{
    char *string = Tcl_GetString(objPtr);
    if (strcmp(string,"flat") == 0) {
        *modelPtr = graphics::RenderContext::FLAT;
    } else if (strcmp(string,"smooth") == 0) {
        *modelPtr = graphics::RenderContext::SMOOTH;
    } else {
        Tcl_AppendResult(interp, "bad shading model \"", string, 
                         "\": should be flat or smooth", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetPolygonMode(Tcl_Interp *interp, Tcl_Obj *objPtr, 
               graphics::RenderContext::PolygonMode *modePtr)
{
    char *string = Tcl_GetString(objPtr);
    if (strcmp(string,"wireframe") == 0) {
        *modePtr = graphics::RenderContext::LINE;
    } else if (strcmp(string,"fill") == 0) {
        *modePtr = graphics::RenderContext::FILL;
    } else {
        Tcl_AppendResult(interp, "invalid polygon mode \"", string, 
			 "\": should be wireframe or fill\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}


static int 
GetVolumeLimits(Tcl_Interp *interp, AxisRange *range) 
{

    /* Find the first volume. */
    int start;
    for (start = 0; start < NanoVis::n_volumes; start++) {
        if (NanoVis::volume[start] != NULL) {
            break;
        }
    }
    if (start == NanoVis::n_volumes) {
        Tcl_AppendResult(interp, "no volumes found", (char *)NULL);
        return TCL_ERROR;
    }
    int axis;
    for (axis = AxisRange::X; axis <= AxisRange::VALUES; axis++) {
	Volume *volPtr;
	int i;

	i = start;
	volPtr = NanoVis::volume[i];
	range[axis] = *volPtr->GetRange(axis);
	for (i++; i < NanoVis::n_volumes; i++) {
	    const AxisRange *rangePtr;

	    volPtr = NanoVis::volume[i];
	    rangePtr = volPtr->GetRange(axis);
	    if (range[axis].max < rangePtr->max) {
		range[axis].max = rangePtr->max;
	    }
	    if (range[axis].min > rangePtr->min) {
		range[axis].min = rangePtr->min;
	    }
	}
    }
    return TCL_OK;
}


/*
 * -----------------------------------------------------------------------
 *
 * CreateHeightMap --
 *
 *      Creates a heightmap from the given the data. The format of the data
 *      should be as follows:
 *
 *              xMin, xMax, xNum, yMin, yMax, yNum, heights...
 *
 *      xNum and yNum must be integer values, all others are real numbers.
 *      The number of heights must be xNum * yNum;
 *
 * -----------------------------------------------------------------------
 */
static HeightMap *
CreateHeightMap(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *CONST *objv)
{
    float xMin, yMin, xMax, yMax;
    int xNum, yNum;

    if (objc != 7) {
        Tcl_AppendResult(interp, 
			 "wrong # of values: should be xMin yMin xMax yMax xNum yNum heights",
			 (char *)NULL);
        return NULL;
    }
    if ((GetFloatFromObj(interp, objv[0], &xMin) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[1], &yMin) != TCL_OK) || 
        (GetFloatFromObj(interp, objv[2], &xMax) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &yMax) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &xNum) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &yNum) != TCL_OK)) {
        return NULL;
    }
    int nValues;
    Tcl_Obj **elem;
    if (Tcl_ListObjGetElements(interp, objv[6], &nValues, &elem) != TCL_OK) {
        return NULL;
    }
    if ((xNum <= 0) || (yNum <= 0)) {
        Tcl_AppendResult(interp, "bad number of x or y values", (char *)NULL);
        return NULL;
    }
    if (nValues != (xNum * yNum)) {
        Tcl_AppendResult(interp, "wrong # of heights", (char *)NULL);
        return NULL;
    }

    float *heights;
    heights = new float[nValues];
    if (heights == NULL) {
        Tcl_AppendResult(interp, "can't allocate array of heights", 
			 (char *)NULL);
        return NULL;
    }

    int i;
    for (i = 0; i < nValues; i++) {
        if (GetFloatFromObj(interp, elem[i], heights + i) != TCL_OK) {
            delete [] heights;
            return NULL;
        }
    }
    HeightMap* hMap;
    hMap = new HeightMap();
    hMap->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, heights);
    hMap->setColorMap(NanoVis::get_transfunc("default"));
    hMap->setVisible(true);
    hMap->setLineContourVisible(true);
    delete [] heights;
    return hMap;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetHeightMap
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetHeightMap(Tcl_Interp *interp, Tcl_Obj *objPtr, HeightMap **hmPtrPtr)
{
    int mapIndex;
    if (Tcl_GetIntFromObj(interp, objPtr, &mapIndex) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((mapIndex < 0) || (mapIndex >= (int)NanoVis::heightMap.size()) || 
        (NanoVis::heightMap[mapIndex] == NULL)) {
        Tcl_AppendResult(interp, "invalid heightmap index \"", 
			 Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    *hmPtrPtr = NanoVis::heightMap[mapIndex];
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumeIndex
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumeIndex(Tcl_Interp *interp, Tcl_Obj *objPtr, unsigned int *indexPtr)
{
    int index;
    if (Tcl_GetIntFromObj(interp, objPtr, &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index < 0) {
        Tcl_AppendResult(interp, "can't have negative index \"", 
			 Tcl_GetString(objPtr), "\"", (char*)NULL);
        return TCL_ERROR;
    }
    if (index >= (int)NanoVis::volume.size()) {
        Tcl_AppendResult(interp, "index \"", Tcl_GetString(objPtr),
                         "\" is out of range", (char*)NULL);
        return TCL_ERROR;
    }
    *indexPtr = (unsigned int)index;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumeFromObj
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Volume **volPtrPtr)
{
    unsigned int index;
    if (GetVolumeIndex(interp, objPtr, &index) != TCL_OK) {
        return TCL_ERROR;
    }
    Volume *vol;
    vol = NanoVis::volume[index];
    if (vol == NULL) {
        Tcl_AppendResult(interp, "no volume defined for index \"", 
			 Tcl_GetString(objPtr), "\"", (char*)NULL);
        return TCL_ERROR;
    }
    *volPtrPtr = vol;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumeIndices()
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumeIndices(Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv,
		 vector<unsigned int>* vectorPtr)
{
    if (objc == 0) {
        for (unsigned int n = 0; n < NanoVis::volume.size(); n++) {
            if (NanoVis::volume[n] != NULL) {
                vectorPtr->push_back(n);
            }
        }
    } else {
        for (int n = 0; n < objc; n++) {
            unsigned int index;

            if (GetVolumeIndex(interp, objv[n], &index) != TCL_OK) {
                return TCL_ERROR;
            }
            if (index < NanoVis::volume.size() && NanoVis::volume[index] != NULL) {
                vectorPtr->push_back(index);
            }
        }
    }
    return TCL_OK;
}

static int
GetIndices(Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv,
	   vector<unsigned int>* vectorPtr)
{
    for (int n = 0; n < objc; n++) {
        int index;

        if (Tcl_GetIntFromObj(interp, objv[n], &index) != TCL_OK) {
            return TCL_ERROR;
        }
        if (index < 0) {
            Tcl_AppendResult(interp, "can't have negative index \"", 
			     Tcl_GetString(objv[n]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        vectorPtr->push_back((unsigned int)index);
    }
    return TCL_OK;
}


/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumes()
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumes(Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv,
	   vector<Volume *>* vectorPtr)
{
    if (objc == 0) {
        for (unsigned int n = 0; n < NanoVis::volume.size(); n++) {
            if (NanoVis::volume[n] != NULL) {
                vectorPtr->push_back(NanoVis::volume[n]);
            }
        }
    } else {
        for (int n = 0; n < objc; n++) {
            Volume *volPtr;

            if (GetVolumeFromObj(interp, objv[n], &volPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            if (volPtr != NULL) {
                vectorPtr->push_back(volPtr);
            }
        }
    }
    return TCL_OK;
}


/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetAxis()
 *
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in valPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetAxis(Tcl_Interp *interp, const char *string, int *indexPtr)
{
    if (string[1] == '\0') {
        char c;

        c = tolower((unsigned char)string[0]);
        if (c == 'x') {
            *indexPtr = AxisRange::X;
            return TCL_OK;
        } else if (c == 'y') {
            *indexPtr = AxisRange::Y;
            return TCL_OK;
        } else if (c == 'z') {
            *indexPtr = AxisRange::Z;
            return TCL_OK;
        }
        /*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
		     "\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetAxisFromObj()
 *
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in indexPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr)
{
    return GetAxis(interp, Tcl_GetString(objPtr), indexPtr);
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetAxisDirFromObj()
 *
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in indexPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetAxisDirFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr, int *dirPtr)
{
    char *string = Tcl_GetString(objPtr);

    int sign = 1;
    if (*string == '-') {
        sign = -1;
        string++;
    }
    if (GetAxis(interp, string, indexPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (dirPtr != NULL) {
        *dirPtr = sign;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetColor()
 *
 * Used internally to decode a color value from a string ("R G B")
 * as a list of three numbers 0-1.  Returns TCL_OK if successful,
 * along with RGB values in rgbPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetColor(Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv, float *rgbPtr)
{
    if (objc < 3) {
        Tcl_AppendResult(interp, "missing color values\": ",
			 "should list of R G B values 0.0 - 1.0", (char*)NULL);
        return TCL_ERROR;
    }
    if ((GetFloatFromObj(interp, objv[0], rgbPtr + 0) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[1], rgbPtr + 1) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[2], rgbPtr + 2) != TCL_OK)) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * -----------------------------------------------------------------------
 *
 * GetDataStream -- 
 *
 *      Read the requested number of bytes from standard input into the given
 *      buffer.  The buffer is then decompressed and decoded.
 *
 * -----------------------------------------------------------------------
 */
static int
GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes)
{
    char buffer[8096];

    clearerr(stdin);
    while (nBytes > 0) {
        unsigned int chunk;
        int nRead;

        chunk = (sizeof(buffer) < (unsigned int) nBytes) ? 
            sizeof(buffer) : nBytes;
        nRead = fread(buffer, sizeof(char), chunk, stdin);
        if (ferror(stdin)) {
            Tcl_AppendResult(interp, "while reading data stream: ",
			     Tcl_PosixError(interp), (char*)NULL);
            return TCL_ERROR;
        }
        if (feof(stdin)) {
            Tcl_AppendResult(interp, "premature EOF while reading data stream",
			     (char*)NULL);
            return TCL_ERROR;
        }
        buf.append(buffer, nRead);
        nBytes -= nRead;
    }
    {
        Rappture::Outcome err;

        err = Rappture::encoding::decode(buf, RPENC_Z|RPENC_B64|RPENC_HDR);
        if (err) {
            printf("ERROR -- DECODING\n");
            fflush(stdout);
            Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
CameraAimOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    double x0, y0, z0;
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &x0) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &y0) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[4], &z0) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::cam->aim(x0, y0, z0);
    return TCL_OK;
}

static int
CameraAngleOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    double xangle, yangle, zangle;
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &xangle) != TCL_OK) || 
        (Tcl_GetDoubleFromObj(interp, objv[3], &yangle) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[4], &zangle) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::cam->rotate(xangle, yangle, zangle);
    return TCL_OK;
}

static int
CameraZoomOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    double zoom;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &zoom) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::zoom(zoom);
    return TCL_OK;
}

static Rappture::CmdSpec cameraOps[] = {
    {"aim",     2, CameraAimOp,      5, 5, "x y z",},
    {"angle",   2, CameraAngleOp,    5, 5, "xAngle yAngle zAngle",},
    {"zoom",    1, CameraZoomOp,     3, 3, "factor",},
};
static int nCameraOps = NumCmdSpecs(cameraOps);

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   camera aim <x0> <y0> <z0>
 *   camera angle <xAngle> <yAngle> <zAngle>
 *   camera zoom <factor>
 *
 * Clients send these commands to manipulate the camera.  The "angle"
 * option controls the angle of the camera around the focal point.
 * The "zoom" option sets the zoom factor, moving the camera in
 * and out.
 * ----------------------------------------------------------------------
 */
static int
CameraCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCameraOps, cameraOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
ScreenShotCmd(ClientData cdata, Tcl_Interp *interp, int objc, 
              Tcl_Obj *CONST *objv)
{
#ifdef XINETD
    NanoVis::resize_offscreen_buffer(1024, 1024);
    NanoVis::cam->set_screen_size(30, 90, 1024 - 60, 1024 - 120);
    NanoVis::offscreen_buffer_capture();  //enable offscreen render
    NanoVis::display();

    // INSOO
    // TBD
    /*
      Volume* vol = NanoVis::volume[0];
      TransferFunction* tf;
      tf = NanoVis::vol_renderer->get_volume_shading(vol);
      if (tf != NULL) 
      {
      float data[512];

      for (int i=0; i < 256; i++) {
      data[i] = data[i+256] = (float)(i/255.0);
      }
      Texture2D* plane = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
      AxisRange *rangePtr;
      rangePtr = vol->GetRange(AxisRange::VALUES);
      NanoVis::color_table_renderer->render(1024, 1024, plane, tf, rangePtr->min,
		rangePtr->max);
      delete plane;
    
      }
    */
#endif

    return TCL_OK;
}

static int
CutplanePositionOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *CONST *objv)
{
    float relval;
    if (GetFloatFromObj(interp, objv[2], &relval) != TCL_OK) {
        return TCL_ERROR;
    }
    
    // keep this just inside the volume so it doesn't disappear
    if (relval < 0.01f) { 
        relval = 0.01f; 
    } else if (relval > 0.99f) { 
        relval = 0.99f; 
    }
    
    int axis;
    if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
        return TCL_ERROR;
    }
    
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->move_cutplane(axis, relval);
    }
    return TCL_OK;
}

static int
CutplaneStateOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                Tcl_Obj *CONST *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    
    int axis;
    if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
        return TCL_ERROR;
    }
    
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    if (state) {
        vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->enable_cutplane(axis);
        } 
    } else {
        vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->disable_cutplane(axis);
        } 
    }
    return TCL_OK;
}

static Rappture::CmdSpec cutplaneOps[] = {
    {"position", 1, CutplanePositionOp, 4, 0, "bool axis ?indices?",},
    {"state",    1, CutplaneStateOp,    4, 0, "relval axis ?indices?",},
};
static int nCutplaneOps = NumCmdSpecs(cutplaneOps);

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   cutplane state on|off <axis> ?<volume>...?
 *   cutplane position <relvalue> <axis> ?<volume>...?
 *
 * Clients send these commands to manipulate the cutplanes in one or
 * more data volumes.  The "state" command turns a cutplane on or
 * off.  The "position" command changes the position to a relative
 * value in the range 0-1.  The <axis> can be x, y, or z.  These
 * options are applied to the volumes represented by one or more
 * <volume> indices.  If no volumes are specified, then all volumes
 * are updated.
 * ----------------------------------------------------------------------
 */
static int
CutplaneCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCutplaneOps, cutplaneOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   legend <volumeIndex> <width> <height>
 *
 * Clients use this to generate a legend image for the specified
 * transfer function.  The legend image is a color gradient from 0
 * to one, drawn in the given transfer function.  The resulting image
 * is returned in the size <width> x <height>.
 * ----------------------------------------------------------------------
 */
static int
LegendCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " volIndex width height\"", (char*)NULL);
        return TCL_ERROR;
    }

    Volume *volPtr;
    if (GetVolumeFromObj(interp, objv[1], &volPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    TransferFunction *tf;
    tf = NanoVis::vol_renderer->get_volume_shading(volPtr);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "no transfer function defined for volume \"", 
			 Tcl_GetString(objv[1]), "\"", (char*)NULL);
        return TCL_ERROR;
    }
    char *label;
    label = Tcl_GetString(objv[1]);

    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    AxisRange range[4];
    if (GetVolumeLimits(interp, range) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::render_legend(tf, 
			   range[AxisRange::VALUES].min, 
			   range[AxisRange::VALUES].max, 
			   w, h, label);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   screen <width> <height>
 *
 * Clients send this command to set the size of the rendering area.
 * Future images are generated at the specified width/height.
 * ----------------------------------------------------------------------
 */
static int
ScreenCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " width height\"", (char*)NULL);
        return TCL_ERROR;
    }

    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[1], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[2], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::resize_offscreen_buffer(w, h);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   transfunc define <name> <colormap> <alphamap>
 *     where <colormap> = { <v> <r> <g> <b> ... }
 *           <alphamap> = { <v> <w> ... }
 *
 * Clients send these commands to manipulate the transfer functions.
 * ----------------------------------------------------------------------
 */
static int
TransfuncCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }

    char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'd') && (strcmp(string, "define") == 0)) {
        if (objc != 5) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " define name colormap alphamap\"", 
			     (char*)NULL);
            return TCL_ERROR;
        }

        // decode the data and store in a series of fields
        Rappture::Field1D rFunc, gFunc, bFunc, wFunc;
        int cmapc, wmapc, i;
        Tcl_Obj **cmapv;
        Tcl_Obj **wmapv;

        wmapv = cmapv = NULL;
        if (Tcl_ListObjGetElements(interp, objv[3], &cmapc, &cmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((cmapc % 4) != 0) {
            Tcl_AppendResult(interp, "bad colormap in transfunc: should be ",
			     "{ v r g b ... }", (char*)NULL);
            return TCL_ERROR;
        }
        if (Tcl_ListObjGetElements(interp, objv[4], &wmapc, &wmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((wmapc % 2) != 0) {
            Tcl_AppendResult(interp, "wrong # elements in alphamap: should be ",
			     " { v w ... }", (char*)NULL);
            return TCL_ERROR;
        }
        for (i = 0; i < cmapc; i += 4) {
            int j;
            double vals[4];

            for (j=0; j < 4; j++) {
                if (Tcl_GetDoubleFromObj(interp, cmapv[i+j], &vals[j]) != TCL_OK) {
                    return TCL_ERROR;
                }
                if ((vals[j] < 0.0) || (vals[j] > 1.0)) {
                    Tcl_AppendResult(interp, "bad value \"", cmapv[i+j],
				     "\": should be in the range 0-1", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            rFunc.define(vals[0], vals[1]);
            gFunc.define(vals[0], vals[2]);
            bFunc.define(vals[0], vals[3]);
        }
        for (i=0; i < wmapc; i += 2) {
            double vals[2];
            int j;

            for (j=0; j < 2; j++) {
                if (Tcl_GetDoubleFromObj(interp, wmapv[i+j], &vals[j]) != TCL_OK) {
                    return TCL_ERROR;
                }
                if ((vals[j] < 0.0) || (vals[j] > 1.0)) {
                    Tcl_AppendResult(interp, "bad value \"", wmapv[i+j],
				     "\": should be in the range 0-1", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            wFunc.define(vals[0], vals[1]);
        }
        // sample the given function into discrete slots
        const int nslots = 256;
        float data[4*nslots];
        for (i=0; i < nslots; i++) {
            double xval = double(i)/(nslots-1);
            data[4*i]   = rFunc.value(xval);
            data[4*i+1] = gFunc.value(xval);
            data[4*i+2] = bFunc.value(xval);
            data[4*i+3] = wFunc.value(xval);
        }

        // find or create this transfer function
        TransferFunction *tf;
        tf = NanoVis::get_transfunc(Tcl_GetString(objv[2]));
        if (tf != NULL) {
            tf->update(data);
        } else {
            tf = NanoVis::set_transfunc(Tcl_GetString(objv[2]), nslots, data);
        }
    } else {
        Tcl_AppendResult(interp, "bad option \"", string,
			 "\": should be define", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   up axis
 *
 * Clients use this to set the "up" direction for all volumes.  Volumes
 * are oriented such that this direction points upward.
 * ----------------------------------------------------------------------
 */
static int
UpCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    if (objc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " x|y|z|-x|-y|-z\"", (char*)NULL);
        return TCL_ERROR;
    }

    int sign;
    int axis;
    if (GetAxisDirFromObj(interp, objv[1], &axis, &sign) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::updir = (axis+1)*sign;
    return TCL_OK;
}


static int
VolumeAnimationCaptureOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			 Tcl_Obj *CONST *objv)
{
    int total;
    if (Tcl_GetIntFromObj(interp, objv[3], &total) != TCL_OK) {
	return TCL_ERROR;
    }
    VolumeInterpolator* interpolator;
    interpolator = NanoVis::vol_renderer->getVolumeInterpolator(); 
    interpolator->start();
    if (interpolator->is_started()) {
	char *fileName = (objc < 5) ? NULL : Tcl_GetString(objv[4]);
	for (int frame_num = 0; frame_num < total; ++frame_num) {
	    float fraction;
	    
	    fraction = ((float)frame_num) / (total - 1);
	    Trace("fraction : %f\n", fraction);
	    //interpolator->update(((float)frame_num) / (total - 1));
	    interpolator->update(fraction);
	    
	    NanoVis::offscreen_buffer_capture();  //enable offscreen render
	    
	    NanoVis::display();
	    NanoVis::read_screen();
	    
	    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            
	    NanoVis::bmp_write_to_file(frame_num, fileName);
	}
    }
    return TCL_OK;
}

static int
VolumeAnimationClearOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *CONST *objv)
{
    NanoVis::vol_renderer->clearAnimatedVolumeInfo();
    return TCL_OK;
}

static int
VolumeAnimationStartOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *CONST *objv)
{
    NanoVis::vol_renderer->startVolumeAnimation();
    return TCL_OK;
}

static int
VolumeAnimationStopOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		      Tcl_Obj *CONST *objv)
{
    NanoVis::vol_renderer->stopVolumeAnimation();
    return TCL_OK;
}

static int
VolumeAnimationVolumesOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			 Tcl_Obj *CONST *objv)
{
    vector<unsigned int> ivol;
    if (GetVolumeIndices(interp, objc - 3, objv + 3, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    Trace("parsing volume index\n");
    vector<unsigned int>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	Trace("index: %d\n", *iter);
	NanoVis::vol_renderer->addAnimatedVolume(NanoVis::volume[*iter], *iter);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeAnimationOps[] = {
    {"capture",   2, VolumeAnimationCaptureOp,  4, 5, "numframes ?filename?",},
    {"clear",     2, VolumeAnimationClearOp,    3, 3, "",},
    {"start",     3, VolumeAnimationStartOp,    3, 3, "",},
    {"stop",      3, VolumeAnimationStopOp,     3, 3, "",},
    {"volumes",   1, VolumeAnimationVolumesOp,  3, 0, "?indices?",},
};

static int nVolumeAnimationOps = NumCmdSpecs(volumeAnimationOps);

static int
VolumeAnimationOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeAnimationOps, volumeAnimationOps, 
				  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}


static int
VolumeDataFollowsOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *CONST *objv)
{
    printf("Data Loading\n");
    fflush(stdout);
    
    int nbytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    
    Rappture::Buffer buf;
    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    int n = NanoVis::n_volumes;
    char header[6];
    memcpy(header, buf.bytes(), sizeof(char) * 5);
    header[5] = '\0';
    
#if _LOCAL_ZINC_TEST_
    //FILE* fp = fopen("/home/nanohub/vrinside/nv/data/HOON/QDWL_100_100_50_strain_8000i.nd_zatom_12_1", "rb");
    FILE* fp;
    
    fp = fopen("/home/nanohub/vrinside/nv/data/HOON/GaAs_AlGaAs_2QD_B4.nd_zc_1_wf", "rb");
    if (fp == NULL) {
        printf("cannot open the file\n");
        fflush(stdout);
        return TCL_ERROR;
    }
    unsigned char* b = (unsigned char*)malloc(buf.size());
    fread(b, buf.size(), 1, fp);
    fclose(fp);
#endif  /*_LOCAL_ZINC_TEST_*/
    printf("Checking header[%s]\n", header);
    fflush(stdout);
    if (strcmp(header, "<HDR>") == 0) {
        Volume* vol = NULL;
        
        printf("ZincBlende stream is in\n");
        fflush(stdout);
        //std::stringstream fdata(std::ios_base::out|std::ios_base::in|std::ios_base::binary);
        //fdata.write(buf.bytes(),buf.size());
        //vol = NvZincBlendeReconstructor::getInstance()->loadFromStream(fdata);
        
#if _LOCAL_ZINC_TEST_
        vol = NvZincBlendeReconstructor::getInstance()->loadFromMemory(b);
#else
        vol = NvZincBlendeReconstructor::getInstance()->loadFromMemory((void*) buf.bytes());
#endif  /*_LOCAL_ZINC_TEST_*/
        if (vol == NULL) {
            Tcl_AppendResult(interp, "can't get volume instance", (char *)NULL);
            return TCL_OK;
        }
        printf("finish loading\n");
        fflush(stdout);
        while (NanoVis::n_volumes <= n) {
            NanoVis::volume.push_back((Volume*) NULL);
            NanoVis::n_volumes++;
        }
        
        if (NanoVis::volume[n] != NULL) {
            delete NanoVis::volume[n];
            NanoVis::volume[n] = NULL;
        }
        
        float dx0 = -0.5;
        float dy0 = -0.5*vol->height/vol->width;
        float dz0 = -0.5*vol->depth/vol->width;
        vol->move(Vector3(dx0, dy0, dz0));
        
        NanoVis::volume[n] = vol;
#if __TEST_CODE__
    } else if (strcmp(header, "<FET>") == 0) {
        printf("FET loading...\n");
        fflush(stdout);
        std::stringstream fdata;
        fdata.write(buf.bytes(),buf.size());
        err = load_volume_stream3(n, fdata);
        if (err) {
            Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
            return TCL_ERROR;
        }
#endif  /*__TEST_CODE__*/
    } else if (strcmp(header, "<ODX>") == 0) {
        Rappture::Outcome err;
        
        printf("Loading DX using OpenDX library...\n");
        fflush(stdout);
        //err = load_volume_stream_odx(n, buf.bytes()+5, buf.size()-5);
        //err = load_volume_stream2(n, fdata);
        if (err) {
            Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        Rappture::Outcome err;
        
        printf("OpenDX loading...\n");
        fflush(stdout);
        std::stringstream fdata;
        fdata.write(buf.bytes(),buf.size());
        
#if ISO_TEST
        err = load_volume_stream2(n, fdata);
#else
        err = load_volume_stream(n, fdata);
#endif
        if (err) {
            Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
            return TCL_ERROR;
        }
    }
    
    //
    // BE CAREFUL: Set the number of slices to something slightly different
    // for each volume.  If we have identical volumes at exactly the same
    // position with exactly the same number of slices, the second volume will
    // overwrite the first, so the first won't appear at all.
    //
    if (NanoVis::volume[n] != NULL) {
        NanoVis::volume[n]->set_n_slice(256-n);
        NanoVis::volume[n]->disable_cutplane(0);
        NanoVis::volume[n]->disable_cutplane(1);
        NanoVis::volume[n]->disable_cutplane(2);
        
        NanoVis::vol_renderer->add_volume(NanoVis::volume[n],
                                          NanoVis::get_transfunc("default"));
    }
    
    {
        Volume *volPtr;
        char info[1024];
        AxisRange overall[4];
        
        if (GetVolumeLimits(interp, overall) != TCL_OK) {
            return TCL_ERROR;
        }
        volPtr = NanoVis::volume[n];
	const AxisRange *rangePtr;
	rangePtr = volPtr->GetRange(AxisRange::VALUES);
        sprintf(info, "nv>data id %d min %g max %g vmin %g vmax %g\n", 
                n, rangePtr->min, rangePtr->max,
		overall[AxisRange::VALUES].min, 
		overall[AxisRange::VALUES].max);
        write(0, info, strlen(info));
    }
    return TCL_OK;
}

static int
VolumeDataStateOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *CONST *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    if (state) {
        vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->enable_data();
        }
    } else {
        vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->disable_data();
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeDataOps[] = {
    {"follows",   1, VolumeDataFollowsOp, 4, 4, "size",},
    {"state",     1, VolumeDataStateOp,   4, 0, "bool ?indices?",},
};
static int nVolumeDataOps = NumCmdSpecs(volumeDataOps);

static int
VolumeDataOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeDataOps, volumeDataOps, 
				  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
VolumeOutlineColorOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		     Tcl_Obj *CONST *objv)
{
    float rgb[3];
    if (GetColor(interp, objc - 3, objv + 3, rgb) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 6, objv + 6, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->set_outline_color(rgb);
    }
    return TCL_OK;
}

static int
VolumeOutlineStateOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		     Tcl_Obj *CONST *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    if (state) {
	vector<Volume *>::iterator iter;
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	    (*iter)->enable_outline();
	}
    } else {
	vector<Volume *>::iterator iter;
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	    (*iter)->disable_outline();
	}
    }
    return TCL_OK;
}


static Rappture::CmdSpec volumeOutlineOps[] = {
    {"color",     1, VolumeOutlineColorOp,  6, 0, "r g b ?indices?",},
    {"state",     1, VolumeOutlineStateOp,  4, 0, "bool ?indices?",},
    {"visible",   1, VolumeOutlineStateOp,  4, 0, "bool ?indices?",},
};
static int nVolumeOutlineOps = NumCmdSpecs(volumeOutlineOps);

static int
VolumeOutlineOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeOutlineOps, volumeOutlineOps, 
				  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
VolumeShadingDiffuseOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *CONST *objv)
{
    float diffuse;
    if (GetFloatFromObj(interp, objv[3], &diffuse) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->set_diffuse(diffuse);
    }
    return TCL_OK;
}

static int
VolumeShadingIsosurfaceOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			  Tcl_Obj *CONST *objv)
{
    bool iso_surface;
    if (GetBooleanFromObj(interp, objv[3], &iso_surface) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->set_isosurface(iso_surface);
    }
    return TCL_OK;
}

static int
VolumeShadingOpacityOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *CONST *objv)
{
    float opacity;
    if (GetFloatFromObj(interp, objv[3], &opacity) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->set_opacity_scale(opacity);
    }
    return TCL_OK;
}

static int
VolumeShadingSpecularOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			Tcl_Obj *CONST *objv)
{
    float specular;
    if (GetFloatFromObj(interp, objv[3], &specular) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->set_specular(specular);
    }
    return TCL_OK;
}

static int
VolumeShadingTransFuncOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			 Tcl_Obj *CONST *objv)
{
    TransferFunction *tf;
    char *name = Tcl_GetString(objv[3]);
    tf = NanoVis::get_transfunc(name);
    if (tf == NULL) {
	Tcl_AppendResult(interp, "transfer function \"", name,
			 "\" is not defined", (char*)NULL);
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	NanoVis::vol_renderer->shade_volume(*iter, tf);
#ifdef POINTSET
	// TBD..
	// POINTSET
	if ((*iter)->pointsetIndex != -1) {
	    g_pointSet[(*iter)->pointsetIndex]->updateColor(tf->getData(), 256);
	}
#endif /*POINTSET*/
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeShadingOps[] = {
    {"diffuse",     1, VolumeShadingDiffuseOp,    4, 0, "value ?indices?",},
    {"isosurface",  1, VolumeShadingIsosurfaceOp, 4, 0, "bool ?indices?",},
    {"opacity",     1, VolumeShadingOpacityOp,    4, 0, "value ?indices?",},
    {"specular",    1, VolumeShadingSpecularOp,   4, 0, "value ?indices?",},
    {"transfunc",   1, VolumeShadingTransFuncOp,  4, 0, "funcName ?indices?",},
};
static int nVolumeShadingOps = NumCmdSpecs(volumeShadingOps);

static int
VolumeShadingOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeShadingOps, volumeShadingOps, 
				  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
VolumeAxisOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *CONST *objv)
{
    char *string = Tcl_GetString(objv[2]);
    char c;
    c = string[0];
    if ((c == 'l') && (strcmp(string, "label") == 0)) {
	int axis;
	if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
	    return TCL_ERROR;
	}
	vector<Volume *> ivol;
	if (GetVolumes(interp, objc - 5, objv + 5, &ivol) != TCL_OK) {
	    return TCL_ERROR;
	}
	vector<Volume *>::iterator iter;
	char *label;
	label = Tcl_GetString(objv[4]);
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	    (*iter)->set_label(axis, label);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", string, 
			 "\": should be label", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
VolumeStateOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *CONST *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
	return TCL_ERROR;
    }
    vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 3, objv + 3, &ivol) != TCL_OK) {
	return TCL_ERROR;
    }
    if (state) {
	vector<Volume *>::iterator iter;
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	    (*iter)->enable();
	}
    } else {
	vector<Volume *>::iterator iter;
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	    (*iter)->disable();
	}
    }
    return TCL_OK;
}

static int
VolumeTestOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *CONST *objv)
{
    NanoVis::volume[1]->disable_data();
    NanoVis::volume[1]->disable();
    return TCL_OK;
}

static Rappture::CmdSpec volumeOps[] = {
    {"animation", 2, VolumeAnimationOp,   3, 3, "oper ?args?",},
    {"axis",      2, VolumeAxisOp,        3, 3, "label axis value ?indices?",},
    {"data",      1, VolumeDataOp,        3, 3, "oper ?args?",},
    {"outline",   1, VolumeOutlineOp,     3, 0, "oper ?args?",},
    {"shading",   2, VolumeShadingOp,     3, 0, "oper ?args?",},
    {"state",     2, VolumeStateOp,       3, 0, "bool ?indices?",},
    {"test2",     1, VolumeTestOp,        2, 2, "",},
};
static int nVolumeOps = NumCmdSpecs(volumeOps);

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   volume axis label x|y|z <value> ?<volumeId> ...?
 *   volume data state on|off ?<volumeId> ...?
 *   volume outline state on|off ?<volumeId> ...?
 *   volume outline color on|off ?<volumeId> ...?
 *   volume shading transfunc <name> ?<volumeId> ...?
 *   volume shading diffuse <value> ?<volumeId> ...?
 *   volume shading specular <value> ?<volumeId> ...?
 *   volume shading opacity <value> ?<volumeId> ...?
 *   volume state on|off ?<volumeId> ...?
 *
 * Clients send these commands to manipulate the volumes.
 * ----------------------------------------------------------------------
 */
static int
VolumeCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeOps, volumeOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
FlowCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    Rappture::Outcome err;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " option ?arg arg?", (char *)NULL);
        return TCL_ERROR;
    }
    char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'v') && (strcmp(string, "vectorid") == 0)) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " vectorid volume", (char *)NULL);
            return TCL_ERROR;
        }
        Volume *volPtr;
	const AxisRange *rangePtr;

        if (GetVolumeFromObj(interp, objv[2], &volPtr) != TCL_OK) { 
            return TCL_ERROR;
        }
	rangePtr = volPtr->GetRange(AxisRange::VALUES);
        if (NanoVis::particleRenderer != NULL) {
            NanoVis::particleRenderer->setVectorField(volPtr->id, 1.0f, 
		volPtr->height / (float)volPtr->width,
		volPtr->depth  / (float)volPtr->width,
		rangePtr->max);
            NanoVis::initParticle();
        }
        if (NanoVis::licRenderer != NULL) {
            NanoVis::licRenderer->setVectorField(volPtr->id,
		1.0f / volPtr->aspect_ratio_width,
		1.0f / volPtr->aspect_ratio_height,
		1.0f / volPtr->aspect_ratio_depth,
		rangePtr->max);
            NanoVis::licRenderer->set_offset(NanoVis::lic_slice_z);
        }
    } else if (c == 'l' && strcmp(string, "lic") == 0) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " lic on|off\"", (char*)NULL);
            return TCL_ERROR;
        }
        bool state;
        if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
            return TCL_ERROR;
        }
        NanoVis::lic_on = state;
    } else if ((c == 'p') && (strcmp(string, "particle") == 0)) {
        if (objc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " particle visible|slice|slicepos arg \"",
			     (char*)NULL);
            return TCL_ERROR;
        }
        char *string = Tcl_GetString(objv[2]);
        c = string[0];
        if ((c == 'v') && (strcmp(string, "visible") == 0)) {
            if (objc != 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", 
				 Tcl_GetString(objv[0]), " particle visible on|off\"", 
				 (char*)NULL);
                return TCL_ERROR;
            }
            bool state;
            if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            NanoVis::particle_on = state;
        } else if ((c == 's') && (strcmp(string, "slice") == 0)) {
            if (objc != 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", 
				 Tcl_GetString(objv[0]),
				 " particle slice volume\"", (char*)NULL);
                return TCL_ERROR;
            }
            int axis;
            if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
                return TCL_ERROR;
            }
            NanoVis::lic_axis = axis;
        } else if ((c == 's') && (strcmp(string, "slicepos") == 0)) {
            if (objc != 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", 
				 Tcl_GetString(objv[0]), " particle slicepos value\"", 
				 (char*)NULL);
                return TCL_ERROR;
            }
            float pos;
            if (GetFloatFromObj(interp, objv[2], &pos) != TCL_OK) {
                return TCL_ERROR;
            }
            if (pos < 0.0f) {
                pos = 0.0f;
            } else if (pos > 1.0f) {
                pos = 1.0f;
            }
            switch (NanoVis::lic_axis) {
            case 0 :
                NanoVis::lic_slice_x = pos;
                break;
            case 1 :
                NanoVis::lic_slice_y = pos;
                break;
            case 2 :
                NanoVis::lic_slice_z = pos;
                break;
            }
        } else {
            Tcl_AppendResult(interp, "unknown option \"",string,"\": should be \"",
			     Tcl_GetString(objv[0]), " visible, slice, or slicepos\"",
			     (char *)NULL);
            return TCL_ERROR;
        }
    } else if ((c == 'r') && (strcmp(string, "reset") == 0)) {
        NanoVis::initParticle();
    } else if ((c == 'c') && (strcmp(string, "capture") == 0)) {
        if (objc > 4 || objc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " capture numframes [directory]\"", 
			     (char*)NULL);
            return TCL_ERROR;
        }
        int total_frame_count;

        if (Tcl_GetIntFromObj(interp, objv[2], &total_frame_count) != TCL_OK) {
            return TCL_ERROR;
        }
        if (NanoVis::licRenderer) {
            NanoVis::licRenderer->activate();
        }
        if (NanoVis::particleRenderer) {
            NanoVis::particleRenderer->activate();
        }
        // Karl
        //
        Trace("FLOW started\n");
        char *fileName;
        fileName = (objc < 4) ? NULL : Tcl_GetString(objv[3]);
        for (int frame_count = 0; frame_count < total_frame_count; 
             frame_count++) {
            
            // Generate the latest frame and send it back to the client
            if (NanoVis::licRenderer && 
                NanoVis::licRenderer->isActivated()) {
                NanoVis::licRenderer->convolve();               
            }
            if (NanoVis::particleRenderer && 
                NanoVis::particleRenderer->isActivated()) {
                NanoVis::particleRenderer->advect();
            }
            NanoVis::offscreen_buffer_capture();  //enable offscreen render
            NanoVis::display();
            
            //          printf("Read Screen for Writing to file...\n");
            
            NanoVis::read_screen();
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

            NanoVis::bmp_write_to_file(frame_count, fileName);
        }
        Trace("FLOW end\n");
        // put your code... 
        if (NanoVis::licRenderer) {
            NanoVis::licRenderer->deactivate();
        }
        if (NanoVis::particleRenderer) {
            NanoVis::particleRenderer->deactivate();
        }
        NanoVis::initParticle();
    } else if ((c == 'd') && (strcmp(string, "data") == 0)) {
        if (objc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " data follows ?args?", (char *)NULL);
            return TCL_ERROR;
        }
        char *string = Tcl_GetString(objv[2]);;
        c = string[0];
        if ((c == 'f') && (strcmp(string,"follows") == 0)) {
            if (objc != 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", 
				 Tcl_GetString(objv[0]), " data follows length", 
				 (char *)NULL);
                return TCL_ERROR;
            }
            int nbytes;
            if (Tcl_GetIntFromObj(interp, objv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }
            Rappture::Buffer buf;
            if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
                return TCL_ERROR;
            }
            int n = NanoVis::n_volumes;
            std::stringstream fdata;
            fdata.write(buf.bytes(),buf.size());
            load_vector_stream(n, fdata);
            Volume *volPtr = NanoVis::volume[n];

            //
            // BE CAREFUL:  Set the number of slices to something
            //   slightly different for each volume.  If we have
            //   identical volumes at exactly the same position
            //   with exactly the same number of slices, the second
            //   volume will overwrite the first, so the first won't
            //   appear at all.
            //
            if (volPtr != NULL) {
                volPtr->set_n_slice(256-n);
                volPtr->disable_cutplane(0);
                volPtr->disable_cutplane(1);
                volPtr->disable_cutplane(2);

                NanoVis::vol_renderer->add_volume(volPtr, 
						  NanoVis::get_transfunc("default"));
            }
        }
    } else {
        return TCL_ERROR;
    }
    return TCL_OK;
}


static int
HeightMapDataFollowsOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *CONST *objv)
{
    Rappture::Buffer buf;
    int nBytes;
    
    if (Tcl_GetIntFromObj(interp, objv[3], &nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    buf.append("\0", 1);
    int result;
    result = Tcl_Eval(interp, buf.bytes());
    if (result != TCL_OK) {
        fprintf(stderr, "error in command: %s\n", 
                Tcl_GetStringResult(interp));
        fflush(stderr);
    }
    return result;
}

static int
HeightMapDataVisibleOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *CONST *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    vector<unsigned int> indices;
    if (GetIndices(interp, objc - 4, objv + 4, &indices) != TCL_OK) {
        return TCL_ERROR;
    }
    for (unsigned int i = 0; i < indices.size(); ++i) {
        if ((indices[i] < NanoVis::heightMap.size()) && 
            (NanoVis::heightMap[indices[i]] != NULL)) {
            NanoVis::heightMap[indices[i]]->setVisible(visible);
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightMapDataOps[] = {
    {"follows",      1, HeightMapDataFollowsOp, 4, 4, "length",},
    {"visible",      1, HeightMapDataVisibleOp, 4, 0, "bool ?indices?",},
};
static int nHeightMapDataOps = NumCmdSpecs(heightMapDataOps);

static int 
HeightMapDataOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapDataOps, heightMapDataOps, 
				  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}


static int
HeightMapLineContourColorOp(ClientData cdata, Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST *objv)
{
    float rgb[3];
    if (GetColor(interp, objc - 3, objv + 3, rgb) != TCL_OK) {
        return TCL_ERROR;
    }           
    vector<unsigned int> indices;
    if (GetIndices(interp, objc-6, objv + 6, &indices) != TCL_OK) {
        return TCL_ERROR;
    }
    for (unsigned int i = 0; i < indices.size(); ++i) {
        if ((indices[i] < NanoVis::heightMap.size()) && 
            (NanoVis::heightMap[indices[i]] != NULL)) {
            NanoVis::heightMap[indices[i]]->setLineContourColor(rgb);
        }
    }
    return TCL_OK;
}

static int
HeightMapLineContourVisibleOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                              Tcl_Obj *CONST *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    vector<unsigned int> indices;
    if (GetIndices(interp, objc-4, objv+4, &indices) != TCL_OK) {
        return TCL_ERROR;
    }
    for (unsigned int i = 0; i < indices.size(); ++i) {
        if ((indices[i] < NanoVis::heightMap.size()) && 
            (NanoVis::heightMap[indices[i]] != NULL)) {
            NanoVis::heightMap[indices[i]]->setLineContourVisible(visible);
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightMapLineContourOps[] = {
    {"color",   1, HeightMapLineContourColorOp,   4, 4, "length",},
    {"visible", 1, HeightMapLineContourVisibleOp, 4, 0, "bool ?indices?",},
};
static int nHeightMapLineContourOps = NumCmdSpecs(heightMapLineContourOps);

static int 
HeightMapLineContourOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		       Tcl_Obj *CONST *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapLineContourOps, 
				  heightMapLineContourOps, Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
HeightMapCullOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		Tcl_Obj *CONST *objv)
{
    graphics::RenderContext::CullMode mode;
    if (GetCullMode(interp, objv[2], &mode) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setCullMode(mode);
    return TCL_OK;
}

static int
HeightMapCreateOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *CONST *objv)
{
    HeightMap *hMap;
    
    /* heightmap create xmin ymin xmax ymax xnum ynum values */
    hMap = CreateHeightMap(cdata, interp, objc - 2, objv + 2);
    if (hMap == NULL) {
        return TCL_ERROR;
    }
    NanoVis::heightMap.push_back(hMap);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), NanoVis::heightMap.size() - 1);;
    return TCL_OK;
}

static int
HeightMapLegendOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *CONST *objv)
{
    HeightMap *hMap;
    if (GetHeightMap(interp, objv[2], &hMap) != TCL_OK) {
        return TCL_ERROR;
    }
    TransferFunction *tf;
    tf = hMap->getColorMap();
    if (tf == NULL) {
        Tcl_AppendResult(interp, "no transfer function defined for heightmap \"", 
			 Tcl_GetString(objv[2]), "\"", (char*)NULL);
        return TCL_ERROR;
    }
    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[3], &w) != TCL_OK) || 
        (Tcl_GetIntFromObj(interp, objv[4], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    const AxisRange *rangePtr;
    rangePtr = hMap->GetRange(AxisRange::VALUES);
    NanoVis::render_legend(tf, rangePtr->min, rangePtr->max, w, h, "label");
    return TCL_OK;
}

static int
HeightMapPolygonOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *CONST *objv)
{
    graphics::RenderContext::PolygonMode mode;
    if (GetPolygonMode(interp, objv[2], &mode) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setPolygonMode(mode);
    return TCL_OK;
}

static int
HeightMapShadeOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *CONST *objv)
{
    graphics::RenderContext::ShadingModel model;
    if (GetShadingModel(interp, objv[2], &model) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setShadingModel(model);
    return TCL_OK;
}

static int
HeightMapTestOp(ClientData cdata, Tcl_Interp *interp, int objc, 
		Tcl_Obj *CONST *objv)
{
    srand((unsigned)time(NULL));

    int size = 20 * 200;
    double sigma = 5.0;
    double mean = exp(0.0) / (sigma * sqrt(2.0));
    float* data = (float*) malloc(sizeof(float) * size);
    
    float x;
    for (int i = 0; i < size; ++i) {
        x = - 10 + i%20;
        data[i] = exp(- (x * x)/(2 * sigma * sigma)) / 
            (sigma * sqrt(2.0)) / mean * 2 + 1000;
    }
    
    HeightMap* heightMap = new HeightMap();
    float minx = 0.0f;
    float maxx = 1.0f;
    float miny = 0.5f;
    float maxy = 3.5f;
    heightMap->setHeight(minx, miny, maxx, maxy, 20, 200, data);
    heightMap->setColorMap(NanoVis::get_transfunc("default"));
    heightMap->setVisible(true);
    heightMap->setLineContourVisible(true);
    NanoVis::heightMap.push_back(heightMap);


    const AxisRange *rangePtr;
    rangePtr = heightMap->GetRange(AxisRange::VALUES);
    Vector3 min(minx, (float)rangePtr->min, miny);
    Vector3 max(maxx, (float)rangePtr->max, maxy);
    
    NanoVis::grid->setMinMax(min, max);
    NanoVis::grid->setVisible(true);
    
    return TCL_OK;
}

static int
HeightMapTransFuncOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *CONST *objv)
{
    char *name;
    name = Tcl_GetString(objv[2]);
    TransferFunction *tf;
    tf = NanoVis::get_transfunc(name);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", name,
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    vector<unsigned int> indices;
    if (GetIndices(interp, objc - 3, objv + 3, &indices) != TCL_OK) {
        return TCL_ERROR;
    }
    for (unsigned int i = 0; i < indices.size(); ++i) {
        if ((indices[i] < NanoVis::heightMap.size()) && 
            (NanoVis::heightMap[indices[i]] != NULL)) {
            NanoVis::heightMap[indices[i]]->setColorMap(tf);
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightMapOps[] = {
    {"create",       2, HeightMapCreateOp,      9, 9, 
     "xmin ymin xmax ymax xnum ynum values",},
    {"cull",         2, HeightMapCullOp,        3, 3, "mode",},
    {"data",         1, HeightMapDataOp,        3, 0, "oper ?args?",},
    {"legend",       2, HeightMapLegendOp,      5, 5, "index width height",},
    {"linecontour",  2, HeightMapLineContourOp, 2, 0, "oper ?args?",},
    {"polygon",      1, HeightMapPolygonOp,     3, 3, "mode",},
    {"shade",        1, HeightMapShadeOp,       3, 3, "model",},
    {"test",         2, HeightMapTestOp,        2, 2, "",},
    {"transfunc",    2, HeightMapTransFuncOp,   3, 0, "name ?indices?",},
};
static int nHeightMapOps = NumCmdSpecs(heightMapOps);

static int 
HeightMapCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{ 
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapOps, heightMapOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int
GridAxisColorOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                Tcl_Obj *CONST *objv)
{
    float r, g, b, a;
    if ((GetFloatFromObj(interp, objv[2], &r) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &g) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &b) != TCL_OK)) {
        return TCL_ERROR;
    }
    a = 1.0f;
    if ((objc == 6) && (GetFloatFromObj(interp, objv[5], &a) != TCL_OK)) {
        return TCL_ERROR;
    }           
    if (NanoVis::grid) {
        NanoVis::grid->setAxisColor(r, g, b, a);
    }
    return TCL_OK;
}

static int
GridAxisNameOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *CONST *objv)
{
    int axisId;
    if (GetAxisFromObj(interp, objv[2], &axisId) != TCL_OK) {
        return TCL_ERROR;
    }
    if (NanoVis::grid) {
        NanoVis::grid->setAxisName(axisId, Tcl_GetString(objv[3]));
    }
    return TCL_OK;
}

static int
GridLineColorOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                Tcl_Obj *CONST *objv)
{
    float r, g, b, a;
    if ((GetFloatFromObj(interp, objv[2], &r) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &g) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &b) != TCL_OK)) {
        return TCL_ERROR;
    }
    a = 1.0f;
    if ((objc == 6) && (GetFloatFromObj(interp, objv[5], &a) != TCL_OK)) {
        return TCL_ERROR;
    }           
    if (NanoVis::grid) {
        NanoVis::grid->setGridLineColor(r, g, b, a);
    }
    return TCL_OK;
}

static int
GridLineCountOp(ClientData cdata, Tcl_Interp *interp, int objc, 
                Tcl_Obj *CONST *objv)
{
    int x, y, z;
    
    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &z) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (NanoVis::grid) {
        NanoVis::grid->setGridLineCount(x, y, z);
    }
    return TCL_OK;
}

static int
GridMinMaxOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    double x1, y1, z1, x2, y2, z2;
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &x1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &y1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[4], &z1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[5], &x2) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[6], &y2) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[7], &z2) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (NanoVis::grid) {
        NanoVis::grid->setMinMax(Vector3(x1, y1, z1), Vector3(x2, y2, z2));
    }
    return TCL_OK;
}

static int
GridVisibleOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[2], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::grid->setVisible(visible);
    return TCL_OK;
}

static Rappture::CmdSpec gridOps[] = {
    {"axiscolor",  5, GridAxisColorOp,  5, 6, "r g b ?a?",},
    {"axisname",   5, GridAxisNameOp,   5, 5, "index width height",},
    {"linecolor",  7, GridLineColorOp,  5, 6, "r g b ?a?",},
    {"linecount",  7, GridLineCountOp,  5, 5, "xCount yCount zCount",},
    {"minmax",     1, GridMinMaxOp,     8, 8, "xMin yMin zMin xMax yMax zMax",},
    {"visible",    1, GridVisibleOp,    3, 3, "bool",},
};
static int nGridOps = NumCmdSpecs(gridOps);

static int 
GridCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{ 
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nGridOps, gridOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

static int 
AxisCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }
    char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'v') && (strcmp(string, "visible") == 0)) {
        bool visible;

        if (GetBooleanFromObj(interp, objv[2], &visible) != TCL_OK) {
            return TCL_ERROR;
        }
        NanoVis::axis_on = visible;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

#if PLANE_CMD
static int 
PlaneNewOp(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{
    fprintf(stderr, "load plane for 2D visualization command\n");
    int index, w, h;
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " plane_index w h \"", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK) {
        return TCL_ERROR;
    }
    
    //Now read w*h*4 bytes. The server expects the plane to be a stream of floats 
    char* tmp = new char[int(w*h*sizeof(float))];
    if (tmp == NULL) {
        Tcl_AppendResult(interp, "can't allocate stream data", (char *)NULL);
        return TCL_ERROR;
    }
    bzero(tmp, w*h*4);
    int status = read(0, tmp, w*h*sizeof(float));
    if (status <= 0) {
        exit(0);                // Bail out on read error?  Should log the
                                // error and return a non-zero exit status.
    }
    plane[index] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, (float*)tmp);
    delete[] tmp;
    return TCL_OK;
}


static int 
PlaneLinkOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *CONST *objv)
{
    fprintf(stderr, "link the plane to the 2D renderer command\n");
    
    int plane_index, tf_index;
    
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " plane_index tf_index \"", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &plane_index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &tf_index) != TCL_OK) {
        return TCL_ERROR;
    }
    //plane_render->add_plane(plane[plane_index], tf[tf_index]);
    return TCL_OK;
}

//Enable a 2D plane for render
//The plane_index is the index mantained in the 2D plane renderer
static int 
PlaneEnableOp(ClientData cdata, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *CONST *objv)
{
    fprintf(stderr,"enable a plane so the 2D renderer can render it command\n");
    
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
			 Tcl_GetString(objv[0]), " plane_index mode \"", (char*)NULL);
        return TCL_ERROR;
    }
    int plane_index;
    if (Tcl_GetIntFromObj(interp, objv[1], &plane_index) != TCL_OK) {
        return TCL_ERROR;
    }
    int mode;
    if (Tcl_GetIntFromObj(interp, objv[2], &mode) != TCL_OK) {
        return TCL_ERROR;
    }
    if (mode == 0) {
        plane_index = -1;
    }
    plane_render->set_active_plane(plane_index);
    return TCL_OK;
}

static Rappture::CmdSpec planeOps[] = {
    {"enable",     1, PlaneEnableOp,    4, 4, "planeIdx mode",},
    {"link",       1, PlaneLinkOp,      4, 4, "planeIdx transfuncIdx",},
    {"new",        1, PlaneNewOp,       5, 5, "planeIdx width height",},
};
static int nPlaneOps = NumCmdSpecs(planeOps);

static int 
PlaneCmd(ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{ 
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nPlaneOps, planeOps, 
				  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (cdata, interp, objc, objv);
}

#endif  /*PLANE_CMD*/

/*
 * This command should be Tcl procedure instead of a C command.  The reason
 * for this that 1) we are using a safe interpreter so we would need a master
 * interpreter to load the Tcl environment properly (including our "unirect2d"
 * procedure). And 2) the way nanovis is currently deployed doesn't make it
 * easy to add new directories for procedures, since it's loaded into /tmp.
 *
 * Ideally, the "unirect2d" proc would do a rundimentary parsing of the data
 * to verify the structure and then pass it to the appropiate Tcl command
 * (heightmap, volume, etc). Our C command always creates a heightmap.  
 */
static int
UniRect2dCmd(ClientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST *objv)
{    
    int xNum, yNum, zNum;
    float xMin, yMin, xMax, yMax;
    float *zValues;

    if ((objc & 0x01) == 0) {
        Tcl_AppendResult(interp, Tcl_GetString(objv[0]), ": ", 
			 "wrong number of arguments: should be key-value pairs", 
			 (char *)NULL);
        return TCL_ERROR;
    }
    zValues = NULL;
    xNum = yNum = zNum = 0;
    xMin = yMin = xMax = yMax = 0.0f;
    int i;
    for (i = 1; i < objc; i += 2) {
        char *string;

        string = Tcl_GetString(objv[i]);
        if (strcmp(string, "xmin") == 0) {
            if (GetFloatFromObj(interp, objv[i+1], &xMin) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (strcmp(string, "xmax") == 0) {
            if (GetFloatFromObj(interp, objv[i+1], &xMax) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (strcmp(string, "xnum") == 0) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &xNum) != TCL_OK) {
                return TCL_ERROR;
            }
            if (xNum <= 0) {
                Tcl_AppendResult(interp, "bad xnum value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
        } else if (strcmp(string, "ymin") == 0) {
            if (GetFloatFromObj(interp, objv[i+1], &yMin) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (strcmp(string, "ymax") == 0) {
            if (GetFloatFromObj(interp, objv[i+1], &yMax) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (strcmp(string, "ynum") == 0) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &yNum) != TCL_OK) {
                return TCL_ERROR;
            }
            if (yNum <= 0) {
                Tcl_AppendResult(interp, "bad ynum value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
        } else if (strcmp(string, "zvalues") == 0) {
            Tcl_Obj **zObj;

            if (Tcl_ListObjGetElements(interp, objv[i+1], &zNum, &zObj)!= TCL_OK) {
                return TCL_ERROR;
            }
            int j;
            zValues = new float[zNum];
            for (j = 0; j < zNum; j++) {
                if (GetFloatFromObj(interp, zObj[j], zValues + j) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        } else {
            Tcl_AppendResult(interp, "unknown key \"", string, 
			     "\": should be xmin, xmax, xnum, ymin, ymax, ynum, or zvalues",
			     (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (zValues == NULL) {
        Tcl_AppendResult(interp, "missing \"zvalues\" key", (char *)NULL);
        return TCL_ERROR;
    }
    if (zNum != (xNum * yNum)) {
        Tcl_AppendResult(interp, "wrong number of z values must be xnum*ynum",
			 (char *)NULL);
        return TCL_ERROR;
    }
    HeightMap* hMap;
    hMap = new HeightMap();
    hMap->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, zValues);
    hMap->setColorMap(NanoVis::get_transfunc("default"));
    hMap->setVisible(true);
    hMap->setLineContourVisible(true);
    NanoVis::heightMap.push_back(hMap);
    delete [] zValues;
    return TCL_OK;
}


void 
initTcl()
{
    /*
     * Ideally the connection is authenticated by nanoscale.  I still like the
     * idea of creating a non-safe master interpreter with a safe slave
     * interpreter.  Alias all the nanovis commands in the slave. That way we
     * can still run Tcl code within nanovis.  The eventual goal is to create
     * a test harness through the interpreter for nanovis.
     */
    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    Tcl_DStringInit(&cmdbuffer);

    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "cutplane",    CutplaneCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "flow",        FlowCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "grid",        GridCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",   HeightMapCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "screenshot",  ScreenShotCmd,  NULL, NULL);
    Tcl_CreateObjCommand(interp, "transfunc",   TransfuncCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "unirect2d",   UniRect2dCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "up",          UpCmd,          NULL, NULL);
    Tcl_CreateObjCommand(interp, "volume",      VolumeCmd,      NULL, NULL);
#if __TEST_CODE__
    Tcl_CreateObjCommand(interp, "test", TestCmd, NULL, NULL);
#endif

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        fprintf(stdin, "WARNING: bad default transfer function\n");
        fprintf(stdin, Tcl_GetStringResult(interp));
    }
}


void 
xinetd_listen()
{
    int flags = fcntl(0, F_GETFL, 0); 
    fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

    int status = TCL_OK;
    int npass = 0;

    //
    //  Read and execute as many commands as we can from stdin...
    //
    while (status == TCL_OK) {
        //
        //  Read the next command from the buffer.  First time through we
        //  block here and wait if necessary until a command comes in.
        //
        //  BE CAREFUL: Read only one command, up to a newline.  The "volume
        //  data follows" command needs to be able to read the data
        //  immediately following the command, and we shouldn't consume it
        //  here.
        //
        while (1) {
            char c = getchar();
            if (c <= 0) {
                if (npass == 0) {
                    exit(0);  // EOF -- we're done!
                } else {
                    break;
                }
            }
            Tcl_DStringAppend(&cmdbuffer, &c, 1);

            if (c=='\n' && Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer))) {
                break;
            }
        }

        // no command? then we're done for now
        if (Tcl_DStringLength(&cmdbuffer) == 0) {
            break;
        }

        // back to original flags during command evaluation...
        fcntl(0, F_SETFL, flags & ~O_NONBLOCK);
        status = Tcl_Eval(interp, Tcl_DStringValue(&cmdbuffer));
        Tcl_DStringSetLength(&cmdbuffer, 0);

        // non-blocking for next read -- we might not get anything
        fcntl(0, F_SETFL, flags | O_NONBLOCK);
        npass++;
    }
    fcntl(0, F_SETFL, flags);

    if (status != TCL_OK) {
        std::ostringstream errmsg;
        errmsg << "ERROR: " << Tcl_GetStringResult(interp) << std::endl;
        write(0, errmsg.str().c_str(), errmsg.str().size());
        return;
    }

    //
    // This is now in "FlowCmd()":
    //  Generate the latest frame and send it back to the client
    //
    /*
      if (NanoVis::licRenderer && NanoVis::licRenderer->isActivated())
      {
      NanoVis::licRenderer->convolve();
      }

      if (NanoVis::particleRenderer && NanoVis::particleRenderer->isActivated())
      {
      NanoVis::particleRenderer->advect();
      }
    */

    NanoVis::update();

    NanoVis::offscreen_buffer_capture();  //enable offscreen render

    NanoVis::display();
    
    // INSOO
#ifdef XINETD
    NanoVis::read_screen();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 
#else
    NanoVis::display_offscreen_buffer(); //display the final rendering on screen
    NanoVis::read_screen();
    glutSwapBuffers(); 
#endif   

#if DO_RLE
    do_rle();
    int sizes[2] = {  offsets_size*sizeof(offsets[0]), rle_size }; 
    fprintf(stderr, "Writing %d,%d\n", sizes[0], sizes[1]); fflush(stderr);
    write(0, &sizes, sizeof(sizes));
    write(0, offsets, offsets_size*sizeof(offsets[0]));
    write(0, rle, rle_size);    //unsigned byte
#else
    NanoVis::bmp_write("nv>image -bytes");
#endif
}
