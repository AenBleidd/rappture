/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <memory.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/uio.h> // for readv/writev
#include <time.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <GL/glew.h>
#include <GL/glut.h>

#include <RpEncode.h>

#include <graphics/RenderContext.h>

#include <util/FilePath.h>
#include <util/Fonts.h>

#include <BMPImageLoaderImpl.h>
#include <ImageLoaderFactory.h>

#include "config.h"
#include "nanovis.h"
#include "define.h"

#include "FlowCmd.h"
#include "Grid.h"
#include "HeightMap.h"
#include "NvCamera.h"
#include "NvShader.h"
#include "NvColorTableRenderer.h"
#include "NvFlowVisRenderer.h"
#include "NvLIC.h"
#include "NvZincBlendeReconstructor.h"
#include "PerfQuery.h"
#include "PlaneRenderer.h"
#ifdef USE_POINTSET_RENDERER
#include "PointSetRenderer.h"
#include "PointSet.h"
#endif
#include "Switch.h"
#include "Trace.h"
#include "Unirect.h"
#include "VelocityArrowsSlice.h"
#include "VolumeInterpolator.h"
#include "VolumeRenderer.h"
#include "ZincBlendeVolume.h"

using namespace nv::graphics;
using namespace nv::util;

#define SIZEOF_BMP_HEADER   54

/// Indicates "up" axis
enum AxisDirections { 
    X_POS = 1,
    Y_POS = 2,
    Z_POS = 3,
    X_NEG = -1,
    Y_NEG = -2,
    Z_NEG = -3
};

#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

#define TRUE	1
#define FALSE	0

typedef struct {
    pid_t pid;
    size_t nFrames;            /**< # of frames sent to client. */
    size_t nBytes;             /**< # of bytes for all frames. */
    size_t nCommands;          /**< # of commands executed */
    double cmdTime;            /**< Elasped time spend executing
                                * commands. */
    struct timeval start;      /**< Start of elapsed time. */
} Stats;

static Stats stats;

// STATIC MEMBER DATA
struct timeval NanoVis::startTime;      /* Start of elapsed time. */
Grid *NanoVis::grid = NULL;
int NanoVis::updir = Y_POS;
NvCamera *NanoVis::cam = NULL;
Tcl_HashTable NanoVis::volumeTable;
Tcl_HashTable NanoVis::heightmapTable;
VolumeRenderer *NanoVis::volRenderer = NULL;
#ifdef USE_POINTSET_RENDERER
PointSetRenderer *NanoVis::pointSetRenderer = NULL;
std::vector<PointSet *> NanoVis::pointSet;
#endif

PlaneRenderer *NanoVis::planeRenderer = NULL;
#ifdef PLANE_CMD
// pointers to 2D planes, currently handle up 10
int NanoVis::numPlanes = 10;
Texture2D *NanoVis::plane[10];
#endif
Texture2D *NanoVis::legendTexture = NULL;
NvColorTableRenderer *NanoVis::colorTableRenderer = NULL;
#ifdef notdef
NvFlowVisRenderer *NanoVis::flowVisRenderer = NULL;
#endif
VelocityArrowsSlice *NanoVis::velocityArrowsSlice = NULL;

RenderContext *NanoVis::renderContext = NULL;
NvLIC *NanoVis::licRenderer = NULL;
Fonts *NanoVis::fonts;

FILE *NanoVis::stdin = NULL;
FILE *NanoVis::logfile = NULL;
FILE *NanoVis::recfile = NULL;

int NanoVis::statsFile = -1;

bool NanoVis::axisOn = true;
bool NanoVis::debugFlag = false;

Tcl_Interp *NanoVis::interp;

//frame buffer for final rendering
GLuint NanoVis::_finalColorTex = 0;
GLuint NanoVis::_finalDepthRb = 0;
GLuint NanoVis::_finalFbo = 0;
int NanoVis::renderWindow = 0;       /* GLUT handle for the render window */
int NanoVis::winWidth = NPIX;        /* Width of the render window */
int NanoVis::winHeight = NPIX;       /* Height of the render window */

unsigned char* NanoVis::screenBuffer = NULL;

unsigned int NanoVis::flags = 0;
Tcl_HashTable NanoVis::flowTable;
double NanoVis::magMin = DBL_MAX;
double NanoVis::magMax = -DBL_MAX;
float NanoVis::xMin = FLT_MAX;
float NanoVis::xMax = -FLT_MAX;
float NanoVis::yMin = FLT_MAX;
float NanoVis::yMax = -FLT_MAX;
float NanoVis::zMin = FLT_MAX;
float NanoVis::zMax = -FLT_MAX;
float NanoVis::wMin = FLT_MAX;
float NanoVis::wMax = -FLT_MAX;

/* FIXME: This variable is always true. */
static bool volumeMode = true; 

// in Command.cpp
extern Tcl_Interp *initTcl();

// maps transfunc name to TransferFunction object
Tcl_HashTable NanoVis::tfTable;

PerfQuery *perf = NULL;                        //performance counter

// Default camera location.
const float def_eye_x = 0.0f;
const float def_eye_y = 0.0f;
const float def_eye_z = 2.5f;

// Image based flow visualization slice location
// FLOW
float NanoVis::_licSlice = 0.5f;
int NanoVis::_licAxis = 2; // z axis

void
NanoVis::removeAllData()
{
    TRACE("Enter");
    if (grid != NULL) {
        TRACE("Deleting grid");
        delete grid;
    }
    if (cam != NULL) {
        TRACE("Deleting cam");
        delete cam;
    }
    if (volRenderer != NULL) {
        TRACE("Deleting volRenderer");
        delete volRenderer;
    }
    if (planeRenderer != NULL) {
        TRACE("Deleting planeRenderer");
        delete planeRenderer;
    }
    if (colorTableRenderer != NULL) {
        TRACE("Deleting colorTableRenderer");
        delete colorTableRenderer;
    }
#ifdef PLANE_CMD
    for (int i = 0; i < numPlanes; i++) {
        TRACE("Deleting plane[%d]", i);
        delete plane[i];
    }
#endif
    if (legendTexture != NULL) {
        TRACE("Deleting legendTexture");
        delete legendTexture;
    }
#ifdef notdef
    if (flowVisRenderer != NULL) {
        TRACE("Deleting flowVisRenderer");
        delete flowVisRenderer;
    }
#endif
    TRACE("Deleting flows");
    DeleteFlows(interp);
    if (licRenderer != NULL) {
        TRACE("Deleting licRenderer");
        delete licRenderer;
    }
    if (velocityArrowsSlice != NULL) {
        TRACE("Deleting velocityArrowsSlice");
        delete velocityArrowsSlice;
    }
    if (renderContext != NULL) {
        TRACE("Deleting renderContext");
        delete renderContext;
    }
    if (screenBuffer != NULL) {
        TRACE("Deleting screenBuffer");
        delete [] screenBuffer;
    }
    if (perf != NULL) {
        TRACE("Deleting perf");
        delete perf;
    }
#ifdef USE_POINTSET_RENDERER
    if (pointSetRenderer != NULL) {
        TRACE("Deleting pointSetRenderer");
        delete pointSetRenderer;
    }
    for (std::vector<PointSet *>::iterator itr = pointSet.begin();
         itr != pointSet.end(); ++itr) {
        TRACE("Deleting pointSet: %p", *itr);
        delete (*itr);
    }
#endif
    TRACE("Leave");
}

