/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * TODO:  In no particular order...
 *        o Use Tcl command option parser to reduce size of procedures, remove
 *          lots of extra error checking code. (almost there)
 *        o Convert GetVolumeIndices to GetVolumes.  Goal is to remove
 *          all references of Nanovis::volume[] from this file.  Don't
 *          want to know how volumes are stored. Same for heightmaps.
 *        o Rationalize volume id scheme. Right now it's the index in
 *          the vector. 1) Use a list instead of a vector. 2) carry
 *          an id field that's a number that gets incremented each new volume.
 *        o Add bookkeeping for volumes, heightmaps, flows, etc. to track
 *          1) id #  2) simulation # 3) include/exclude.  The include/exclude
 *          is to indicate whether the item should contribute to the overall
 *          limits of the axes.
 */

#include <assert.h>
#include <stdlib.h>

#include <tcl.h>

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpEncode.h>
#include <RpOutcome.h>
#include <RpBuffer.h>

#include <vrmath/Vector3f.h>

#include "nanovis.h"
#include "CmdProc.h"
#include "Trace.h"

#ifdef PLANE_CMD
#include "PlaneRenderer.h"
#endif
#ifdef USE_POINTSET_RENDERER
#include "PointSet.h"
#endif
#include "dxReader.h"
#include "Grid.h"
#include "HeightMap.h"
#include "NvCamera.h"
#include "NvZincBlendeReconstructor.h"
#include "Unirect.h"
#include "Volume.h"
#include "VolumeRenderer.h"

using namespace nv::graphics;

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
  0.00000  0.0\n\
  0.19999  0.0\n\
  0.20000  1.0\n\
  0.20001  0.0\n\
  0.39999  0.0\n\
  0.40000  1.0\n\
  0.40001  0.0\n\
  0.59999  0.0\n\
  0.60000  1.0\n\
  0.60001  0.0\n\
  0.79999  0.0\n\
  0.80000  1.0\n\
  0.80001  0.0\n\
  1.00000  0.0\n\
}";

static Tcl_ObjCmdProc AxisCmd;
static Tcl_ObjCmdProc CameraCmd;
static Tcl_ObjCmdProc CutplaneCmd;
extern Tcl_AppInitProc FlowCmdInitProc;
static Tcl_ObjCmdProc GridCmd;
static Tcl_ObjCmdProc LegendCmd;
#ifdef PLANE_CMD
static Tcl_ObjCmdProc PlaneCmd;
#endif
static Tcl_ObjCmdProc ScreenCmd;
static Tcl_ObjCmdProc SnapshotCmd;
static Tcl_ObjCmdProc TransfuncCmd;
static Tcl_ObjCmdProc Unirect2dCmd;
static Tcl_ObjCmdProc UpCmd;
static Tcl_ObjCmdProc VolumeCmd;

bool
GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, bool *boolPtr)
{
    int value;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *boolPtr = (bool)value;
    return TCL_OK;
}

