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
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

/*
 * TODO:  In no particular order...
 *        o Use Tcl command option parser to reduce size of procedures, remove
 *          lots of extra error checking code. (almost there)
 *        o Add bookkeeping for volumes, heightmaps, flows, etc. to track
 *          1) simulation # 2) include/exclude.  The include/exclude
 *          is to indicate whether the item should contribute to the overall
 *          limits of the axes.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>                     /* Needed for getpid, gethostname,
                                         * write, etc. */
#include <tcl.h>

#include <RpField1D.h>
#include <RpEncode.h>
#include <RpBuffer.h>

#include <vrmath/Vector3f.h>

#include "nanovisServer.h"
#include "nanovis.h"
#include "ReadBuffer.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif
#include "Command.h"
#include "CmdProc.h"
#include "FlowCmd.h"
#include "dxReader.h"
#ifdef USE_VTK
#include "VtkDataSetReader.h"
#else
#include "VtkReader.h"
#endif
#include "BMPWriter.h"
#include "PPMWriter.h"
#include "Grid.h"
#include "HeightMap.h"
#include "Camera.h"
#include "ZincBlendeReconstructor.h"
#include "OrientationIndicator.h"
#include "Unirect.h"
#include "Volume.h"
#include "VolumeRenderer.h"
#include "Trace.h"

using namespace nv;
using namespace nv::graphics;
using namespace vrmath;

// default transfer function
static const char def_transfunc[] =
    "transfunc define default {\n\
  0.00  0 0 1\n\
  0.25  0 1 1\n\
  0.50  0 1 0\n\
  0.75  1 1 0\n\
  1.00  1 0 0\n\
} {\n\
  0.000000 0.0\n\
  0.107847 0.0\n\
  0.107857 1.0\n\
  0.177857 1.0\n\
  0.177867 0.0\n\
  0.250704 0.0\n\
  0.250714 1.0\n\
  0.320714 1.0\n\
  0.320724 0.0\n\
  0.393561 0.0\n\
  0.393571 1.0\n\
  0.463571 1.0\n\
  0.463581 0.0\n\
  0.536419 0.0\n\
  0.536429 1.0\n\
  0.606429 1.0\n\
  0.606439 0.0\n\
  0.679276 0.0\n\
  0.679286 1.0\n\
  0.749286 1.0\n\
  0.749296 0.0\n\
  0.822133 0.0\n\
  0.822143 1.0\n\
  0.892143 1.0\n\
  0.892153 0.0\n\
  1.000000 0.0\n\
}";

static int lastCmdStatus;

#ifdef USE_THREADS
void
nv::queueResponse(const void *bytes, size_t len, 
              Response::AllocationType allocType,
              Response::ResponseType type)
{
    Response *response = new Response(type);
    response->setMessage((unsigned char *)bytes, len, allocType);
    g_queue->enqueue(response);
}
#else

ssize_t
nv::SocketWrite(const void *bytes, size_t len)
{
    size_t ofs = 0;
    ssize_t bytesWritten;
    while ((bytesWritten = write(g_fdOut, (const char *)bytes + ofs, len - ofs)) > 0) {
        ofs += bytesWritten;
        if (ofs == len)
            break;
    }
    if (bytesWritten < 0) {
        ERROR("write: %s", strerror(errno));
    }
    return bytesWritten;
}

#endif  /*USE_THREADS*/

bool
nv::SocketRead(char *bytes, size_t len)
{
    ReadBuffer::BufferStatus status;
    status = g_inBufPtr->followingData((unsigned char *)bytes, len);
    TRACE("followingData status: %d", status);
    return (status == ReadBuffer::OK);
}

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    int result;
#ifdef WANT_TRACE
    char *str = Tcl_DStringValue(dsPtr);
    std::string cmd(str);
    cmd.erase(cmd.find_last_not_of(" \n\r\t")+1);
    TRACE("command %lu: '%s'", g_stats.nCommands+1, cmd.c_str());