void 
NanoVis::eventuallyRedraw(unsigned int flag)
{
    if (flag) {
        flags |= flag;
    }
    if ((flags & REDRAW_PENDING) == 0) {
        glutPostRedisplay();
        flags |= REDRAW_PENDING;
    }
}

#ifdef KEEPSTATS

#ifndef STATSDIR
#define STATSDIR	"/var/tmp/visservers"
#endif  /*STATSDIR*/

int
NanoVis::getStatsFile(Tcl_Obj *objPtr)
{
    Tcl_DString ds;
    Tcl_Obj **objv;
    int objc;
    int i;
    char fileName[33];
    const char *path;
    md5_state_t state;
    md5_byte_t digest[16];
    char *string;
    int length;

    if ((objPtr == NULL) || (statsFile >= 0)) {
        return statsFile;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return -1;
    }
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj("pid", 3));
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewIntObj(getpid()));
    string = Tcl_GetStringFromObj(objPtr, &length);

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)string, length);
    md5_finish(&state, digest);
    for (i = 0; i < 16; i++) {
        sprintf(fileName + i * 2, "%02x", digest[i]);
    }
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, STATSDIR, -1);
    Tcl_DStringAppend(&ds, "/", 1);
    Tcl_DStringAppend(&ds, fileName, 32);
    path = Tcl_DStringValue(&ds);

    statsFile = open(path, O_EXCL | O_CREAT | O_WRONLY, 0600);
    Tcl_DStringFree(&ds);
    if (statsFile < 0) {
	ERROR("can't open \"%s\": %s", fileName, strerror(errno));
	return -1;
    }
    return statsFile;
}

int
NanoVis::writeToStatsFile(int f, const char *s, size_t length)
{
    if (f >= 0) {
        ssize_t numWritten;

        numWritten = write(f, s, length);
        if (numWritten == (ssize_t)length) {
            close(dup(f));
        }
    }
    return 0;
}

static int
serverStats(int code) 
{
    double start, finish;
    char buf[BUFSIZ];
    Tcl_DString ds;
    int result;
    int f;

    {
	struct timeval tv;

	/* Get ending time.  */
	gettimeofday(&tv, NULL);
	finish = CVT2SECS(tv);
	tv = stats.start;
	start = CVT2SECS(tv);
    }
    /* 
     * Session information:
     *   - Name of render server
     *   - Process ID
     *   - Hostname where server is running
     *   - Start date of session
     *   - Start date of session in seconds
     *   - Number of frames returned
     *   - Number of bytes total returned (in frames)
     *   - Number of commands received
     *   - Total elapsed time of all commands
     *   - Total elapsed time of session
     *   - Exit code of vizserver
     *   - User time 
     *   - System time
     *   - User time of children 
     *   - System time of children
     */ 

    Tcl_DStringInit(&ds);
    
    Tcl_DStringAppendElement(&ds, "render_stop");
    /* renderer */
    Tcl_DStringAppendElement(&ds, "renderer");
    Tcl_DStringAppendElement(&ds, "nanovis");
    /* pid */
    Tcl_DStringAppendElement(&ds, "pid");
    sprintf(buf, "%d", getpid());
    Tcl_DStringAppendElement(&ds, buf);
    /* host */
    Tcl_DStringAppendElement(&ds, "host");
    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date */
    Tcl_DStringAppendElement(&ds, "date");
    strcpy(buf, ctime(&stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date_secs */
    Tcl_DStringAppendElement(&ds, "date_secs");
    sprintf(buf, "%ld", stats.start.tv_sec);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_frames */
    Tcl_DStringAppendElement(&ds, "num_frames");
    sprintf(buf, "%lu", (unsigned long int)stats.nFrames);
    Tcl_DStringAppendElement(&ds, buf);
    /* frame_bytes */
    Tcl_DStringAppendElement(&ds, "frame_bytes");
    sprintf(buf, "%lu", (unsigned long int)stats.nBytes);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_commands */
    Tcl_DStringAppendElement(&ds, "num_commands");
    sprintf(buf, "%lu", (unsigned long int)stats.nCommands);
    Tcl_DStringAppendElement(&ds, buf);
    /* cmd_time */
    Tcl_DStringAppendElement(&ds, "cmd_time");
    sprintf(buf, "%g", stats.cmdTime);
    Tcl_DStringAppendElement(&ds, buf);
    /* session_time */
    Tcl_DStringAppendElement(&ds, "session_time");
    sprintf(buf, "%g", finish - start);
    Tcl_DStringAppendElement(&ds, buf);
    /* status */
    Tcl_DStringAppendElement(&ds, "status");
    sprintf(buf, "%d", code);
    Tcl_DStringAppendElement(&ds, buf);
    {
	long clocksPerSec = sysconf(_SC_CLK_TCK);
	double clockRes = 1.0 / clocksPerSec;
	struct tms tms;

	memset(&tms, 0, sizeof(tms));
	times(&tms);
	/* utime */
	Tcl_DStringAppendElement(&ds, "utime");
	sprintf(buf, "%g", tms.tms_utime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* stime */
	Tcl_DStringAppendElement(&ds, "stime");
	sprintf(buf, "%g", tms.tms_stime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* cutime */
	Tcl_DStringAppendElement(&ds, "cutime");
	sprintf(buf, "%g", tms.tms_cutime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
	/* cstime */
	Tcl_DStringAppendElement(&ds, "cstime");
	sprintf(buf, "%g", tms.tms_cstime * clockRes);
	Tcl_DStringAppendElement(&ds, buf);
    }
    Tcl_DStringAppend(&ds, "\n", -1);
    f = NanoVis::getStatsFile(NULL);
    result = NanoVis::writeToStatsFile(f, Tcl_DStringValue(&ds), 
                                       Tcl_DStringLength(&ds));
    close(f);
    Tcl_DStringFree(&ds);
    return result;
}

#endif

static void
initService()
{
    TRACE("Enter");

    const char* user = getenv("USER");
    char* logName = NULL;
    int logNameLen = 0;

    if (user == NULL) {
        logNameLen = 20+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/nanovis_log.txt", logNameLen);
    } else {
        logNameLen = 17+1+strlen(user);
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/nanovis_log_", logNameLen);
        strncat(logName, user, strlen(user));
    }

    //open log and map stderr to log file
    NanoVis::logfile = fopen(logName, "w");
    dup2(fileno(NanoVis::logfile), 2);
    /* dup2(2,1); */

    // clean up malloc'd memory
    if (logName != NULL) {
        free(logName);
    }

    TRACE("Leave");
}

static void
exitService(int code)
{
    TRACE("Enter: %d", code);

    NanoVis::removeAllData();

    NvShader::exitCg();

    //close log file
    if (NanoVis::logfile != NULL) {
        fclose(NanoVis::logfile);
	NanoVis::logfile = NULL;
    }

#ifdef KEEPSTATS
    serverStats(code);
#endif
    closelog();

    exit(code);
}

static int
executeCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    struct timeval tv;
    double start, finish;
    int result;

#ifdef WANT_TRACE
    char *str = Tcl_DStringValue(dsPtr);
    std::string cmd(str);
    cmd.erase(cmd.find_last_not_of(" \n\r\t")+1);
    TRACE("command %lu: '%s'", stats.nCommands+1, cmd.c_str());
#endif

    gettimeofday(&tv, NULL);
    start = CVT2SECS(tv);

    if (NanoVis::recfile != NULL) {
        fprintf(NanoVis::recfile, "%s", Tcl_DStringValue(dsPtr));
        fflush(NanoVis::recfile);
    }
    result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
    Tcl_DStringSetLength(dsPtr, 0);

    gettimeofday(&tv, NULL);
    finish = CVT2SECS(tv);

    stats.cmdTime += finish - start;
    stats.nCommands++;
    TRACE("Leave status=%d", result);
    return result;
}

void
NanoVis::pan(float dx, float dy)
{
    /* Move the camera and its target by equal amounts along the x and y
     * axes. */
    TRACE("pan: x=%f, y=%f", dx, dy);

    cam->x(def_eye_x - dx);
    cam->y(def_eye_y + dy);
    TRACE("set eye to %f %f", cam->x(), cam->y());
}

void
NanoVis::zoom(float z)
{
    /* Move the camera and its target by equal amounts along the x and y
     * axes. */
    TRACE("zoom: z=%f", z);

    cam->z(def_eye_z / z);
    TRACE("set cam z to %f", cam->z());
}

/** \brief Load a 3D volume
 *
 * \param n_component the number of scalars for each space point. All component 
 * scalars for a point are placed consequtively in data array
 * width, height and depth: number of points in each dimension
 * \param data pointer to an array of floats.
 */
Volume *
NanoVis::loadVolume(const char *name, int width, int height, int depth, 
                    int n_component, float *data, double vmin, double vmax, 
                    double nzero_min)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&volumeTable, name);
    if (hPtr != NULL) {
        Volume *volPtr; 
        WARN("volume \"%s\" already exists", name);
        volPtr = (Volume *)Tcl_GetHashValue(hPtr);
        removeVolume(volPtr);
    }
    int isNew;
    hPtr = Tcl_CreateHashEntry(&volumeTable, name, &isNew);
    Volume* volPtr;
    volPtr = new Volume(0.f, 0.f, 0.f, width, height, depth, n_component,
                        data, vmin, vmax, nzero_min);
    Volume::updatePending = true;
    Tcl_SetHashValue(hPtr, volPtr);
    volPtr->name(Tcl_GetHashKey(&volumeTable, hPtr));
    return volPtr;
}

// Gets a colormap 1D texture by name.
TransferFunction *
NanoVis::getTransfunc(const char *name) 
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&tfTable, name);
    if (hPtr == NULL) {
        ERROR("No transfer function named \"%s\" found", name);
        return NULL;
    }
    return (TransferFunction *)Tcl_GetHashValue(hPtr);
}