int
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
            RenderContext::CullMode *modePtr)
{
    const char *string = Tcl_GetString(objPtr);
    if (strcmp(string, "none") == 0) {
        *modePtr = RenderContext::NO_CULL;
    } else if (strcmp(string, "front") == 0) {
        *modePtr = RenderContext::FRONT;
    } else if (strcmp(string, "back") == 0) {
        *modePtr = RenderContext::BACK;
    } else {
        Tcl_AppendResult(interp, "invalid cull mode \"", string,
                         "\": should be front, back, or none\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetShadingModel(Tcl_Interp *interp, Tcl_Obj *objPtr,
                RenderContext::ShadingModel *modelPtr)
{
    const char *string = Tcl_GetString(objPtr);

    if (strcmp(string,"flat") == 0) {
        *modelPtr = RenderContext::FLAT;
    } else if (strcmp(string,"smooth") == 0) {
        *modelPtr = RenderContext::SMOOTH;
    } else {
        Tcl_AppendResult(interp, "bad shading model \"", string,
                         "\": should be flat or smooth", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetPolygonMode(Tcl_Interp *interp, Tcl_Obj *objPtr,
               RenderContext::PolygonMode *modePtr)
{
    const char *string = Tcl_GetString(objPtr);

    if (strcmp(string,"wireframe") == 0) {
        *modePtr = RenderContext::LINE;
    } else if (strcmp(string,"fill") == 0) {
        *modePtr = RenderContext::FILL;
    } else {
        Tcl_AppendResult(interp, "invalid polygon mode \"", string,
                         "\": should be wireframe or fill\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/**
 * Creates a heightmap from the given the data. The format of the data
 * should be as follows:
 *
 *     xMin, xMax, xNum, yMin, yMax, yNum, heights...
 *
 * xNum and yNum must be integer values, all others are real numbers.
 * The number of heights must be xNum * yNum;
 */
static HeightMap *
CreateHeightMap(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
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
    HeightMap* hmPtr;
    hmPtr = new HeightMap();
    hmPtr->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, heights);
    hmPtr->transferFunction(NanoVis::getTransfunc("default"));
    hmPtr->setVisible(true);
    hmPtr->setLineContourVisible(true);
    delete [] heights;
    return hmPtr;
}

static int
GetHeightMapFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, HeightMap **hmPtrPtr)
{
    const char *string;
    string = Tcl_GetString(objPtr);

    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&NanoVis::heightmapTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a heightmap named \"",
                         string, "\"", (char*)NULL);
	}
        return TCL_ERROR;
    }
    *hmPtrPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}


/**
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 */
static int
GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Volume **volPtrPtr)
{
    const char *string;
    string = Tcl_GetString(objPtr);

    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&NanoVis::volumeTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a volume named \"",
                         string, "\"", (char*)NULL);
	}
        return TCL_ERROR;
    }
    *volPtrPtr = (Volume *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

/**
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 */
static int
GetVolumes(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
           std::vector<Volume *>* vectorPtr)
{
    if (objc == 0) {
	// No arguments. Get all volumes. 
	Tcl_HashSearch iter;
	Tcl_HashEntry *hPtr;
        for (hPtr = Tcl_FirstHashEntry(&NanoVis::volumeTable, &iter); 
	     hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
	    Volume *volPtr;
	    volPtr = (Volume *)Tcl_GetHashValue(hPtr);
	    vectorPtr->push_back(volPtr);
	}
    } else {
	// Get the volumes associated with the given index arguments.
        for (int n = 0; n < objc; n++) {
            Volume *volPtr;

            if (GetVolumeFromObj(interp, objv[n], &volPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            vectorPtr->push_back(volPtr);
        }
    }
    return TCL_OK;
}

/**
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 */
static int
GetHeightMaps(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
              std::vector<HeightMap *>* vectorPtr)
{
    if (objc == 0) {
	// No arguments. Get all heightmaps. 
	Tcl_HashSearch iter;
	Tcl_HashEntry *hPtr;
        for (hPtr = Tcl_FirstHashEntry(&NanoVis::heightmapTable, &iter); 
	     hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
	    HeightMap *hmPtr;
	    hmPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
	    vectorPtr->push_back(hmPtr);
	}
    } else {
        for (int n = 0; n < objc; n++) {
            HeightMap *hmPtr;

            if (GetHeightMapFromObj(interp, objv[n], &hmPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            vectorPtr->push_back(hmPtr);
        }
    }
    return TCL_OK;
}

/**
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in valPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 */
static int
GetAxis(Tcl_Interp *interp, const char *string, int *indexPtr)
{
    if (string[1] == '\0') {
        char c;

        c = tolower((unsigned char)string[0]);
        if (c == 'x') {
            *indexPtr = 0;
            return TCL_OK;
        } else if (c == 'y') {
            *indexPtr = 1;
            return TCL_OK;
        } else if (c == 'z') {
            *indexPtr = 2;
            return TCL_OK;
        }
        /*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
                     "\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

/**
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in indexPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 */
int
GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr)
{
    return GetAxis(interp, Tcl_GetString(objPtr), indexPtr);
}

/**
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in indexPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 */
static int
GetAxisDirFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr, int *dirPtr)
{
    const char *string = Tcl_GetString(objPtr);

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

/**
 * Used internally to decode a color value from a string ("R G B")
 * as a list of three numbers 0-1.  Returns TCL_OK if successful,
 * along with RGB values in rgbPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 */
static int
GetColor(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, float *rgbPtr)
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

/**
 * Read the requested number of bytes from standard input into the given
 * buffer.  The buffer is then decompressed and decoded.
 */
int
GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes)
{
    char buffer[8096];

    clearerr(NanoVis::stdin);
    while (nBytes > 0) {
        unsigned int chunk;
        int nRead;

        chunk = (sizeof(buffer) < (unsigned int) nBytes) ?
            sizeof(buffer) : nBytes;
        nRead = fread(buffer, sizeof(char), chunk, NanoVis::stdin);
        if (ferror(NanoVis::stdin)) {
            Tcl_AppendResult(interp, "while reading data stream: ",
                             Tcl_PosixError(interp), (char*)NULL);
            return TCL_ERROR;
        }
        if (feof(NanoVis::stdin)) {
            Tcl_AppendResult(interp, "premature EOF while reading data stream",
                             (char*)NULL);
            return TCL_ERROR;
        }
        buf.append(buffer, nRead);
        nBytes -= nRead;
    }
    if (NanoVis::recfile != NULL) {
	ssize_t nWritten;

        nWritten = fwrite(buf.bytes(), sizeof(char), buf.size(), 
			  NanoVis::recfile);
	assert(nWritten == (ssize_t)buf.size());
        fflush(NanoVis::recfile);
    }
    Rappture::Outcome err;
    TRACE("Checking header[%.13s]", buf.bytes());
    if (strncmp (buf.bytes(), "@@RP-ENC:", 9) == 0) {
	/* There's a header on the buffer, use it to decode the data. */
	if (!Rappture::encoding::decode(err, buf, RPENC_HDR)) {
	    Tcl_AppendResult(interp, err.remark(), (char*)NULL);
	    return TCL_ERROR;
	}
    } else if (Rappture::encoding::isBase64(buf.bytes(), buf.size())) {
	/* No header, but it's base64 encoded.  It's likely that it's both
	 * base64 encoded and compressed. */
	if (!Rappture::encoding::decode(err, buf, RPENC_B64 | RPENC_Z)) {
	    Tcl_AppendResult(interp, err.remark(), (char*)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
CameraAngleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    float theta, phi, psi;
    if ((GetFloatFromObj(interp, objv[2], &phi) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &theta) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &psi) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::cam->rotate(phi, theta, psi);
    return TCL_OK;
}

static int
CameraOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double quat[4];
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &quat[3]) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &quat[0]) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[4], &quat[1]) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[5], &quat[2]) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::cam->rotate(quat);
    return TCL_OK;
}

static int
CameraPanOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    float x, y;
    if ((GetFloatFromObj(interp, objv[2], &x) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::pan(x, y);
    return TCL_OK;
}

static int
CameraPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    float x, y, z;
    if ((GetFloatFromObj(interp, objv[2], &x) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &y) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &z) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::cam->x(x);
    NanoVis::cam->y(y);
    NanoVis::cam->z(z);
    return TCL_OK;
}

static int
CameraResetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool all = false;
    if (objc == 3) {
        const char *string = Tcl_GetString(objv[2]);
        char c = string[0];
        if ((c != 'a') || (strcmp(string, "all") != 0)) {
            Tcl_AppendResult(interp, "bad camera reset option \"", string,
                             "\": should be all", (char*)NULL);
            return TCL_ERROR;
        }
        all = true;
    }
    NanoVis::resetCamera(all);
    return TCL_OK;
}

static int
CameraZoomOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    float z;
    if (GetFloatFromObj(interp, objv[2], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::zoom(z);
    return TCL_OK;
}

static Rappture::CmdSpec cameraOps[] = {
    {"angle",   2, CameraAngleOp,    5, 5, "xAngle yAngle zAngle",},
    {"orient",  1, CameraOrientOp,   6, 6, "qw qx qy qz",},
    {"pan",     2, CameraPanOp,      4, 4, "x y",},
    {"pos",     2, CameraPositionOp, 5, 5, "x y z",},
    {"reset",   1, CameraResetOp,    2, 3, "?all?",},
    {"zoom",    1, CameraZoomOp,     3, 3, "factor",},
};
static int nCameraOps = NumCmdSpecs(cameraOps);

static int
CameraCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCameraOps, cameraOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
SnapshotCmd(ClientData clientData, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv)
{
    int w, h;

    w = NanoVis::winWidth, h = NanoVis::winHeight;

    NanoVis::resizeOffscreenBuffer(2048, 2048);
#ifdef notdef
    NanoVis::cam->setScreenSize(0, 0, NanoVis::winWidth, NanoVis::winHeight);
    NanoVis::cam->setScreenSize(30, 90, 2048 - 60, 2048 - 120);
#endif
    NanoVis::bindOffscreenBuffer();  //enable offscreen render
    NanoVis::display();
    NanoVis::readScreen();

    NanoVis::ppmWrite("nv>image -type print -bytes %d");
    NanoVis::resizeOffscreenBuffer(w, h);

    return TCL_OK;
}

static int
CutplanePositionOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
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

    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->moveCutplane(axis, relval);
    }
    return TCL_OK;
}

/*
 * cutplane state $bool $axis vol,,,
 */
static int
CutplaneStateOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }

    int axis;
    if (GetAxisFromObj(interp, objv[3], &axis) != TCL_OK) {
        return TCL_ERROR;
    }

    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    if (state) {
        std::vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->enableCutplane(axis);
        }
    } else {
        std::vector<Volume *>::iterator iter;
        for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->disableCutplane(axis);
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec cutplaneOps[] = {
    {"position", 1, CutplanePositionOp, 4, 0, "relval axis ?indices?",},
    {"state",    1, CutplaneStateOp,    4, 0, "bool axis ?indices?",},
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
CutplaneCmd(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCutplaneOps, cutplaneOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/* 
 * ClientInfoCmd --
 *
 *      Log initial values to stats file.  The first time this is called
 *      "render_start" is written into the stats file.  Afterwards, it 
 *      is "render_info". 
 *	
 *         clientinfo list
 */
static int
ClientInfoCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_DString ds;
    Tcl_Obj *objPtr, *listObjPtr, **items;
    int result;
    int i, numItems, length;
    char buf[BUFSIZ];
    const char *string;
    static int first = 1;

    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # of arguments: should be \"", 
                Tcl_GetString(objv[0]), " list\"", (char *)NULL);
	return TCL_ERROR;
    }
#ifdef KEEPSTATS
    int f;

    /* Use the initial client key value pairs as the parts for a generating
     * a unique file name. */
    f = NanoVis::getStatsFile(objv[1]);
    if (f < 0) {
	Tcl_AppendResult(interp, "can't open stats file: ", 
                         Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
#endif
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_IncrRefCount(listObjPtr);
    if (first) {
        first = false;
        objPtr = Tcl_NewStringObj("render_start", 12);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* server */
        objPtr = Tcl_NewStringObj("server", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("nanovis", 7);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* pid */
        objPtr = Tcl_NewStringObj("pid", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(getpid()));
        /* machine */
        objPtr = Tcl_NewStringObj("machine", 7);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        gethostname(buf, BUFSIZ-1);
        buf[BUFSIZ-1] = '\0';
        objPtr = Tcl_NewStringObj(buf, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    } else {
        objPtr = Tcl_NewStringObj("render_info", 11);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_DStringInit(&ds);
    /* date */
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("date", 4));
    strcpy(buf, ctime(&NanoVis::startTime.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(buf, -1));
    /* date_secs */
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("date_secs", 9));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewLongObj(NanoVis::startTime.tv_sec));
    /* Client arguments. */
    if (Tcl_ListObjGetElements(interp, objv[1], &numItems, &items) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < numItems; i++) {
        Tcl_ListObjAppendElement(interp, listObjPtr, items[i]);
    }
    Tcl_DStringInit(&ds);
    string = Tcl_GetStringFromObj(listObjPtr, &length);
    Tcl_DStringAppend(&ds, string, length);
    Tcl_DStringAppend(&ds, "\n", 1);
#ifdef KEEPSTATS
    result = NanoVis::writeToStatsFile(f, Tcl_DStringValue(&ds), 
                                       Tcl_DStringLength(&ds));
#else
    TRACE("clientinfo: %s", Tcl_DStringValue(&ds));
#endif
    Tcl_DStringFree(&ds);
    Tcl_DecrRefCount(listObjPtr);
    return result;
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
LegendCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " transfunc width height\"", (char*)NULL);
        return TCL_ERROR;
    }

    const char *name;
    name = Tcl_GetString(objv[1]);
    TransferFunction *tf;
    tf = NanoVis::getTransfunc(name);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "unknown transfer function \"", name, "\"",
                             (char*)NULL);
        return TCL_ERROR;
    }
    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (Volume::updatePending) {
        NanoVis::setVolumeRanges();
    }
    NanoVis::renderLegend(tf, Volume::valueMin, Volume::valueMax, w, h, name);
    return TCL_OK;
}