#endif
    lastCmdStatus = TCL_OK;
    result = Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), 
                        Tcl_DStringLength(dsPtr), 
                        TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);
    Tcl_DStringSetLength(dsPtr, 0);
    if (lastCmdStatus == TCL_BREAK) {
        return TCL_BREAK;
    }
    lastCmdStatus = result;
    if (result != TCL_OK) {
        TRACE("Error: %d", result);
    }
    return result;
}

int
nv::GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, bool *boolPtr)
{
    int value;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *boolPtr = (bool)value;
    return TCL_OK;
}

int
nv::GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, float *valuePtr)
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
    HeightMap *heightMap = new HeightMap();
    heightMap->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, heights);
    heightMap->transferFunction(NanoVis::getTransferFunction("default"));
    heightMap->setVisible(true);
    heightMap->setLineContourVisible(true);
    delete [] heights;
    return heightMap;
}

static int
GetHeightMapFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, HeightMap **hmPtrPtr)
{
    const char *string = Tcl_GetString(objPtr);

    NanoVis::HeightMapHashmap::iterator itr = NanoVis::heightMapTable.find(string);
    if (itr == NanoVis::heightMapTable.end()) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a heightmap named \"",
                         string, "\"", (char*)NULL);
        }
        return TCL_ERROR;
    }
    *hmPtrPtr = itr->second;
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
int
nv::GetVolumeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Volume **volPtrPtr)
{
    const char *string = Tcl_GetString(objPtr);

    NanoVis::VolumeHashmap::iterator itr = NanoVis::volumeTable.find(string);
    if (itr == NanoVis::volumeTable.end()) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a volume named \"",
                         string, "\"", (char*)NULL);
        }
        return TCL_ERROR;
    }
    *volPtrPtr = itr->second;
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
        NanoVis::VolumeHashmap::iterator itr;
        for (itr = NanoVis::volumeTable.begin();
             itr != NanoVis::volumeTable.end(); ++itr) {
            vectorPtr->push_back(itr->second);
        }
    } else {
        // Get the volumes associated with the given index arguments.
        for (int n = 0; n < objc; n++) {
            Volume *volume;
            if (GetVolumeFromObj(interp, objv[n], &volume) != TCL_OK) {
                return TCL_ERROR;
            }
            vectorPtr->push_back(volume);
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
        NanoVis::HeightMapHashmap::iterator itr;
        for (itr = NanoVis::heightMapTable.begin();
             itr != NanoVis::heightMapTable.end(); ++itr) {
            vectorPtr->push_back(itr->second);
        }
    } else {
        for (int n = 0; n < objc; n++) {
            HeightMap *heightMap;
            if (GetHeightMapFromObj(interp, objv[n], &heightMap) != TCL_OK) {
                return TCL_ERROR;
            }
            vectorPtr->push_back(heightMap);
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
nv::GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr)
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
 * buffer.  The buffer must have already been allocated for nBytes.  The 
 * buffer is then decompressed and decoded.
 */
int
nv::GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes)
{
    if (!SocketRead((char *)buf.bytes(), nBytes)) {
        return TCL_ERROR;
    }
    buf.count(nBytes);
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
    //NanoVis::orientCamera(phi, theta, psi);
    //return TCL_OK;
    Tcl_AppendResult(interp, "The 'camera angle' command is deprecated, use 'camera orient'", (char*)NULL);
    return TCL_ERROR;
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
    NanoVis::orientCamera(quat);
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
    NanoVis::panCamera(x, y);
    return TCL_OK;
}

static int
CameraPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Vector3f pos;
    if ((GetFloatFromObj(interp, objv[2], &pos.x) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[3], &pos.y) != TCL_OK) ||
        (GetFloatFromObj(interp, objv[4], &pos.z) != TCL_OK)) {
        return TCL_ERROR;
    }

    NanoVis::setCameraPosition(pos);
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
    NanoVis::zoomCamera(z);
    return TCL_OK;
}