// Creates of updates a colormap 1D texture by name.
TransferFunction *
NanoVis::defineTransferFunction(const char *name, size_t n, float *data)
{
    int isNew;
    Tcl_HashEntry *hPtr;
    TransferFunction *tfPtr;

    hPtr = Tcl_CreateHashEntry(&tfTable, name, &isNew);
    if (isNew) {
        TRACE("Creating new transfer function \"%s\"", name);

        tfPtr = new TransferFunction(n, data);
        tfPtr->name(Tcl_GetHashKey(&tfTable, hPtr));
        Tcl_SetHashValue(hPtr, tfPtr);
    } else {
        TRACE("Updating existing transfer function \"%s\"", name);

        /* 
         * You can't delete the transfer function because many 
         * objects may be holding its pointer.  We must update it.
         */
        tfPtr = (TransferFunction *)Tcl_GetHashValue(hPtr);
        tfPtr->update(n, data);
    }
    return tfPtr;
}

int
NanoVis::renderLegend(TransferFunction *tf, double min, double max,
                      int width, int height, const char *volArg)
{
    TRACE("Enter");

    int old_width = winWidth;
    int old_height = winHeight;

    planeRenderer->setScreenSize(width, height);
    resizeOffscreenBuffer(width, height);

    // generate data for the legend
    float data[512];
    for (int i=0; i < 256; i++) {
        data[i] = data[i+256] = (float)(i/255.0);
    }
    legendTexture = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
    int index = planeRenderer->addPlane(legendTexture, tf);
    planeRenderer->setActivePlane(index);

    bindOffscreenBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen
    planeRenderer->render();

    // INSOO
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screenBuffer);
    //glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, screenBuffer); // INSOO's

    {
        char prefix[200];
        ssize_t nWritten;

        TRACE("Sending ppm legend image %s min:%g max:%g", volArg, min, max);
        sprintf(prefix, "nv>legend %s %g %g", volArg, min, max);
        ppmWrite(prefix);
        nWritten = write(1, "\n", 1);
        assert(nWritten == 1);
    }
    planeRenderer->removePlane(index);
    resizeOffscreenBuffer(old_width, old_height);

    delete legendTexture;
    legendTexture = NULL;
    TRACE("Leave");
    return TCL_OK;
}