static int
ScreenBgColorOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    float rgb[3];
    if ((GetFloatFromObj(interp, objv[2], &rgb[0]) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &rgb[1]) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &rgb[2]) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::setBgColor(rgb);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   screen size <width> <height>
 *
 * Clients send this command to set the size of the rendering area.
 * Future images are generated at the specified width/height.
 * ----------------------------------------------------------------------
 */
static int
ScreenSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::resizeOffscreenBuffer(w, h);
    return TCL_OK;
}

static Rappture::CmdSpec screenOps[] = {
    {"bgcolor",  1, ScreenBgColorOp,  5, 5, "r g b",},
    {"size",     1, ScreenSizeOp, 4, 4, "width height",},
};
static int nScreenOps = NumCmdSpecs(screenOps);

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nScreenOps, screenOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
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
TransfuncCmd(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }

    const char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'd') && (strcmp(string, "define") == 0)) {
        if (objc != 5) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " define name colorMap alphaMap\"",
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
            Tcl_AppendResult(interp, "wrong # elements is colormap: should be ",
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
            double q[4];

            for (j=0; j < 4; j++) {
                if (Tcl_GetDoubleFromObj(interp, cmapv[i+j], &q[j]) != TCL_OK) {
                    return TCL_ERROR;
                }
                if ((q[j] < 0.0) || (q[j] > 1.0)) {
                    Tcl_AppendResult(interp, "bad colormap value \"",
                        Tcl_GetString(cmapv[i+j]),
                        "\": should be in the range 0-1", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            rFunc.define(q[0], q[1]);
            gFunc.define(q[0], q[2]);
            bFunc.define(q[0], q[3]);
        }
        for (i=0; i < wmapc; i += 2) {
            double q[2];
            int j;

            for (j=0; j < 2; j++) {
                if (Tcl_GetDoubleFromObj(interp, wmapv[i+j], &q[j]) != TCL_OK) {
                    return TCL_ERROR;
                }
                if ((q[j] < 0.0) || (q[j] > 1.0)) {
                    Tcl_AppendResult(interp, "bad alphamap value \"",
                        Tcl_GetString(wmapv[i+j]),
                        "\": should be in the range 0-1", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            wFunc.define(q[0], q[1]);
        }
        // sample the given function into discrete slots
        const int nslots = 256;
        float data[4*nslots];
        for (i=0; i < nslots; i++) {
            double x = double(i)/(nslots-1);
            data[4*i]   = rFunc.value(x);
            data[4*i+1] = gFunc.value(x);
            data[4*i+2] = bFunc.value(x);
            data[4*i+3] = wFunc.value(x);
        }
        // find or create this transfer function
        NanoVis::defineTransferFunction(Tcl_GetString(objv[2]), nslots, data);
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
UpCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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
VolumeAnimationCaptureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    int total;
    if (Tcl_GetIntFromObj(interp, objv[3], &total) != TCL_OK) {
        return TCL_ERROR;
    }
    VolumeInterpolator* interpolator;
    interpolator = NanoVis::volRenderer->getVolumeInterpolator();
    interpolator->start();
    if (interpolator->isStarted()) {
        const char *fileName = (objc < 5) ? NULL : Tcl_GetString(objv[4]);
        for (int frame_num = 0; frame_num < total; ++frame_num) {
            float fraction;

            fraction = ((float)frame_num) / (total - 1);
            TRACE("fraction : %f", fraction);
            //interpolator->update(((float)frame_num) / (total - 1));
            interpolator->update(fraction);

            int fboOrig;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

            NanoVis::bindOffscreenBuffer();  //enable offscreen render

            NanoVis::display();
            NanoVis::readScreen();

            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

            NanoVis::bmpWriteToFile(frame_num, fileName);
        }
    }
    return TCL_OK;
}

static int
VolumeAnimationClearOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    NanoVis::volRenderer->clearAnimatedVolumeInfo();
    return TCL_OK;
}

static int
VolumeAnimationStartOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    NanoVis::volRenderer->startVolumeAnimation();
    return TCL_OK;
}

static int
VolumeAnimationStopOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    NanoVis::volRenderer->stopVolumeAnimation();
    return TCL_OK;
}

static int
VolumeAnimationVolumesOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    std::vector<Volume *> volumes;
    if (GetVolumes(interp, objc - 3, objv + 3, &volumes) != TCL_OK) {
        return TCL_ERROR;
    }
    TRACE("parsing volume data identifier");
    Tcl_HashSearch iter;
    Tcl_HashEntry *hPtr;
    for (hPtr = Tcl_FirstHashEntry(&NanoVis::volumeTable, &iter); hPtr != NULL;
	 hPtr = Tcl_NextHashEntry(&iter)) {
	Volume *volPtr;
	volPtr = (Volume *)Tcl_GetHashValue(hPtr);
        NanoVis::volRenderer->addAnimatedVolume(volPtr);
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
VolumeAnimationOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeAnimationOps, 
		volumeAnimationOps, Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    TRACE("Data Loading");

    int nbytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *tag;
    tag = Tcl_GetString(objv[4]);
    Rappture::Buffer buf;
    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *bytes;
    size_t nBytes;

    bytes = buf.bytes();
    nBytes = buf.size();

    TRACE("Checking header[%.20s]", bytes);

    Volume *volPtr = NULL;

    if ((nBytes > 5) && (strncmp(bytes, "<HDR>", 5) == 0)) {
        TRACE("ZincBlende stream is in");
         //std::stringstream fdata(std::ios_base::out|std::ios_base::in|std::ios_base::binary);
        //fdata.write(buf.bytes(),buf.size());
        //vol = NvZincBlendeReconstructor::getInstance()->loadFromStream(fdata);

        volPtr = NvZincBlendeReconstructor::getInstance()->loadFromMemory((void*) buf.bytes());
        if (volPtr == NULL) {
            Tcl_AppendResult(interp, "can't get volume instance", (char *)NULL);
            return TCL_ERROR;
        }
        TRACE("finish loading");

        vrmath::Vector3f scale = volPtr->getPhysicalScaling();
        vrmath::Vector3f loc(scale);
        loc *= -0.5;
        volPtr->location(loc);

	int isNew;
	Tcl_HashEntry *hPtr;
        hPtr = Tcl_CreateHashEntry(&NanoVis::volumeTable, tag, &isNew);
	if (!isNew) {
            Tcl_AppendResult(interp, "volume \"", tag, "\" already exists.",
			     (char *)NULL);
            return TCL_ERROR;
	}
	Tcl_SetHashValue(hPtr, volPtr);
	volPtr->name(Tcl_GetHashKey(&NanoVis::volumeTable, hPtr));
    } else {
	if ((nBytes > 5) && (strncmp(bytes, "<ODX>", 5) == 0)) {
	    bytes += 5;
	    nBytes -= 5;
        }
        TRACE("DX loading...");
        std::stringstream fdata;
        fdata.write(bytes, nBytes);
	if (nBytes <= 0) {
	    ERROR("data buffer is empty");
	    abort();
	}
        Rappture::Outcome context;
        volPtr = load_volume_stream(context, tag, fdata);
        if (volPtr == NULL) {
            Tcl_AppendResult(interp, context.remark(), (char*)NULL);
            return TCL_ERROR;
        }
    }

    //
    // BE CAREFUL: Set the number of slices to something slightly different
    // for each volume.  If we have identical volumes at exactly the same
    // position with exactly the same number of slices, the second volume will
    // overwrite the first, so the first won't appear at all.
    //
    if (volPtr != NULL) {
        //volPtr->numSlices(512-n);
        //volPtr->numSlices(256-n);
        volPtr->disableCutplane(0);
        volPtr->disableCutplane(1);
        volPtr->disableCutplane(2);
        volPtr->transferFunction(NanoVis::getTransfunc("default"));
	volPtr->visible(true);

        char info[1024];
	ssize_t nWritten;

        if (Volume::updatePending) {
            NanoVis::setVolumeRanges();
        }

        // FIXME: strlen(info) is the return value of sprintf
        sprintf(info, "nv>data tag %s min %g max %g vmin %g vmax %g\n", tag, 
		volPtr->wAxis.min(), volPtr->wAxis.max(),
                Volume::valueMin, Volume::valueMax);
        nWritten  = write(1, info, strlen(info));
	assert(nWritten == (ssize_t)strlen(info));
    }
    return TCL_OK;
}

static int
VolumeDataStateOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->dataEnabled(state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeDataOps[] = {
    {"follows",   1, VolumeDataFollowsOp, 5, 5, "size tag",},
    {"state",     1, VolumeDataStateOp,   4, 0, "bool ?indices?",},
};
static int nVolumeDataOps = NumCmdSpecs(volumeDataOps);

static int
VolumeDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeDataOps, volumeDataOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    int i;

    for (i = 2; i < objc; i++) {
	Volume *volPtr;

	if (GetVolumeFromObj(interp, objv[i], &volPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::removeVolume(volPtr);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
VolumeExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    bool value;
    Volume *volPtr;

    value = false;
    if (GetVolumeFromObj(NULL, objv[2], &volPtr) == TCL_OK) {
	value = true;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (int)value);
    return TCL_OK;
}

static int
VolumeNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
	      Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    Tcl_HashEntry *hPtr; 
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&NanoVis::volumeTable, &iter); hPtr != NULL; 
	 hPtr = Tcl_NextHashEntry(&iter)) {
	Volume *volPtr;
	volPtr = (Volume *)Tcl_GetHashValue(hPtr);
	Tcl_Obj *objPtr;
	objPtr = Tcl_NewStringObj(volPtr->name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
VolumeOutlineColorOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    float rgb[3];
    if (GetColor(interp, objc - 3, objv + 3, rgb) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 6, objv + 6, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->setOutlineColor(rgb);
    }
    return TCL_OK;
}

static int
VolumeOutlineStateOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->outline(state);
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
VolumeOutlineOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeOutlineOps, volumeOutlineOps,
        Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeShadingAmbientOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    float ambient;
    if (GetFloatFromObj(interp, objv[3], &ambient) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ambient < 0.0f || ambient > 1.0f) {
        WARN("Invalid ambient coefficient requested: %g, should be [0,1]", ambient);
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->ambient(ambient);
    }
    return TCL_OK;
}

static int
VolumeShadingDiffuseOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    float diffuse;
    if (GetFloatFromObj(interp, objv[3], &diffuse) != TCL_OK) {
        return TCL_ERROR;
    }
    if (diffuse < 0.0f || diffuse > 1.0f) {
        WARN("Invalid diffuse coefficient requested: %g, should be [0,1]", diffuse);
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->diffuse(diffuse);
    }
    return TCL_OK;
}

static int
VolumeShadingIsosurfaceOp(ClientData clientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *const *objv)
{
    bool iso_surface;
    if (GetBooleanFromObj(interp, objv[3], &iso_surface) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->isosurface(iso_surface);
    }
    return TCL_OK;
}

static int
VolumeShadingLight2SideOp(ClientData clientData, Tcl_Interp *interp, int objc,
                          Tcl_Obj *const *objv)
{
    bool twoSidedLighting;
    if (GetBooleanFromObj(interp, objv[3], &twoSidedLighting) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->twoSidedLighting(twoSidedLighting);
    }
    return TCL_OK;
}

static int
VolumeShadingOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{

    float opacity;
    if (GetFloatFromObj(interp, objv[3], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    TRACE("set opacity %f", opacity);
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->opacityScale(opacity);
    }
    return TCL_OK;
}

static int
VolumeShadingSpecularOp(ClientData clientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *const *objv)
{
    float specular;
    if (GetFloatFromObj(interp, objv[3], &specular) != TCL_OK) {
        return TCL_ERROR;
    }
    if (specular < 0.0f || specular > 1.0f) {
        WARN("Invalid specular coefficient requested: %g, should be [0,1]", specular);
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->specularLevel(specular);
    }
    return TCL_OK;
}

static int
VolumeShadingSpecularExpOp(ClientData clientData, Tcl_Interp *interp, int objc,
                           Tcl_Obj *const *objv)
{
    float specularExp;
    if (GetFloatFromObj(interp, objv[3], &specularExp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (specularExp < 0.0f || specularExp > 128.0f) {
        WARN("Invalid specular exponent requested: %g, should be [0,128]", specularExp);
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
        (*iter)->specularExponent(specularExp);
    }
    return TCL_OK;
}

static int
VolumeShadingTransFuncOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    TransferFunction *tfPtr;
    const char *name = Tcl_GetString(objv[3]);
    tfPtr = NanoVis::getTransfunc(name);
    if (tfPtr == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", name,
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	TRACE("setting %s with transfer function %s", (*iter)->name(),
	       tfPtr->name());
        (*iter)->transferFunction(tfPtr);
#ifdef USE_POINTSET_RENDERER
        // TBD..
        if ((*iter)->pointsetIndex != -1) {
            NanoVis::pointSet[(*iter)->pointsetIndex]->updateColor(tfPtr->getData(), 256);
        }
#endif
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeShadingOps[] = {
    {"ambient",       1, VolumeShadingAmbientOp,     4, 0, "value ?indices?",},
    {"diffuse",       1, VolumeShadingDiffuseOp,     4, 0, "value ?indices?",},
    {"isosurface",    1, VolumeShadingIsosurfaceOp,  4, 0, "bool ?indices?",},
    {"light2side",    1, VolumeShadingLight2SideOp,  4, 0, "bool ?indices?",},
    {"opacity",       1, VolumeShadingOpacityOp,     4, 0, "value ?indices?",},
    {"specularExp",   9, VolumeShadingSpecularExpOp, 4, 0, "value ?indices?",},
    {"specularLevel", 9, VolumeShadingSpecularOp,    4, 0, "value ?indices?",},
    {"transfunc",     1, VolumeShadingTransFuncOp,   4, 0, "funcName ?indices?",},
};
static int nVolumeShadingOps = NumCmdSpecs(volumeShadingOps);

static int
VolumeShadingOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeShadingOps, volumeShadingOps,
        Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeStateOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 3, objv + 3, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
	(*iter)->visible(state);
    }
    return TCL_OK;
}

static int
VolumeTestOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    // Find the first volume in the vector.
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    hPtr = Tcl_FirstHashEntry(&NanoVis::volumeTable, &iter); 
    if (hPtr != NULL) {
	Volume *volPtr;
	volPtr = (Volume *)Tcl_GetHashValue(hPtr);
        volPtr->dataEnabled(false);
        volPtr->visible(false);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeOps[] = {
    {"animation", 2, VolumeAnimationOp,   3, 0, "oper ?args?",},
    {"data",      2, VolumeDataOp,        3, 0, "oper ?args?",},
    {"delete",    2, VolumeDeleteOp,      3, 0, "?name...?",},
    {"exists",    1, VolumeExistsOp,      3, 3, "name",},
    {"names",     1, VolumeNamesOp,       2, 3, "?pattern?",},
    {"outline",   1, VolumeOutlineOp,     3, 0, "oper ?args?",},
    {"shading",   2, VolumeShadingOp,     3, 0, "oper ?args?",},
    {"state",     2, VolumeStateOp,       3, 0, "bool ?indices?",},
    {"test2",     1, VolumeTestOp,        2, 2, "",},
};
static int nVolumeOps = NumCmdSpecs(volumeOps);

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
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
VolumeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeOps, volumeOps,
        Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
HeightMapDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    int nBytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *tag;
    tag = Tcl_GetString(objv[4]);
    int isNew;
    Tcl_HashEntry *hPtr;

    Rappture::Buffer buf;
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    Rappture::Unirect2d data(1);
    if (data.parseBuffer(interp, buf) != TCL_OK) {
	return TCL_ERROR;
    }
    if (data.nValues() == 0) {
	Tcl_AppendResult(interp, "no data found in stream", (char *)NULL);
	return TCL_ERROR;
    }
    if (!data.isInitialized()) {
	return TCL_ERROR;
    }
    HeightMap* hmPtr;
    hPtr = Tcl_CreateHashEntry(&NanoVis::heightmapTable, tag, &isNew);
    if (isNew) {
	hmPtr = new HeightMap();
	Tcl_SetHashValue(hPtr, hmPtr);
    } else {
	hmPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
    }
    TRACE("Number of heightmaps=%d", NanoVis::heightmapTable.numEntries);
    // Must set units before the heights.
    hmPtr->xAxis.units(data.xUnits());
    hmPtr->yAxis.units(data.yUnits());
    hmPtr->zAxis.units(data.vUnits());
    hmPtr->wAxis.units(data.yUnits());
    hmPtr->setHeight(data.xMin(), data.yMin(), data.xMax(), data.yMax(), 
		     data.xNum(), data.yNum(), data.transferValues());
    hmPtr->transferFunction(NanoVis::getTransfunc("default"));
    hmPtr->setVisible(true);
    hmPtr->setLineContourVisible(true);
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
HeightMapDataVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *> imap;
    if (GetHeightMaps(interp, objc - 4, objv + 4, &imap) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *>::iterator iter;
    for (iter = imap.begin(); iter != imap.end(); iter++) {
        (*iter)->setVisible(visible);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static Rappture::CmdSpec heightMapDataOps[] = {
    {"follows",  1, HeightMapDataFollowsOp, 5, 5, "size tag",},
    {"visible",  1, HeightMapDataVisibleOp, 4, 0, "bool ?indices?",},
};
static int nHeightMapDataOps = NumCmdSpecs(heightMapDataOps);

static int
HeightMapDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapDataOps, heightMapDataOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}


static int
HeightMapLineContourColorOp(ClientData clientData, Tcl_Interp *interp, int objc,
                            Tcl_Obj *const *objv)
{
    float rgb[3];
    if (GetColor(interp, objc - 3, objv + 3, rgb) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *> imap;
    if (GetHeightMaps(interp, objc - 6, objv + 6, &imap) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *>::iterator iter;
    for (iter = imap.begin(); iter != imap.end(); iter++) {
        (*iter)->setLineContourColor(rgb);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
HeightMapLineContourVisibleOp(ClientData clientData, Tcl_Interp *interp, 
			      int objc, Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *> imap;
    if (GetHeightMaps(interp, objc - 4, objv + 4, &imap) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *>::iterator iter;
    for (iter = imap.begin(); iter != imap.end(); iter++) {
        (*iter)->setLineContourVisible(visible);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static Rappture::CmdSpec heightMapLineContourOps[] = {
    {"color",   1, HeightMapLineContourColorOp,   4, 4, "length",},
    {"visible", 1, HeightMapLineContourVisibleOp, 4, 0, "bool ?indices?",},
};
static int nHeightMapLineContourOps = NumCmdSpecs(heightMapLineContourOps);

static int 
HeightMapLineContourOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapLineContourOps,
        heightMapLineContourOps, Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
HeightMapCullOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    RenderContext::CullMode mode;
    if (GetCullMode(interp, objv[2], &mode) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setCullMode(mode);
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
HeightMapCreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    const char *tag;
    tag = Tcl_GetString(objv[2]);
    Tcl_HashEntry *hPtr;
    int isNew;
    hPtr = Tcl_CreateHashEntry(&NanoVis::heightmapTable, tag, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "heightmap \"", tag, "\" already exists.",
			 (char *)NULL);
	return TCL_ERROR;
    }
    HeightMap *hmPtr;
    /* heightmap create xmin ymin xmax ymax xnum ynum values */
    hmPtr = CreateHeightMap(clientData, interp, objc - 3, objv + 3);
    if (hmPtr == NULL) {
        return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, hmPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), tag, -1);;
    NanoVis::eventuallyRedraw();
    TRACE("Number of heightmaps=%d", NanoVis::heightmapTable.numEntries);
    return TCL_OK;
}

static int
HeightMapLegendOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    HeightMap *hmPtr;
    if (GetHeightMapFromObj(interp, objv[2], &hmPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *tag;
    tag = Tcl_GetString(objv[2]);
    TransferFunction *tfPtr;
    tfPtr = hmPtr->transferFunction();
    if (tfPtr == NULL) {
        Tcl_AppendResult(interp, "no transfer function defined for heightmap"
			 " \"", tag, "\"", (char*)NULL);
        return TCL_ERROR;
    }
    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[3], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (HeightMap::updatePending) {
        NanoVis::setHeightmapRanges();
    }
    NanoVis::renderLegend(tfPtr, HeightMap::valueMin, HeightMap::valueMax, 
                          w, h, tag);
    return TCL_OK;
}

static int
HeightMapPolygonOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    RenderContext::PolygonMode mode;
    if (GetPolygonMode(interp, objv[2], &mode) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setPolygonMode(mode);
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
HeightMapShadingOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    RenderContext::ShadingModel model;
    if (GetShadingModel(interp, objv[2], &model) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::renderContext->setShadingModel(model);
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
HeightMapTransFuncOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    const char *name;
    name = Tcl_GetString(objv[2]);
    TransferFunction *tfPtr;
    tfPtr = NanoVis::getTransfunc(name);
    if (tfPtr == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", name,
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    std::vector<HeightMap *> imap;
    if (GetHeightMaps(interp, objc - 3, objv + 3, &imap) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *>::iterator iter;
    for (iter = imap.begin(); iter != imap.end(); iter++) {
        (*iter)->transferFunction(tfPtr);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}


static int
HeightMapOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    float opacity;
    if (GetFloatFromObj(interp, objv[2], &opacity) != TCL_OK) {
	return TCL_ERROR;
    }
    std::vector<HeightMap *> heightmaps;
    if (GetHeightMaps(interp, objc - 3, objv + 3, &heightmaps) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<HeightMap *>::iterator iter;
    for (iter = heightmaps.begin(); iter != heightmaps.end(); iter++) {
        (*iter)->opacity(opacity);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static Rappture::CmdSpec heightMapOps[] = {
    {"create",       2, HeightMapCreateOp,      10, 10, "tag xmin ymin xmax ymax xnum ynum values",},
    {"cull",         2, HeightMapCullOp,        3, 3, "mode",},
    {"data",         1, HeightMapDataOp,        3, 0, "oper ?args?",},
    {"legend",       2, HeightMapLegendOp,      5, 5, "index width height",},
    {"linecontour",  2, HeightMapLineContourOp, 2, 0, "oper ?args?",},
    {"opacity",      1, HeightMapOpacityOp,     3, 0, "value ?heightmap...? ",},
    {"polygon",      1, HeightMapPolygonOp,     3, 3, "mode",},
    {"shading",      1, HeightMapShadingOp,     3, 3, "model",},
    {"transfunc",    2, HeightMapTransFuncOp,   3, 0, "name ?heightmap...?",},
};
static int nHeightMapOps = NumCmdSpecs(heightMapOps);

static int
HeightMapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightMapOps, heightMapOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
GridAxisColorOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
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
GridAxisNameOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int axis;
    if (GetAxisFromObj(interp, objv[2], &axis) != TCL_OK) {
        return TCL_ERROR;
    }
    if (NanoVis::grid != NULL) {
        Axis *axisPtr;

        axisPtr = NULL;     /* Suppress compiler warning. */
        switch (axis) {
        case 0: axisPtr = &NanoVis::grid->xAxis; break;
        case 1: axisPtr = &NanoVis::grid->yAxis; break;
        case 2: axisPtr = &NanoVis::grid->zAxis; break;
        }
        axisPtr->name(Tcl_GetString(objv[3]));
        axisPtr->units(Tcl_GetString(objv[4]));
    }
    return TCL_OK;
}

static int
GridLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
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
        NanoVis::grid->setLineColor(r, g, b, a);
    }
    return TCL_OK;
}

static int
GridVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
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
    {"axisname",   5, GridAxisNameOp,   5, 5, "index title units",},
    {"linecolor",  7, GridLineColorOp,  5, 6, "r g b ?a?",},
    {"visible",    1, GridVisibleOp,    3, 3, "bool",},
};
static int nGridOps = NumCmdSpecs(gridOps);

static int
GridCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nGridOps, gridOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
AxisCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'v') && (strcmp(string, "visible") == 0)) {
        bool visible;

        if (GetBooleanFromObj(interp, objv[2], &visible) != TCL_OK) {
            return TCL_ERROR;
        }
        NanoVis::axisOn = visible;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

#ifdef PLANE_CMD
static int 
PlaneAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    TRACE("load plane for 2D visualization command");

    int index, w, h;
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
            Tcl_GetString(objv[0]), " plane_index w h \"", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (index >= NanoVis::numPlanes) {
        Tcl_AppendResult(interp, "Invalid plane_index", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK) {
        return TCL_ERROR;
    }

    //Now read w*h*4 bytes. The server expects the plane to be a stream of
    //floats
    char *tmp = new char[int(w*h*sizeof(float))];
    if (tmp == NULL) {
        Tcl_AppendResult(interp, "can't allocate stream data", (char *)NULL);
        return TCL_ERROR;
    }
    bzero(tmp, w*h*4);
    int status = read(fileno(NanoVis::stdin), tmp, w*h*sizeof(float));
    if (status <= 0) {
        delete[] tmp;
        Tcl_AppendResult(interp, "Failed to read image data for plane", (char*)NULL);
        return TCL_ERROR;
    }
    NanoVis::plane[index] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, (float*)tmp);
    delete[] tmp;
    return TCL_OK;
}

static int
PlaneLinkOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TRACE("link the plane to the 2D renderer command");

    int plane_index;

    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " plane_index transfunc_name \"", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &plane_index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (plane_index >= NanoVis::numPlanes) {
        Tcl_AppendResult(interp, "Invalid plane_index", (char*)NULL);
        return TCL_ERROR;
    }
    NanoVis::planeRenderer->addPlane(NanoVis::plane[plane_index],
                                     NanoVis::getTransfunc(Tcl_GetString(objv[2])));
    return TCL_OK;
}

//Enable a 2D plane for render
//The plane_index is the index mantained in the 2D plane renderer
static int
PlaneEnableOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    TRACE("enable a plane so the 2D renderer can render it command");

    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
            Tcl_GetString(objv[0]), " plane_index mode \"", (char*)NULL);
        return TCL_ERROR;
    }
    int plane_index;
    if (Tcl_GetIntFromObj(interp, objv[1], &plane_index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (plane_index >= NanoVis::numPlanes) {
        Tcl_AppendResult(interp, "Invalid plane_index", (char*)NULL);
        return TCL_ERROR;
    } else if (plane_index < 0) {
        plane_index = -1;
    }

    NanoVis::planeRenderer->setActivePlane(plane_index);
    return TCL_OK;
}

static Rappture::CmdSpec planeOps[] = {
    {"active",     2, PlaneEnableOp,    3, 3, "planeIdx",},
    {"add",        2, PlaneAddOp,       5, 5, "planeIdx width height",},
    {"link",       1, PlaneLinkOp,      4, 4, "planeIdx transfunc_name",},
};
static int nPlaneOps = NumCmdSpecs(planeOps);

static int
PlaneCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nPlaneOps, planeOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

#endif /*PLANE_CMD*/

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
Unirect2dCmd(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Rappture::Unirect2d *dataPtr = (Rappture::Unirect2d *)clientData;

    return dataPtr->loadData(interp, objc, objv);
}

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
Unirect3dCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Rappture::Unirect3d *dataPtr = (Rappture::Unirect3d *)clientData;

    return dataPtr->loadData(interp, objc, objv);
}

Tcl_Interp *
initTcl()
{
    /*
     * Ideally the connection is authenticated by nanoscale.  I still like the
     * idea of creating a non-safe master interpreter with a safe slave
     * interpreter.  Alias all the nanovis commands in the slave. That way we
     * can still run Tcl code within nanovis.  The eventual goal is to create
     * a test harness through the interpreter for nanovis.
     */
    Tcl_Interp *interp;
    interp = Tcl_CreateInterp();
    /*
    Tcl_MakeSafe(interp);
    */
    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "clientinfo",  ClientInfoCmd,  NULL, NULL);
    Tcl_CreateObjCommand(interp, "cutplane",    CutplaneCmd,    NULL, NULL);
    if (FlowCmdInitProc(interp) != TCL_OK) {
	return NULL;
    }
    Tcl_CreateObjCommand(interp, "grid",        GridCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",   HeightMapCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      NULL, NULL);
#ifdef PLANE_CMD
    Tcl_CreateObjCommand(interp, "plane",       PlaneCmd,       NULL, NULL);
#endif
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "snapshot",    SnapshotCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "transfunc",   TransfuncCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "unirect2d",   Unirect2dCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "unirect3d",   Unirect3dCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "up",          UpCmd,          NULL, NULL);
    Tcl_CreateObjCommand(interp, "volume",      VolumeCmd,      NULL, NULL);

    Tcl_InitHashTable(&NanoVis::volumeTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&NanoVis::heightmapTable, TCL_STRING_KEYS);
    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        WARN("bad default transfer function:\n%s", 
	     Tcl_GetStringResult(interp));
    }
    return interp;
}