static CmdSpec cameraOps[] = {
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

    proc = GetOpFromObj(interp, nCameraOps, cameraOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
SnapshotCmd(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    int origWidth, origHeight, width, height;

    origWidth = NanoVis::winWidth;
    origHeight = NanoVis::winHeight;
    width = 2048;
    height = 2048;

    NanoVis::resizeOffscreenBuffer(width, height);
    NanoVis::bindOffscreenBuffer();
    NanoVis::render();
    NanoVis::readScreen();
#ifdef USE_THREADS
    queuePPM(g_queue, "nv>image -type print -bytes",
             NanoVis::screenBuffer, width, height);
#else
    writePPM(g_fdOut, "nv>image -type print -bytes",
             NanoVis::screenBuffer, width, height);
#endif
    NanoVis::resizeOffscreenBuffer(origWidth, origHeight);

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
        (*iter)->setCutplanePosition(axis, relval);
    }
    return TCL_OK;
}

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

static int
CutplaneVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc,
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
        (*iter)->cutplanesVisible(state);
    }
    return TCL_OK;
}

static CmdSpec cutplaneOps[] = {
    {"position", 1, CutplanePositionOp, 4, 0, "relval axis ?indices?",},
    {"state",    1, CutplaneStateOp,    4, 0, "bool axis ?indices?",},
    {"visible",  1, CutplaneVisibleOp,  3, 0, "bool ?indices?",},
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

    proc = GetOpFromObj(interp, nCutplaneOps, cutplaneOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    /* Use the initial client key value pairs as the parts for a generating
     * a unique file name. */
    int f = getStatsFile(interp, objv[1]);
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
    strcpy(buf, ctime(&g_stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(buf, -1));
    /* date_secs */
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("date_secs", 9));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewLongObj(g_stats.start.tv_sec));
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
    result = writeToStatsFile(f, Tcl_DStringValue(&ds), 
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

    const char *tfName = Tcl_GetString(objv[1]);
    TransferFunction *tf = NanoVis::getTransferFunction(tfName);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "unknown transfer function \"", tfName, "\"",
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
    NanoVis::renderLegend(tf, Volume::valueMin, Volume::valueMax, w, h, tfName);
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

static CmdSpec screenOps[] = {
    {"bgcolor",  1, ScreenBgColorOp,  5, 5, "r g b",},
    {"size",     1, ScreenSizeOp, 4, 4, "width height",},
};
static int nScreenOps = NumCmdSpecs(screenOps);

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nScreenOps, screenOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    NanoVis::setCameraUpdir(Camera::AxisDirection((axis+1)*sign));
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
    VolumeInterpolator *interpolator =
        NanoVis::volRenderer->getVolumeInterpolator();
    interpolator->start();
    if (interpolator->isStarted()) {
        const char *dirName = (objc < 5) ? NULL : Tcl_GetString(objv[4]);
        for (int frameNum = 0; frameNum < total; ++frameNum) {
            float fraction;

            fraction = ((float)frameNum) / (total - 1);
            TRACE("fraction : %f", fraction);
            interpolator->update(fraction);

            int fboOrig;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

            NanoVis::bindOffscreenBuffer();  //enable offscreen render

            NanoVis::render();
            NanoVis::readScreen();

            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

            /* FIXME: this function requires 4-byte aligned RGB rows,
             * but screen buffer is now 1 byte aligned for PPM images.
             */
            writeBMPFile(frameNum, dirName,
                         NanoVis::screenBuffer,
                         NanoVis::winWidth, NanoVis::winHeight);
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
    NanoVis::VolumeHashmap::iterator itr;
    for (itr = NanoVis::volumeTable.begin();
         itr != NanoVis::volumeTable.end(); ++itr) {
        NanoVis::volRenderer->addAnimatedVolume(itr->second);
    }
    return TCL_OK;
}

static CmdSpec volumeAnimationOps[] = {
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

    proc = GetOpFromObj(interp, nVolumeAnimationOps, 
                        volumeAnimationOps, CMDSPEC_ARG2, objc, objv, 0);
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
    const char *tag = Tcl_GetString(objv[4]);

    Rappture::Buffer buf(nbytes);
    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *bytes = buf.bytes();
    size_t nBytes = buf.size();

    TRACE("Checking header[%.20s]", bytes);

    Volume *volume = NULL;

    if ((nBytes > 5) && (strncmp(bytes, "<HDR>", 5) == 0)) {
        TRACE("ZincBlende Stream loading...");
        volume = (Volume *)ZincBlendeReconstructor::getInstance()->loadFromMemory(bytes);
        if (volume == NULL) {
            Tcl_AppendResult(interp, "can't get volume instance", (char *)NULL);
            return TCL_ERROR;
        }
        TRACE("finish loading");

        NanoVis::VolumeHashmap::iterator itr = NanoVis::volumeTable.find(tag);
        if (itr != NanoVis::volumeTable.end()) {
            Tcl_AppendResult(interp, "volume \"", tag, "\" already exists.",
                             (char *)NULL);
            return TCL_ERROR;
        }
        NanoVis::volumeTable[tag] = volume;
        volume->name(tag);
    } else if ((nBytes > 14) && (strncmp(bytes, "# vtk DataFile", 14) == 0)) {
        TRACE("VTK loading...");
        if (nBytes <= 0) {
            ERROR("data buffer is empty");
            abort();
        }
#ifdef USE_VTK
        volume = load_vtk_volume_stream(tag, bytes, nBytes);
#else
        std::stringstream fdata;
        fdata.write(bytes, nBytes);
        volume = load_vtk_volume_stream(tag, fdata);
#endif
        if (volume == NULL) {
            Tcl_AppendResult(interp, "Failed to load VTK file", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        // **Deprecated** OpenDX format
        if ((nBytes > 5) && (strncmp(bytes, "<ODX>", 5) == 0)) {
            bytes += 5;
            nBytes -= 5;
        }
        TRACE("DX loading...");
        if (nBytes <= 0) {
            ERROR("data buffer is empty");
            abort();
        }
        std::stringstream fdata;
        fdata.write(bytes, nBytes);
        volume = load_dx_volume_stream(tag, fdata);
        if (volume == NULL) {
            Tcl_AppendResult(interp, "Failed to load DX file", (char*)NULL);
            return TCL_ERROR;
        }
    }

    if (volume != NULL) {
        volume->disableCutplane(0);
        volume->disableCutplane(1);
        volume->disableCutplane(2);
        volume->transferFunction(NanoVis::getTransferFunction("default"));
        volume->visible(true);

        if (Volume::updatePending) {
            NanoVis::setVolumeRanges();
        }

        char info[1024];
        // FIXME: strlen(info) is the return value of sprintf
        int cmdLength = 
            sprintf(info, "nv>data tag %s min %g max %g vmin %g vmax %g\n", tag, 
                    volume->wAxis.min(), volume->wAxis.max(),
                    Volume::valueMin, Volume::valueMax);
#ifdef USE_THREADS
        queueResponse(info, cmdLength, Response::VOLATILE);
#else
        if (SocketWrite(info, (size_t)cmdLength) != (ssize_t)cmdLength) {
            ERROR("Short write");
            return TCL_ERROR;
        }
#endif
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

static CmdSpec volumeDataOps[] = {
    {"follows",   1, VolumeDataFollowsOp, 5, 5, "nbytes tag",},
    {"state",     1, VolumeDataStateOp,   4, 0, "bool ?indices?",},
};
static int nVolumeDataOps = NumCmdSpecs(volumeDataOps);

static int
VolumeDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nVolumeDataOps, volumeDataOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    for (int i = 2; i < objc; i++) {
        Volume *volume;
        if (GetVolumeFromObj(interp, objv[i], &volume) != TCL_OK) {
            return TCL_ERROR;
        }
        NanoVis::removeVolume(volume);
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
VolumeExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    bool value;
    Volume *volume;

    value = false;
    if (GetVolumeFromObj(NULL, objv[2], &volume) == TCL_OK) {
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
    NanoVis::VolumeHashmap::iterator itr;
    for (itr = NanoVis::volumeTable.begin();
         itr != NanoVis::volumeTable.end(); ++itr) {
        Tcl_Obj *objPtr = Tcl_NewStringObj(itr->second->name(), -1);
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

static CmdSpec volumeOutlineOps[] = {
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

    proc = GetOpFromObj(interp, nVolumeOutlineOps, volumeOutlineOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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
    const char *name = Tcl_GetString(objv[3]);
    TransferFunction *tf = NanoVis::getTransferFunction(name);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", name,
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    std::vector<Volume *> ivol;
    if (GetVolumes(interp, objc - 4, objv + 4, &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Volume *>::iterator iter;
    for (iter = ivol.begin(); iter != ivol.end(); ++iter) {
        TRACE("setting %s with transfer function %s", (*iter)->name(),
               tf->name());
        (*iter)->transferFunction(tf);
    }
    return TCL_OK;
}

static CmdSpec volumeShadingOps[] = {
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

    proc = GetOpFromObj(interp, nVolumeShadingOps, volumeShadingOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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

static CmdSpec volumeOps[] = {
    {"animation", 2, VolumeAnimationOp,   3, 0, "oper ?args?",},
    {"data",      2, VolumeDataOp,        3, 0, "oper ?args?",},
    {"delete",    2, VolumeDeleteOp,      3, 0, "?name...?",},
    {"exists",    1, VolumeExistsOp,      3, 3, "name",},
    {"names",     1, VolumeNamesOp,       2, 2, "",},
    {"outline",   1, VolumeOutlineOp,     3, 0, "oper ?args?",},
    {"shading",   2, VolumeShadingOp,     3, 0, "oper ?args?",},
    {"state",     2, VolumeStateOp,       3, 0, "bool ?indices?",},
};
static int nVolumeOps = NumCmdSpecs(volumeOps);

static int
VolumeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nVolumeOps, volumeOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    const char *tag = Tcl_GetString(objv[4]);

    Rappture::Buffer buf(nBytes);
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    Unirect2d data(1);
    if (data.parseBuffer(interp, buf.bytes(), buf.size()) != TCL_OK) {
        return TCL_ERROR;
    }
    if (data.nValues() == 0) {
        Tcl_AppendResult(interp, "no data found in stream", (char *)NULL);
        return TCL_ERROR;
    }
    if (!data.isInitialized()) {
        return TCL_ERROR;
    }
    HeightMap *heightMap;
    NanoVis::HeightMapHashmap::iterator itr = NanoVis::heightMapTable.find(tag);
    if (itr != NanoVis::heightMapTable.end()) {
        heightMap = itr->second;
    } else {
        heightMap = new HeightMap();
        NanoVis::heightMapTable[tag] = heightMap;
    }
    TRACE("Number of heightmaps=%d", NanoVis::heightMapTable.size());
    // Must set units before the heights.
    heightMap->xAxis.units(data.xUnits());
    heightMap->yAxis.units(data.yUnits());
    heightMap->zAxis.units(data.vUnits());
    heightMap->wAxis.units(data.yUnits());
    heightMap->setHeight(data.xMin(), data.yMin(), data.xMax(), data.yMax(), 
                         data.xNum(), data.yNum(), data.transferValues());
    heightMap->transferFunction(NanoVis::getTransferFunction("default"));
    heightMap->setVisible(true);
    heightMap->setLineContourVisible(true);
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

static CmdSpec heightMapDataOps[] = {
    {"follows",  1, HeightMapDataFollowsOp, 5, 5, "size heightmapName",},
    {"visible",  1, HeightMapDataVisibleOp, 4, 0, "bool ?heightmapNames...?",},
};
static int nHeightMapDataOps = NumCmdSpecs(heightMapDataOps);

static int
HeightMapDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nHeightMapDataOps, heightMapDataOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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

static CmdSpec heightMapLineContourOps[] = {
    {"color",   1, HeightMapLineContourColorOp,   6, 0, "r g b ?heightmapNames...?",},
    {"visible", 1, HeightMapLineContourVisibleOp, 4, 0, "bool ?heightmapNames...?",},
};
static int nHeightMapLineContourOps = NumCmdSpecs(heightMapLineContourOps);

static int 
HeightMapLineContourOp(ClientData clientData, Tcl_Interp *interp, int objc,
                       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nHeightMapLineContourOps,
                        heightMapLineContourOps, CMDSPEC_ARG2, objc, objv, 0);
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
    const char *tag = Tcl_GetString(objv[2]);
    NanoVis::HeightMapHashmap::iterator itr = NanoVis::heightMapTable.find(tag);
    if (itr != NanoVis::heightMapTable.end()) {
        Tcl_AppendResult(interp, "heightmap \"", tag, "\" already exists.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    /* heightmap create xmin ymin xmax ymax xnum ynum values */
    HeightMap *heightMap = CreateHeightMap(clientData, interp, objc - 3, objv + 3);
    if (heightMap == NULL) {
        return TCL_ERROR;
    }
    NanoVis::heightMapTable[tag] = heightMap;
    NanoVis::eventuallyRedraw();
    TRACE("Number of heightmaps=%d", NanoVis::heightMapTable.size());
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
    TransferFunction *tf = NanoVis::getTransferFunction(name);
    if (tf == NULL) {
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
        (*iter)->transferFunction(tf);
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

static CmdSpec heightMapOps[] = {
    {"create",       2, HeightMapCreateOp,      10, 10, "heightmapName xmin ymin xmax ymax xnum ynum values",},
    {"cull",         2, HeightMapCullOp,        3, 3, "mode",},
    {"data",         1, HeightMapDataOp,        3, 0, "oper ?args?",},
    {"legend",       2, HeightMapLegendOp,      5, 5, "heightmapName width height",},
    {"linecontour",  2, HeightMapLineContourOp, 2, 0, "oper ?args?",},
    {"opacity",      1, HeightMapOpacityOp,     3, 0, "value ?heightmapNames...? ",},
    {"polygon",      1, HeightMapPolygonOp,     3, 3, "mode",},
    {"shading",      1, HeightMapShadingOp,     3, 3, "model",},
    {"transfunc",    2, HeightMapTransFuncOp,   3, 0, "name ?heightmapNames...?",},
};
static int nHeightMapOps = NumCmdSpecs(heightMapOps);

static int
HeightMapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nHeightMapOps, heightMapOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

static CmdSpec gridOps[] = {
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

    proc = GetOpFromObj(interp, nGridOps, gridOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
        NanoVis::orientationIndicator->setVisible(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ImageFlushCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    lastCmdStatus = TCL_BREAK;
    return TCL_OK;
}

/**
 * \brief Execute commands from client in Tcl interpreter
 * 
 * In this threaded model, the select call is for event compression.  We
 * want to execute render server commands as long as they keep coming.  
 * This lets us execute a stream of many commands but render once.  This 
 * benefits camera movements, screen resizing, and opacity changes 
 * (using a slider on the client).  The down side is you don't render
 * until there's a lull in the command stream.  If the client needs an
 * image, it can issue an "imgflush" command.  That breaks us out of the
 * read loop.
 */
int
nv::processCommands(Tcl_Interp *interp,
                    ReadBuffer *inBufPtr, int fdOut)
{
    int ret = 1;
    int status = TCL_OK;

    Tcl_DString command;
    Tcl_DStringInit(&command);
    fd_set readFds;
    struct timeval tv, *tvPtr;

    FD_ZERO(&readFds);
    FD_SET(inBufPtr->file(), &readFds);
    tvPtr = NULL;                       /* Wait for the first read. This is so
                                         * that we don't spin when no data is
                                         * available. */
    while (inBufPtr->isLineAvailable() || 
           (select(1, &readFds, NULL, NULL, tvPtr) > 0)) {
        size_t numBytes;
        unsigned char *buffer;

        /* A short read is treated as an error here because we assume that we
         * will always get commands line by line. */
        if (inBufPtr->getLine(&numBytes, &buffer) != ReadBuffer::OK) {
            /* Terminate the server if we can't communicate with the client
             * anymore. */
            if (inBufPtr->status() == ReadBuffer::ENDFILE) {
                TRACE("Exiting server on EOF from client");
                return -1;
            } else {
                ERROR("Exiting server, failed to read from client: %s",
                      strerror(errno));
                return -1;
            }
        }
        Tcl_DStringAppend(&command, (char *)buffer, numBytes);
        if (Tcl_CommandComplete(Tcl_DStringValue(&command))) {
            struct timeval start, finish;
            gettimeofday(&start, NULL);
            status = ExecuteCommand(interp, &command);
            gettimeofday(&finish, NULL);
            g_stats.cmdTime += (MSECS_ELAPSED(start, finish) / 1.0e+3);
            g_stats.nCommands++;
            if (status == TCL_BREAK) {
                return 1;               /* This was caused by a "imgflush"
                                         * command. Break out of the read loop
                                         * and allow a new image to be
                                         * rendered. */
            } else { //if (status != TCL_OK) {
                ret = 0;
                if (handleError(interp, status, fdOut) < 0) {
                    return -1;
                }
            }
        }

        tv.tv_sec = tv.tv_usec = 0L;    /* On successive reads, we break out
                                         * if no data is available. */
        FD_SET(inBufPtr->file(), &readFds);
        tvPtr = &tv;
    }

    return ret;
}

/**
 * \brief Send error message to client socket
 */
int
nv::handleError(Tcl_Interp *interp, int status, int fdOut)
{
    const char *string;
    int nBytes;

    if (status != TCL_OK) {
        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        nBytes = strlen(string);
        if (nBytes > 0) {
            TRACE("status=%d errorInfo=(%s)", status, string);

            std::ostringstream oss;
            oss << "nv>viserror -type internal_error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
            nBytes = oss.str().length();

#ifdef USE_THREADS
            queueResponse(oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR);
#else
            if (write(fdOut, oss.str().c_str(), nBytes) < 0) {
                ERROR("write failed: %s", strerror(errno));
                return -1;
            }
#endif
        }
    }

    string = getUserMessages();
    nBytes = strlen(string);
    if (nBytes > 0) {
        TRACE("userError=(%s)", string);

        std::ostringstream oss;
        oss << "nv>viserror -type error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
        nBytes = oss.str().length();

#ifdef USE_THREADS
        queueResponse(oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR);
#else
        if (write(fdOut, oss.str().c_str(), nBytes) < 0) {
            ERROR("write failed: %s", strerror(errno));
            return -1;
        }
#endif
        clearUserMessages();
    }

    return 0;
}

void
nv::initTcl(Tcl_Interp *interp, ClientData clientData)
{
    /*
     * Ideally the connection is authenticated by nanoscale.  I still like the
     * idea of creating a non-safe master interpreter with a safe slave
     * interpreter.  Alias all the nanovis commands in the slave. That way we
     * can still run Tcl code within nanovis.  The eventual goal is to create
     * a test harness through the interpreter for nanovis.
     */

    Tcl_MakeSafe(interp);

    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "clientinfo",  ClientInfoCmd,  clientData, NULL);
    Tcl_CreateObjCommand(interp, "cutplane",    CutplaneCmd,    clientData, NULL);
    FlowCmdInitProc(interp, clientData);
    Tcl_CreateObjCommand(interp, "grid",        GridCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",   HeightMapCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgflush",    ImageFlushCmd,  clientData, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "snapshot",    SnapshotCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "transfunc",   TransfuncCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "up",          UpCmd,          clientData, NULL);
    Tcl_CreateObjCommand(interp, "volume",      VolumeCmd,      clientData, NULL);

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        WARN("bad default transfer function:\n%s", 
             Tcl_GetStringResult(interp));
    }
}