//initialize frame buffer objects for offscreen rendering
void
NanoVis::initOffscreenBuffer()
{
    TRACE("Enter");
    assert(_finalFbo == 0);
    // Initialize a fbo for final display.
    glGenFramebuffersEXT(1, &_finalFbo);

    glGenTextures(1, &_finalColorTex);
    glBindTexture(GL_TEXTURE_2D, _finalColorTex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

#if defined(HAVE_FLOAT_TEXTURES) && defined(USE_HALF_FLOAT)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#elif defined(HAVE_FLOAT_TEXTURES)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#endif

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _finalFbo);
    glGenRenderbuffersEXT(1, &_finalDepthRb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _finalDepthRb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 
                             winWidth, winHeight);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, _finalColorTex, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, _finalDepthRb);

    GLenum status;
    if (!CheckFBO(&status)) {
        PrintFBOStatus(status, "finalFbo");
        exitService(3);
    }

    TRACE("Leave");
}

//resize the offscreen buffer 
void 
NanoVis::resizeOffscreenBuffer(int w, int h)
{
    TRACE("Enter (%d, %d)", w, h);
    if ((w == winWidth) && (h == winHeight)) {
        return;
    }
    winWidth = w;
    winHeight = h;

    if (fonts) {
        fonts->resize(w, h);
    }
    TRACE("screenBuffer size: %d %d", w, h);

    if (screenBuffer != NULL) {
        delete [] screenBuffer;
        screenBuffer = NULL;
    }

    screenBuffer = new unsigned char[4*winWidth*winHeight];
    assert(screenBuffer != NULL);
    
    //delete the current render buffer resources
    glDeleteTextures(1, &_finalColorTex);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _finalDepthRb);
    glDeleteRenderbuffersEXT(1, &_finalDepthRb);

    TRACE("before deleteframebuffers");
    glDeleteFramebuffersEXT(1, &_finalFbo);

    TRACE("reinitialize FBO");
    //Reinitialize final fbo for final display
    glGenFramebuffersEXT(1, &_finalFbo);

    glGenTextures(1, &_finalColorTex);
    glBindTexture(GL_TEXTURE_2D, _finalColorTex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

#if defined(HAVE_FLOAT_TEXTURES) && defined(USE_HALF_FLOAT)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#elif defined(HAVE_FLOAT_TEXTURES)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winWidth, winHeight, 0,
                 GL_RGB, GL_INT, NULL);
#endif

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _finalFbo);
    glGenRenderbuffersEXT(1, &_finalDepthRb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _finalDepthRb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 
                             winWidth, winHeight);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, _finalColorTex, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, _finalDepthRb);

    GLenum status;
    if (!CheckFBO(&status)) {
        PrintFBOStatus(status, "finalFbo");
        exitService(3);
    }

    TRACE("change camera");
    //change the camera setting
    cam->setScreenSize(0, 0, winWidth, winHeight);
    planeRenderer->setScreenSize(winWidth, winHeight);

    TRACE("Leave (%d, %d)", w, h);
}

static
void cgErrorCallback(void)
{
    if (!NvShader::printErrorInfo()) {
        TRACE("Cg error, exiting...");
        exitService(-1);
    }
}

void NanoVis::init(const char* path)
{
    // print OpenGL driver information
    TRACE("-----------------------------------------------------------");
    TRACE("OpenGL version: %s", glGetString(GL_VERSION));
    TRACE("OpenGL vendor: %s", glGetString(GL_VENDOR));
    TRACE("OpenGL renderer: %s", glGetString(GL_RENDERER));
    TRACE("-----------------------------------------------------------");

    if (path == NULL) {
        ERROR("No path defined for shaders or resources");
        exitService(1);
    }
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        ERROR("Can't init GLEW: %s", glewGetErrorString(err));
        exitService(1);
    }
    TRACE("Using GLEW %s", glewGetString(GLEW_VERSION));

    // OpenGL 2.1 includes VBOs, PBOs, MRT, NPOT textures, point parameters, point sprites,
    // GLSL 1.2, and occlusion queries.
    if (!GLEW_VERSION_2_1) {
        ERROR("OpenGL version 2.1 or greater is required");
        exitService(1);
    }

    // NVIDIA driver may report OpenGL 2.1, but not support PBOs in 
    // indirect GLX contexts
    if (!GLEW_ARB_pixel_buffer_object) {
        ERROR("Pixel buffer objects are not supported by driver, please check that the user running nanovis has permissions to create a direct rendering context (e.g. user has read/write access to /dev/nvidia* device nodes in Linux).");
        exitService(1);
    }

    // Additional extensions not in 2.1 core

    // Framebuffer objects were promoted in 3.0
    if (!GLEW_EXT_framebuffer_object) {
        ERROR("EXT_framebuffer_oject extension is required");
        exitService(1);
    }
    // Rectangle textures were promoted in 3.1
    // FIXME: can use NPOT in place of rectangle textures
    if (!GLEW_ARB_texture_rectangle) {
        ERROR("ARB_texture_rectangle extension is required");
        exitService(1);
    }
#ifdef HAVE_FLOAT_TEXTURES
    // Float color buffers and textures were promoted in 3.0
    if (!GLEW_ARB_texture_float ||
        !GLEW_ARB_color_buffer_float) {
        ERROR("ARB_texture_float and ARB_color_buffer_float extensions are required");
        exitService(1);
    }
#endif
    // FIXME: should use ARB programs or (preferably) a GLSL profile for portability
    if (!GLEW_NV_vertex_program3 ||
        !GLEW_NV_fragment_program2) {
        ERROR("NV_vertex_program3 and NV_fragment_program2 extensions are required");
        exitService(1);
    }

    if (!FilePath::getInstance()->setPath(path)) {
        ERROR("can't set file path to %s", path);
        exitService(1);
    }

    ImageLoaderFactory::getInstance()->addLoaderImpl("bmp", new BMPImageLoaderImpl());

    NvShader::initCg();
    NvShader::setErrorCallback(cgErrorCallback);

    fonts = new Fonts();
    fonts->addFont("verdana", "verdana.fnt");
    fonts->setFont("verdana");

    colorTableRenderer = new NvColorTableRenderer();
    colorTableRenderer->setFonts(fonts);
#ifdef notdef
    flowVisRenderer = new NvFlowVisRenderer(NMESH, NMESH);
#endif
    velocityArrowsSlice = new VelocityArrowsSlice;

    licRenderer = new NvLIC(NMESH, NPIX, NPIX, _licAxis, _licSlice);

    grid = new Grid();
    grid->setFont(fonts);

#ifdef USE_POINTSET_RENDERER
    pointSetRenderer = new PointSetRenderer();
#endif
}

void
NanoVis::initGL()
{
    TRACE("in initGL");
    //buffer to store data read from the screen
    if (screenBuffer) {
        delete[] screenBuffer;
        screenBuffer = NULL;
    }
    screenBuffer = new unsigned char[4*winWidth*winHeight];
    assert(screenBuffer != NULL);

    //create the camera with default setting
    cam = new NvCamera(0, 0, winWidth, winHeight,
                       def_eye_x, def_eye_y, def_eye_z);

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //initialize lighting
    GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat mat_shininess[] = {30.0};
    GLfloat white_light[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat green_light[] = {0.1, 0.5, 0.1, 1.0};

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, green_light);
    glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);

    // init table of transfer functions
    Tcl_InitHashTable(&tfTable, TCL_STRING_KEYS);

    //check if performance query is supported
    if (PerfQuery::checkQuerySupport()) {
        //create queries to count number of rendered pixels
        perf = new PerfQuery();
    }

    initOffscreenBuffer();    //frame buffer object for offscreen rendering

    //create volume renderer and add volumes to it
    volRenderer = new VolumeRenderer();

    // create
    renderContext = new RenderContext();

    //create a 2D plane renderer
    planeRenderer = new PlaneRenderer(winWidth, winHeight);

    //assert(glGetError()==0);

    TRACE("leaving initGL");
}

#ifdef DO_RLE
char rle[512*512*3];
int rleSize;

short offsets[512*512*3];
int offsetsSize;

static void 
doRle()
{
    int len = NanoVis::winWidth*NanoVis::winHeight*3;
    rleSize = 0;
    offsetsSize = 0;

    int i = 0;
    while (i < len) {
        if (NanoVis::screenBuffer[i] == 0) {
            int pos = i+1;
            while ( (pos < len) && (NanoVis::screenBuffer[pos] == 0)) {
                pos++;
            }
            offsets[offsetsSize++] = -(pos - i);
            i = pos;
        }

        else {
            int pos;
            for (pos = i; (pos < len) && (NanoVis::screenBuffer[pos] != 0); pos++){
                rle[rleSize++] = NanoVis::screenBuffer[pos];
            }
            offsets[offsetsSize++] = (pos - i);
            i = pos;
        }

    }
}
#endif

// used internally to build up the BMP file header
// Writes an integer value into the header data structure at pos
static inline void
bmpHeaderAddInt(unsigned char* header, int& pos, int data)
{
#ifdef WORDS_BIGENDIAN
    header[pos++] = (data >> 24) & 0xFF;
    header[pos++] = (data >> 16) & 0xFF;
    header[pos++] = (data >> 8)  & 0xFF;
    header[pos++] = (data)       & 0xFF;
#else
    header[pos++] = data & 0xff;
    header[pos++] = (data >> 8) & 0xff;
    header[pos++] = (data >> 16) & 0xff;
    header[pos++] = (data >> 24) & 0xff;
#endif
}

// INSOO
// FOR DEBUGGING
void
NanoVis::bmpWriteToFile(int frame_number, const char *directory_name)
{
    unsigned char header[SIZEOF_BMP_HEADER];
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3*winWidth) % 4 > 0) {
        pad = 4 - ((3*winWidth) % 4);
    }

    // file size in bytes
    int fsize = (3*winWidth+pad)*winHeight + SIZEOF_BMP_HEADER;
    bmpHeaderAddInt(header, pos, fsize);

    // reserved value (must be 0)
    bmpHeaderAddInt(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmpHeaderAddInt(header, pos, SIZEOF_BMP_HEADER);

    // size of the BITMAPINFOHEADER
    bmpHeaderAddInt(header, pos, 40);

    // width of the image in pixels
    bmpHeaderAddInt(header, pos, winWidth);

    // height of the image in pixels
    bmpHeaderAddInt(header, pos, winHeight);

    // 1 plane + (24 bits/pixel << 16)
    bmpHeaderAddInt(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char* scr = screenBuffer;
    for (int row=0; row < winHeight; row++) {
        for (int col=0; col < winWidth; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    FILE* f;
    char filename[100];
    if (frame_number >= 0) {
        if (directory_name)
            sprintf(filename, "%s/image%03d.bmp", directory_name, frame_number);
        else
            sprintf(filename, "/tmp/flow_animation/image%03d.bmp", frame_number);

        TRACE("Writing %s", filename);
        f = fopen(filename, "wb");
        if (f == 0) {
            ERROR("cannot create file");
        }
    } else {
        f = fopen("/tmp/image.bmp", "wb");
        if (f == 0) {
            ERROR("cannot create file");
        }
    }
    if (fwrite(header, SIZEOF_BMP_HEADER, 1, f) != 1) {
        ERROR("can't write header: short write");
    }
    if (fwrite(screenBuffer, (3*winWidth+pad)*winHeight, 1, f) != 1) {
        ERROR("can't write data: short write");
    }
    fclose(f);
}

void
NanoVis::bmpWrite(const char *prefix)
{
    unsigned char header[SIZEOF_BMP_HEADER];
    ssize_t nWritten;
    int pos = 0;

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3*winWidth) % 4 > 0) {
        pad = 4 - ((3*winWidth) % 4);
    }
    pad = 0;
    int fsize = (3*winWidth+pad)*winHeight + sizeof(header);

    char string[200];
    sprintf(string, "%s %d\n", prefix, fsize);
    nWritten = write(1, string, strlen(string));
    assert(nWritten == (ssize_t)strlen(string));
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    bmpHeaderAddInt(header, pos, fsize);

    // reserved value (must be 0)
    bmpHeaderAddInt(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmpHeaderAddInt(header, pos, SIZEOF_BMP_HEADER);

    // size of the BITMAPINFOHEADER
    bmpHeaderAddInt(header, pos, 40);

    // width of the image in pixels
    bmpHeaderAddInt(header, pos, winWidth);

    // height of the image in pixels
    bmpHeaderAddInt(header, pos, winHeight);

    // 1 plane + (24 bits/pixel << 16)
    bmpHeaderAddInt(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char* scr = screenBuffer;
    for (int row=0; row < winHeight; row++) {
        for (int col=0; col < winWidth; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    nWritten = write(1, header, SIZEOF_BMP_HEADER);
    assert(nWritten == SIZEOF_BMP_HEADER);
    nWritten = write(1, screenBuffer, (3*winWidth+pad)*winHeight);
    assert(nWritten == (3*winWidth+pad)*winHeight);
    stats.nFrames++;
    stats.nBytes += (3*winWidth+pad)*winHeight;
}

/*
 * ppmWrite --
 *
 *  Writes the screen image as PPM binary data to the nanovisviewer
 *  client.  The PPM binary format is very simple.
 *
 *      P6 w h 255\n
 *      3-byte RGB pixel data.
 *
 *  The nanovisviewer client (using the TkImg library) will do less work
 *  to unpack this format, as opposed to BMP or PNG.  (This doesn't
 *  eliminate the need to look into DXT compression performed on the GPU).
 *
 *      Note that currently the image data from the screen is both row-padded
 *      and the scan lines are reversed.  This routine could be made even
 *      simpler (faster) if the screen buffer is an array of packed 3-bytes
 *      per pixels (no padding) and where the origin is the top-left corner.
 */
void
NanoVis::ppmWrite(const char *prefix)
{
#define PPM_MAXVAL 255
    char header[200];

    TRACE("Enter (%dx%d)", winWidth, winHeight);
    // Generate the PPM binary file header
    sprintf(header, "P6 %d %d %d\n", winWidth, winHeight, PPM_MAXVAL);

    size_t header_length = strlen(header);
    size_t data_length = winWidth * winHeight * 3;

    char command[200];
    sprintf(command, "%s %lu\n", prefix, 
            (unsigned long)header_length + data_length);

    size_t wordsPerRow = (winWidth * 24 + 31) / 32;
    size_t bytesPerRow = wordsPerRow * 4;
    size_t rowLength = winWidth * 3;
    size_t nRecs = winHeight + 2;

    struct iovec *iov;
    iov = (struct iovec *)malloc(sizeof(struct iovec) * nRecs);

    // Write the nanovisviewer command, then the image header and data.
    // Command
    iov[0].iov_base = command;
    iov[0].iov_len = strlen(command);
    // Header of image data
    iov[1].iov_base = header;
    iov[1].iov_len = header_length;
    // Image data.
    int y;
    unsigned char *srcRowPtr = screenBuffer;
    for (y = winHeight + 1; y >= 2; y--) {
        iov[y].iov_base = srcRowPtr;
        iov[y].iov_len = rowLength;
        srcRowPtr += bytesPerRow;
    }
    if (writev(1, iov, nRecs) < 0) {
        ERROR("write failed: %s", strerror(errno));
    }
    free(iov);
    stats.nFrames++;
    stats.nBytes += (bytesPerRow * winHeight);
    TRACE("Leave (%dx%d)", winWidth, winHeight);
}

void
NanoVis::sendDataToClient(const char *command, const char *data, size_t dlen)
{
    /*
      char header[200];

      // Generate the PPM binary file header
      sprintf(header, "P6 %d %d %d\n", winWidth, winHeight, PPM_MAXVAL);

      size_t header_length = strlen(header);
      size_t data_length = winWidth * winHeight * 3;

      char command[200];
      sprintf(command, "%s %lu\n", prefix, 
      (unsigned long)header_length + data_length);
    */

    //    size_t wordsPerRow = (winWidth * 24 + 31) / 32;
    //    size_t bytesPerRow = wordsPerRow * 4;
    //    size_t rowLength = winWidth * 3;
    size_t nRecs = 2;

    struct iovec *iov = new iovec[nRecs];

    // Write the nanovisviewer command, then the image header and data.
    // Command
    // FIXME: shouldn't have to cast this
    iov[0].iov_base = (char *)command;
    iov[0].iov_len = strlen(command);
    // Data
    // FIXME: shouldn't have to cast this
    iov[1].iov_base = (char *)data;
    iov[1].iov_len = dlen;
    if (writev(1, iov, nRecs) < 0) {
        ERROR("write failed: %s", strerror(errno));
    }
    delete [] iov;
    // stats.nFrames++;
    // stats.nBytes += (bytesPerRow * winHeight);
}

void
NanoVis::idle()
{
    TRACE("Enter");

    glutSetWindow(renderWindow);

    processCommands();

    TRACE("Leave");
}

void 
NanoVis::draw3dAxis()
{
    glPushAttrib(GL_ENABLE_BIT);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    //draw axes
    GLUquadric *obj;

    obj = gluNewQuadric();

    int segments = 50;

    glColor3f(0.8, 0.8, 0.8);
    glPushMatrix();
    glTranslatef(0.4, 0., 0.);
    glRotatef(90, 1, 0, 0);
    glRotatef(180, 0, 1, 0);
    glScalef(0.0005, 0.0005, 0.0005);
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'x');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0., 0.4, 0.);
    glRotatef(90, 1, 0, 0);
    glRotatef(180, 0, 1, 0);
    glScalef(0.0005, 0.0005, 0.0005);
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'y');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0., 0., 0.4);
    glRotatef(90, 1, 0, 0);
    glRotatef(180, 0, 1, 0);
    glScalef(0.0005, 0.0005, 0.0005);
    glutStrokeCharacter(GLUT_STROKE_ROMAN, 'z');
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    //glColor3f(0.2, 0.2, 0.8);
    glPushMatrix();
    glutSolidSphere(0.02, segments, segments );
    glPopMatrix();

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0., 0.3, 0.);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90, 0, 1, 0);
    gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.3, 0., 0.);
    glRotatef(90, 0, 1, 0);
    gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
    glPopMatrix();

    glPushMatrix();
    gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0., 0., 0.3);
    gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
    glPopMatrix();

    gluDeleteQuadric(obj);

    glPopAttrib();
}

void NanoVis::update()
{
    VolumeInterpolator *volInterp = volRenderer->getVolumeInterpolator();
    if (volInterp->isStarted()) {
        struct timeval clock;
        gettimeofday(&clock, NULL);
        double elapsed_time;

        elapsed_time = clock.tv_sec + clock.tv_usec/1000000.0 -
            volInterp->getStartTime();

        TRACE("%lf %lf", elapsed_time, 
              volInterp->getInterval());
        float fraction;
        float f;

        f = fmod((float) elapsed_time, (float)volInterp->getInterval());
        if (f == 0.0) {
            fraction = 0.0f;
        } else {
            fraction = f / volInterp->getInterval();
        }
        TRACE("fraction : %f", fraction);
        volInterp->update(fraction);
    }
}

void
NanoVis::setVolumeRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    TRACE("Enter");
    xMin = yMin = zMin = wMin = DBL_MAX;
    xMax = yMax = zMax = wMax = -DBL_MAX;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&volumeTable, &iter); hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&iter)) {
        Volume *volPtr = (Volume *)Tcl_GetHashValue(hPtr);
        if (xMin > volPtr->xAxis.min()) {
            xMin = volPtr->xAxis.min();
        }
        if (xMax < volPtr->xAxis.max()) {
            xMax = volPtr->xAxis.max();
        }
        if (yMin > volPtr->yAxis.min()) {
            yMin = volPtr->yAxis.min();
        }
        if (yMax < volPtr->yAxis.max()) {
            yMax = volPtr->yAxis.max();
        }
        if (zMin > volPtr->zAxis.min()) {
            zMin = volPtr->zAxis.min();
        }
        if (zMax < volPtr->zAxis.max()) {
            zMax = volPtr->zAxis.max();
        }
        if (wMin > volPtr->wAxis.min()) {
            wMin = volPtr->wAxis.min();
        }
        if (wMax < volPtr->wAxis.max()) {
            wMax = volPtr->wAxis.max();
        }
    }
    if ((xMin < DBL_MAX) && (xMax > -DBL_MAX)) {
        grid->xAxis.setScale(xMin, xMax);
    }
    if ((yMin < DBL_MAX) && (yMax > -DBL_MAX)) {
        grid->yAxis.setScale(yMin, yMax);
    }
    if ((zMin < DBL_MAX) && (zMax > -DBL_MAX)) {
        grid->zAxis.setScale(zMin, zMax);
    }
    if ((wMin < DBL_MAX) && (wMax > -DBL_MAX)) {
        Volume::valueMin = wMin;
        Volume::valueMax = wMax;
    }
    Volume::updatePending = false;
    TRACE("Leave");
}

void
NanoVis::setHeightmapRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    TRACE("Enter");
    xMin = yMin = zMin = wMin = DBL_MAX;
    xMax = yMax = zMax = wMax = -DBL_MAX;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    for (hPtr = Tcl_FirstHashEntry(&heightmapTable, &iter); hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&iter)) {
        HeightMap *hmPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
        if (xMin > hmPtr->xAxis.min()) {
            xMin = hmPtr->xAxis.min();
        }
        if (xMax < hmPtr->xAxis.max()) {
            xMax = hmPtr->xAxis.max();
        }
        if (yMin > hmPtr->yAxis.min()) {
            yMin = hmPtr->yAxis.min();
        }
        if (yMax < hmPtr->yAxis.max()) {
            yMax = hmPtr->yAxis.max();
        }
        if (zMin > hmPtr->zAxis.min()) {
            zMin = hmPtr->zAxis.min();
        }
        if (zMax < hmPtr->zAxis.max()) {
            zMax = hmPtr->zAxis.max();
        }
        if (wMin > hmPtr->wAxis.min()) {
            wMin = hmPtr->wAxis.min();
        }
        if (wMax < hmPtr->wAxis.max()) {
            wMax = hmPtr->wAxis.max();
        }
    }
    if ((xMin < DBL_MAX) && (xMax > -DBL_MAX)) {
        grid->xAxis.setScale(xMin, xMax);
    }
    if ((yMin < DBL_MAX) && (yMax > -DBL_MAX)) {
        grid->yAxis.setScale(yMin, yMax);
    }
    if ((zMin < DBL_MAX) && (zMax > -DBL_MAX)) {
        grid->zAxis.setScale(zMin, zMax);
    }
    if ((wMin < DBL_MAX) && (wMax > -DBL_MAX)) {
        HeightMap::valueMin = grid->yAxis.min();
        HeightMap::valueMax = grid->yAxis.max();
    }
    for (hPtr = Tcl_FirstHashEntry(&heightmapTable, &iter); hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&iter)) {
        HeightMap *hmPtr;
        hmPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
        hmPtr->mapToGrid(grid);
    }
    HeightMap::updatePending = false;
    TRACE("Leave");
}

void
NanoVis::display()
{
    TRACE("Enter");

    if (flags & MAP_FLOWS) {
#ifdef notdef
        xMin = yMin = zMin = wMin = FLT_MAX, magMin = DBL_MAX;
        xMax = yMax = zMax = wMax = -FLT_MAX, magMax = -DBL_MAX;
#endif
        MapFlows();
        grid->xAxis.setScale(xMin, xMax);
        grid->yAxis.setScale(yMin, yMax);
        grid->zAxis.setScale(zMin, zMax);
    }
    //assert(glGetError()==0);
    if (HeightMap::updatePending) {
        setHeightmapRanges();
    }
    if (Volume::updatePending) {
        setVolumeRanges();
    }

    //start final rendering

    // Need to reset fbo since it may have been changed to default (0)
    bindOffscreenBuffer();

    TRACE("glClear");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

    if (volumeMode) {
        TRACE("volumeMode");
        //3D rendering mode
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_COLOR_MATERIAL);

        //camera setting activated
        cam->initialize();

        //set up the orientation of items in the scene.
        glPushMatrix();

        switch (updir) {
        case X_POS:
            glRotatef(90, 0, 0, 1);
            glRotatef(90, 1, 0, 0);
            break;
        case Y_POS:
            // this is the default
            break;
        case Z_POS:
            glRotatef(-90, 1, 0, 0);
            glRotatef(-90, 0, 0, 1);
            break;
        case X_NEG:
            glRotatef(-90, 0, 0, 1);
            break;
        case Y_NEG:
            glRotatef(180, 0, 0, 1);
            glRotatef(-90, 0, 1, 0);
            break;
        case Z_NEG:
            glRotatef(90, 1, 0, 0);
            break;
        }

        //now render things in the scene
        if (axisOn) {
            draw3dAxis();
        }
        if (grid->isVisible()) {
            grid->render();
        }
        if ((licRenderer != NULL) && (licRenderer->active())) {
            licRenderer->render();
        }

        if ((velocityArrowsSlice != NULL) && (velocityArrowsSlice->enabled())) {
            velocityArrowsSlice->render();
        }
#ifdef notdef
        if ((flowVisRenderer != NULL) && (flowVisRenderer->active())) {
            flowVisRenderer->render();
        }
#endif
        if (flowTable.numEntries > 0) {
            RenderFlows();
        }

        volRenderer->renderAll();

        if (heightmapTable.numEntries > 0) {
            TRACE("render heightmap");
            Tcl_HashEntry *hPtr;
            Tcl_HashSearch iter;
            for (hPtr = Tcl_FirstHashEntry(&heightmapTable, &iter); hPtr != NULL;
                 hPtr = Tcl_NextHashEntry(&iter)) {
                HeightMap *hmPtr;
                hmPtr = (HeightMap *)Tcl_GetHashValue(hPtr);
                if (hmPtr->isVisible()) {
                    hmPtr->render(renderContext);
                }
            }
        }
        glPopMatrix();
    } else {
        //2D rendering mode
        perf->enable();
        planeRenderer->render();
        perf->disable();
    }

    perf->reset();
    CHECK_FRAMEBUFFER_STATUS();
    TRACE("Leave");
}

void 
NanoVis::processCommands()
{
    flags &= ~REDRAW_PENDING;

    TRACE("Enter");

    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

    int status = TCL_OK;

    //  Read and execute as many commands as we can from stdin...
    Tcl_DString cmdbuffer;
    Tcl_DStringInit(&cmdbuffer);
    int nCommands = 0;
    bool isComplete = false;
    while ((!feof(NanoVis::stdin)) && (status == TCL_OK)) {
        //
        //  Read the next command from the buffer.  First time through we
        //  block here and wait if necessary until a command comes in.
        //
        //  BE CAREFUL: Read only one command, up to a newline.  The "volume
        //  data follows" command needs to be able to read the data
        //  immediately following the command, and we shouldn't consume it
        //  here.
        //
        while (!feof(NanoVis::stdin)) {
            int c = fgetc(NanoVis::stdin);
            char ch;
            if (c <= 0) {
                if (errno == EWOULDBLOCK) {
                    break;
                }
                exitService(100);
            }
            ch = (char)c;
            Tcl_DStringAppend(&cmdbuffer, &ch, 1);
            if (ch == '\n') {
                isComplete = Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer));
                if (isComplete) {
                    break;
                }
            }
        }
        // no command? then we're done for now
        if (Tcl_DStringLength(&cmdbuffer) == 0) {
            break;
        }
        if (isComplete) {
            // back to original flags during command evaluation...
            fcntl(0, F_SETFL, flags & ~O_NONBLOCK);
            status = executeCommand(interp, &cmdbuffer);
            // non-blocking for next read -- we might not get anything
            fcntl(0, F_SETFL, flags | O_NONBLOCK);
            isComplete = false;
            nCommands++;
            CHECK_FRAMEBUFFER_STATUS();
        }
    }
    fcntl(0, F_SETFL, flags);

    if (status != TCL_OK) {
        char *msg;
        char hdr[200];
        int msgSize, hdrSize;
        Tcl_Obj *objPtr;

        objPtr = Tcl_GetObjResult(interp);
        msg = Tcl_GetStringFromObj(objPtr, &msgSize);
        hdrSize = sprintf(hdr, "nv>viserror -type internal_error -bytes %d\n", msgSize);
        {
            struct iovec iov[2];

            iov[0].iov_base = hdr;
            iov[0].iov_len = hdrSize;
            iov[1].iov_base = msg;
            iov[1].iov_len = msgSize;
            if (writev(1, iov, 2) < 0) {
                ERROR("write failed: %s", strerror(errno));
            }
        }
        TRACE("Leaving on ERROR");
        return;
    }
    if (feof(NanoVis::stdin)) {
        TRACE("Exiting server on EOF from client");
        exitService(90);
    }

    update();

    bindOffscreenBuffer();  //enable offscreen render

    display();

    readScreen();

    if (feof(NanoVis::stdin)) {
        exitService(90);
    }
#ifdef DO_RLE
    doRle();
    int sizes[2] = {  offsets_size*sizeof(offsets[0]), rle_size };
    TRACE("Writing %d,%d", sizes[0], sizes[1]); 
    write(1, &sizes, sizeof(sizes));
    write(1, offsets, offsets_size*sizeof(offsets[0]));
    write(1, rle, rle_size);    //unsigned byte
#else
    ppmWrite("nv>image -type image -bytes");
#endif
    TRACE("Leave");
}

int 
main(int argc, char **argv)
{
    const char *path;
    char *newPath;

    newPath = NULL;
    path = NULL;
    NanoVis::stdin = stdin;

    fprintf(stdout, "NanoVis %s (build %s)\n", NANOVIS_VERSION, SVN_VERSION);
    fflush(stdout);

    openlog("nanovis", LOG_CONS | LOG_PERROR | LOG_PID, LOG_USER);
    gettimeofday(&NanoVis::startTime, NULL);
    stats.start = NanoVis::startTime;

    /* Initialize GLUT here so it can parse and remove GLUT-specific 
     * command-line options before we parse the command-line below. */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(NanoVis::winWidth, NanoVis::winHeight);
    glutInitWindowPosition(10, 10);
    NanoVis::renderWindow = glutCreateWindow("nanovis");
    glutIdleFunc(NanoVis::idle);

    glutDisplayFunc(NanoVis::display);
    glutReshapeFunc(NanoVis::resizeOffscreenBuffer);

    while (1) {
        static struct option long_options[] = {
            {"infile",  required_argument, NULL, 0},
            {"path",    required_argument, NULL, 2},
            {"debug",   no_argument,       NULL, 3},
            {"record",  required_argument, NULL, 4},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c;

        c = getopt_long(argc, argv, ":dp:i:l:r:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case '?':
            fprintf(stderr, "unknown option -%c\n", optopt);
            return 1;
        case ':':
            if (optopt < 4) {
                fprintf(stderr, "argument missing for --%s option\n", 
                        long_options[optopt].name);
            } else {
                fprintf(stderr, "argument missing for -%c option\n", optopt);
            }
            return 1;
        case 2:
        case 'p':
            path = optarg;
            break;
        case 3:
        case 'd':
            NanoVis::debugFlag = true;
            break;
        case 0:
        case 'i':
            NanoVis::stdin = fopen(optarg, "r");
            if (NanoVis::stdin == NULL) {
                perror(optarg);
                return 2;
            }
            break;
        case 4:
        case 'r':
            Tcl_DString ds;
            char buf[200];

            Tcl_DStringInit(&ds);
            Tcl_DStringAppend(&ds, optarg, -1);
            sprintf(buf, ".%d", getpid());
            Tcl_DStringAppend(&ds, buf, -1);
            NanoVis::recfile = fopen(Tcl_DStringValue(&ds), "w");
            if (NanoVis::recfile == NULL) {
                perror(optarg);
                return 2;
            }
            break;
        default:
            fprintf(stderr,"unknown option '%c'.\n", c);
            return 1;
        }
    }     
    if (path == NULL) {
        char *p;

        // See if we can derive the path from the location of the program.
        // Assume program is in the form <path>/bin/nanovis.
        path = argv[0];
        p = strrchr((char *)path, '/');
        if (p != NULL) {
            *p = '\0';
            p = strrchr((char *)path, '/');
        }
        if (p == NULL) {
            TRACE("path not specified");
            return 1;
        }
        *p = '\0';
        newPath = new char[(strlen(path) + 15) * 2 + 1];
        sprintf(newPath, "%s/lib/shaders:%s/lib/resources", path, path);
        path = newPath;
    }

    FilePath::getInstance()->setWorkingDirectory(argc, (const char**) argv);

#ifdef notdef
    signal(SIGPIPE, SIG_IGN);
#endif
    initService();

    NanoVis::init(path);
    if (newPath != NULL) {
        delete [] newPath;
    }
    NanoVis::initGL();

    NanoVis::interp = initTcl();

    NanoVis::resizeOffscreenBuffer(NanoVis::winWidth, NanoVis::winHeight);

    glutMainLoop();

    exitService(80);
}

void 
NanoVis::removeVolume(Volume *volPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&volumeTable, volPtr->name());
    if (hPtr != NULL) {
        Tcl_DeleteHashEntry(hPtr);
    }
    delete volPtr;
}
