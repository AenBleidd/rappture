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
 *  Copyright (c) 2004-2006  Purdue Research Foundation
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
#include <cmath>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpEncode.h>

#include <R2/R2FilePath.h>
#include <R2/R2Fonts.h>

#include <vrutil/vrFilePath.h>

#include <BMPImageLoaderImpl.h>
#include <ImageLoaderFactory.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include "config.h"
#include "nanovis.h"
#include "define.h"

#include "FlowCmd.h"
#include "Grid.h"
#include "HeightMap.h"
#include "NvCamera.h"
#include "NvEventLog.h"
#include "RenderContext.h"
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
    size_t nFrames;		/* # of frames sent to client. */
    size_t nBytes;		/* # of bytes for all frames. */
    size_t nCommands;		/* # of commands executed */
    double cmdTime;		/* Elasped time spend executing commands. */
    struct timeval start;	/* Start of elapsed time. */
} Stats;

static Stats stats;

// STATIC MEMBER DATA
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
#if PLANE_CMD
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

graphics::RenderContext *NanoVis::renderContext = NULL;
NvLIC *NanoVis::licRenderer = NULL;
R2Fonts *NanoVis::fonts;

FILE *NanoVis::stdin = NULL;
FILE *NanoVis::logfile = NULL;
FILE *NanoVis::recfile = NULL;

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

PerfQuery *perf = NULL;                        //perfromance counter

// Variables for mouse events

#ifdef OLD_CAMERA
// Default camera rotation angles.
const float def_rot_x = 90.0f;
const float def_rot_y = 180.0f;
const float def_rot_z = -135.0f;

// Default camera target.
const float def_target_x = 0.0f;
const float def_target_y = 0.0f;
const float def_target_z = 0.0f; //100.0f; 

// Default camera location.
const float def_eye_x = 0.0f;
const float def_eye_y = 0.0f;
const float def_eye_z = -2.5f;

#else

// Default camera rotation angles.
const float def_rot_x = 0.0f; // 45.0f;
const float def_rot_y = 0.0f; // -45.0f;
const float def_rot_z = 0.0f;

// Default camera target.
const float def_target_x = 0.0f;
const float def_target_y = 0.0f;
const float def_target_z = 0.0f;

// Default camera location.
const float def_eye_x = 0.0f;
const float def_eye_y = 0.0f;
const float def_eye_z = 2.5f;
#endif

#ifndef XINETD
// Last locations mouse events
static int left_last_x;
static int left_last_y;
static int right_last_x;
static int right_last_y;
static bool left_down = false;
static bool right_down = false;
#endif /*XINETD*/

// Image based flow visualization slice location
// FLOW
float NanoVis::_licSlice = 0.5f;
int NanoVis::_licAxis = 2; // z axis

void
NanoVis::removeAllData()
{
    //
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

#if KEEPSTATS

static int
writeStats(const char *who, int code) 
{
    double start, finish;
    pid_t pid;
    char buf[BUFSIZ];
    Tcl_DString ds;

    {
        struct timeval tv;

        /* Get ending time.  */
        gettimeofday(&tv, NULL);
        finish = CVT2SECS(tv);
        tv = stats.start;
        start = CVT2SECS(tv);
    }
    pid = getpid();
    Tcl_DStringInit(&ds);

    sprintf(buf, "<session server=\"%s\" ", who);
    Tcl_DStringAppend(&ds, buf, -1);

    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    Tcl_DStringAppend(&ds, "host=\"", -1);
    Tcl_DStringAppend(&ds, buf, -1);
    Tcl_DStringAppend(&ds, "\" ", -1);

    strcpy(buf, ctime(&stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppend(&ds, "date=\"", -1);
    Tcl_DStringAppend(&ds, buf, -1);
    Tcl_DStringAppend(&ds, "\" ", -1);

    sprintf(buf, "date_secs=\"%ld\" ", stats.start.tv_sec);
    Tcl_DStringAppend(&ds, buf, -1);

    sprintf(buf, "pid=\"%d\" ", pid);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "num_frames=\"%lu\" ", (unsigned long int)stats.nFrames);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "frame_bytes=\"%lu\" ", (unsigned long int)stats.nBytes);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "num_commands=\"%lu\" ", (unsigned long int)stats.nCommands);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "cmd_time=\"%g\" ", stats.cmdTime);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "session_time=\"%g\" ", finish - start);
    Tcl_DStringAppend(&ds, buf, -1);
    sprintf(buf, "status=\"%d\" ", code);
    Tcl_DStringAppend(&ds, buf, -1);
    {
        long clocksPerSec = sysconf(_SC_CLK_TCK);
        double clockRes = 1.0 / clocksPerSec;
        struct tms tms;

        memset(&tms, 0, sizeof(tms));
        times(&tms);
        sprintf(buf, "utime=\"%g\" ", tms.tms_utime * clockRes);
        Tcl_DStringAppend(&ds, buf, -1);
        sprintf(buf, "stime=\"%g\" ", tms.tms_stime * clockRes);
        Tcl_DStringAppend(&ds, buf, -1);
        sprintf(buf, "cutime=\"%g\" ", tms.tms_cutime * clockRes);
        Tcl_DStringAppend(&ds, buf, -1);
        sprintf(buf, "cstime=\"%g\" ", tms.tms_cstime * clockRes);
        Tcl_DStringAppend(&ds, buf, -1);
    }
    Tcl_DStringAppend(&ds, "/>\n", -1);

    {
        int f;
        ssize_t length;
        int result;

#define STATSDIR	"/var/tmp/visservers"
#define STATSFILE	STATSDIR "/" "data.xml"
        if (access(STATSDIR, X_OK) != 0) {
            mkdir(STATSDIR, 0770);
        }
        length = Tcl_DStringLength(&ds);
        f = open(STATSFILE, O_APPEND | O_CREAT | O_WRONLY, 0600);
        result = FALSE;
        if (f < 0) {
            goto error;
        }
        if (write(f, Tcl_DStringValue(&ds), length) != length) {
            goto error;
        }
        result = TRUE;
    error:
        if (f >= 0) {
            close(f);
        }
        Tcl_DStringFree(&ds);
        return result;
    }
}
#endif

static void
doExit(int code)
{
    TRACE("in doExit: %d\n", code);
    NanoVis::removeAllData();

    NvShader::exitCg();

#ifdef EVENTLOG
    NvExitEventLog();
#endif

#ifdef XINETD
    NvExitService();
#endif

#if KEEPSTATS
    writeStats("nanovis", code);
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

    TRACE("command: '%s'", Tcl_DStringValue(dsPtr));

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
    TRACE("leaving executeCommand status=%d\n", result);
    return result;
}

void
NanoVis::pan(float dx, float dy)
{
    /* Move the camera and its target by equal amounts along the x and y
     * axes. */
    TRACE("pan: x=%f, y=%f\n", dx, dy);

#ifdef OLD_CAMERA
    cam->x(def_eye_x + dx);
#else
    cam->x(def_eye_x - dx);
#endif
    cam->y(def_eye_y + dy);
    TRACE("set eye to %f %f\n", cam->x(), cam->y());
#ifdef OLD_CAMERA
    cam->xAim(def_target_x + dx);
#else
    cam->xAim(def_target_x - dx);
#endif
    cam->yAim(def_target_y + dy);
    TRACE("set aim to %f %f\n", cam->xAim(), cam->yAim());
}

void
NanoVis::zoom(float z)
{
    /* Move the camera and its target by equal amounts along the x and y
     * axes. */
    TRACE("zoom: z=%f\n", z);

    cam->z(def_eye_z / z);
    TRACE("set cam z to %f\n", cam->z());
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
    volPtr = new Volume(0.f, 0.f, 0.f, width, height, depth, 1., n_component,
                        data, vmin, vmax, nzero_min);
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
        tfPtr = new TransferFunction(n, data);
        tfPtr->name(Tcl_GetHashKey(&tfTable, hPtr));
        Tcl_SetHashValue(hPtr, tfPtr);
    } else {
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
    TRACE("in renderLegend\n");

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
    TRACE("leaving renderLegend\n");
    return TCL_OK;
}

//initialize frame buffer objects for offscreen rendering
void
NanoVis::initOffscreenBuffer()
{
    TRACE("in initOffscreenBuffer\n");
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
        doExit(3);
    }

    TRACE("leaving initOffscreenBuffer\n");
}

//resize the offscreen buffer 
void 
NanoVis::resizeOffscreenBuffer(int w, int h)
{
    TRACE("in resizeOffscreenBuffer(%d, %d)\n", w, h);
    if ((w == winWidth) && (h == winHeight)) {
        return;
    }
    winWidth = w;
    winHeight = h;

    if (fonts) {
        fonts->resize(w, h);
    }
    TRACE("screenBuffer size: %d %d\n", w, h);

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

    TRACE("before deleteframebuffers\n");
    glDeleteFramebuffersEXT(1, &_finalFbo);

    TRACE("reinitialize FBO\n");
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
        doExit(3);
    }

    TRACE("change camera\n");
    //change the camera setting
    cam->setScreenSize(0, 0, winWidth, winHeight);
    planeRenderer->setScreenSize(winWidth, winHeight);

    TRACE("leaving resizeOffscreenBuffer(%d, %d)\n", w, h);
}

#if PROTOTYPE
/*
 * FIXME: This routine is fairly expensive (60000 floating pt divides).
 *      I've put an ifdef around the call to it so that the released
 *      builds don't include it.  Define PROTOTYPE to 1 in config.h
 *      to turn it back on.
 */
static void 
makeTest2DData()
{
    int w = 300;
    int h = 200;
    float *data = new float[w*h];

    //procedurally make a gradient plane
    for (int j = 0; j < h; j++){
        for (int i = 0; i < w; i++){
            data[w*j+i] = float(i)/float(w);
        }
    }

    NanoVis::plane[0] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, data);
    delete[] data;
}
#endif

static
void cgErrorCallback(void)
{
    if (!NvShader::printErrorInfo()) {
        TRACE("Cg error, exiting...\n");
        doExit(-1);
    }
}

void NanoVis::init(const char* path)
{
    // print OpenGL driver information
    TRACE("-----------------------------------------------------------\n");
    TRACE("OpenGL version: %s\n", glGetString(GL_VERSION));
    TRACE("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
    TRACE("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    TRACE("-----------------------------------------------------------\n");

    if (path == NULL) {
        ERROR("No path defined for shaders or resources\n");
        doExit(1);
    }
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        ERROR("Can't init GLEW: %s\n", glewGetErrorString(err));
        doExit(1);
    }
    TRACE("Using GLEW %s\n", glewGetString(GLEW_VERSION));

    // OpenGL 2.1 includes VBOs, PBOs, MRT, NPOT textures, point parameters, point sprites,
    // GLSL 1.2, and occlusion queries.
    if (!GLEW_VERSION_2_1) {
        ERROR("OpenGL version 2.1 or greater is required\n");
        doExit(1);
    }

    // Additional extensions not in 2.1 core

    // Framebuffer objects were promoted in 3.0
    if (!GLEW_EXT_framebuffer_object) {
        ERROR("EXT_framebuffer_oject extension is required\n");
        doExit(1);
    }
    // Rectangle textures were promoted in 3.1
    // FIXME: can use NPOT in place of rectangle textures
    if (!GLEW_ARB_texture_rectangle) {
        ERROR("ARB_texture_rectangle extension is required\n");
        doExit(1);
    }
#ifdef HAVE_FLOAT_TEXTURES
    // Float color buffers and textures were promoted in 3.0
    if (!GLEW_ARB_texture_float ||
        !GLEW_ARB_color_buffer_float) {
        ERROR("ARB_texture_float and ARB_color_buffer_float extensions are required\n");
        doExit(1);
    }
#endif
    // FIXME: should use ARB programs or (preferably) a GLSL profile for portability
    if (!GLEW_NV_vertex_program3 ||
        !GLEW_NV_fragment_program2) {
        ERROR("NV_vertex_program3 and NV_fragment_program2 extensions are required\n");
        doExit(1);
    }

    if (!R2FilePath::getInstance()->setPath(path)) {
        ERROR("can't set file path to %s\n", path);
        doExit(1);
    }

    vrFilePath::getInstance()->setPath(path);

    ImageLoaderFactory::getInstance()->addLoaderImpl("bmp", new BMPImageLoaderImpl());

    NvShader::initCg();
    NvShader::setErrorCallback(cgErrorCallback);

    fonts = new R2Fonts();
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
    TRACE("in initGL\n");
    //buffer to store data read from the screen
    if (screenBuffer) {
        delete[] screenBuffer;
        screenBuffer = NULL;
    }
    screenBuffer = new unsigned char[4*winWidth*winHeight];
    assert(screenBuffer != NULL);

    //create the camera with default setting
    cam = new NvCamera(0, 0, winWidth, winHeight,
                       def_eye_x, def_eye_y, def_eye_z,          /* location. */
                       def_target_x, def_target_y, def_target_z, /* target. */
                       def_rot_x, def_rot_y, def_rot_z);         /* angle. */

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
    renderContext = new graphics::RenderContext();

    //create a 2D plane renderer
    planeRenderer = new PlaneRenderer(winWidth, winHeight);
#if PROTOTYPE
    make_test_2D_data();
    planeRenderer->addPlane(plane[0], getTransfunc("default"));
#endif

    //assert(glGetError()==0);

    TRACE("leaving initGL\n");
}

#if DO_RLE
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

        TRACE("Writing %s\n", filename);
        f = fopen(filename, "wb");
        if (f == 0) {
            ERROR("cannot create file\n");
        }
    } else {
        f = fopen("/tmp/image.bmp", "wb");
        if (f == 0) {
            ERROR("cannot create file\n");
        }
    }
    if (fwrite(header, SIZEOF_BMP_HEADER, 1, f) != 1) {
        ERROR("can't write header: short write\n");
    }
    if (fwrite(screenBuffer, (3*winWidth+pad)*winHeight, 1, f) != 1) {
        ERROR("can't write data: short write\n");
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

    TRACE("Enter ppmWrite (%dx%d)\n", winWidth, winHeight);
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
        ERROR("write failed: %s\n", strerror(errno));
    }
    free(iov);
    stats.nFrames++;
    stats.nBytes += (bytesPerRow * winHeight);
    TRACE("Leaving ppmWrite (%dx%d)\n", winWidth, winHeight);
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
        ERROR("write failed: %s\n", strerror(errno));
    }
    delete [] iov;
    // stats.nFrames++;
    // stats.nBytes += (bytesPerRow * winHeight);
}


/*----------------------------------------------------*/
void
NanoVis::idle()
{
    TRACE("in idle()\n");
    glutSetWindow(renderWindow);

#ifdef XINETD
    xinetdListen();
#else
    glutPostRedisplay();
#endif
    TRACE("leaving idle()\n");
}

void 
NanoVis::displayOffscreenBuffer()
{
    TRACE("in display_offscreen_buffer\n");

    glEnable(GL_TEXTURE_2D);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindTexture(GL_TEXTURE_2D, _finalColorTex);

    glViewport(0, 0, winWidth, winHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.,1.,1.);                //MUST HAVE THIS LINE!!!
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(winWidth, 0);
        glTexCoord2f(1, 1); glVertex2f(winWidth, winHeight);
        glTexCoord2f(0, 1); glVertex2f(0, winHeight);
    }
    glEnd();

    TRACE("leaving display_offscreen_buffer\n");
}

#if 0
//oddeven sort on GPU
static void
sortstep()
{
    // perform one step of the current sorting algorithm

#ifdef notdef
    // swap buffers
    int sourceBuffer = targetBuffer;
    targetBuffer = (targetBuffer+1)%2;
    int pstage = (1<<stage);
    int ppass  = (1<<pass);
    int activeBitonicShader = 0;

#ifdef _WIN32
    buffer->BindBuffer(wglTargets[sourceBuffer]);
#else
    buffer->BindBuffer(glTargets[sourceBuffer]);
#endif
    if (buffer->IsDoubleBuffered()) glDrawBuffer(glTargets[targetBuffer]);
#endif

    checkGLError("after db");

    int pstage = (1<<stage);
    int ppass  = (1<<pass);
    //int activeBitonicShader = 0;

    // switch on correct sorting shader
    oddevenMergeSort.bind();
    glUniform3fARB(oddevenMergeSort.getUniformLocation("Param1"), float(pstage+pstage),
                   float(ppass%pstage), float((pstage+pstage)-(ppass%pstage)-1));
    glUniform3fARB(oddevenMergeSort.getUniformLocation("Param2"),
                   float(psys_width), float(psys_height), float(ppass));
    glUniform1iARB(oddevenMergeSort.getUniformLocation("Data"), 0);
    staticdebugmsg("sort","stage "<<pstage<<" pass "<<ppass);

    // This clear is not necessary for sort to function. But if we are in
    // interactive mode unused parts of the texture that are visible will look
    // bad.
#ifdef notdef
    if (!perfTest) glClear(GL_COLOR_BUFFER_BIT);

    buffer->Bind();
    buffer->EnableTextureTarget();
#endif

    // Initiate the sorting step on the GPU a full-screen quad
    glBegin(GL_QUADS);
    {
#ifdef notdef
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f);
        glVertex2f(-1.0f,-1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f);
        glVertex2f(1.0f,-1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f);
        glVertex2f(1.0f,1.0f);
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f);
        glVertex2f(-1.0f,1.0f);
#endif
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f);
        glVertex2f(0.,0.);
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f);
        glVertex2f(float(psys_width), 0.);
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f);
        glVertex2f(float(psys_width), float(psys_height));
        glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f);
        glVertex2f(0., float(psys_height));
    }
    glEnd();

    // switch off sorting shader
    oddevenMergeSort.release();

    //buffer->DisableTextureTarget();

    //assert(glGetError()==0);
}
#endif

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
    if (volRenderer->_volumeInterpolator->isStarted()) {
        struct timeval clock;
        gettimeofday(&clock, NULL);
        double elapsed_time;

        elapsed_time = clock.tv_sec + clock.tv_usec/1000000.0 -
            volRenderer->_volumeInterpolator->getStartTime();

        TRACE("%lf %lf\n", elapsed_time, 
               volRenderer->_volumeInterpolator->getInterval());
        float fraction;
        float f;

        f = fmod((float) elapsed_time, (float)volRenderer->_volumeInterpolator->getInterval());
        if (f == 0.0) {
            fraction = 0.0f;
        } else {
            fraction = f / volRenderer->_volumeInterpolator->getInterval();
        }
        TRACE("fraction : %f\n", fraction);
        volRenderer->_volumeInterpolator->update(fraction);
    }
}

void
NanoVis::setVolumeRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    TRACE("in SetVolumeRanges\n");
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
    TRACE("leaving SetVolumeRanges\n");
}

void
NanoVis::setHeightmapRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    TRACE("in setHeightmapRanges\n");
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
    TRACE("leaving setHeightmapRanges\n");
}

/*----------------------------------------------------*/
void
NanoVis::display()
{
    TRACE("in display\n");
#ifdef notdef
    if (flags & MAP_FLOWS) {
        xMin = yMin = zMin = wMin = FLT_MAX, magMin = DBL_MAX;
        xMax = yMax = zMax = wMax = -FLT_MAX, magMax = -DBL_MAX;
    }
#endif
    if (flags & MAP_FLOWS) {
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

    TRACE("in display: glClear\n");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

    if (volumeMode) {
        TRACE("in display: volumeMode\n");
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

        //soft_display_verts();
        //perf->enable();
        //perf->disable();
        //TRACE("particle pixels: %d\n", perf->get_pixel_count());
        //perf->reset();

        //perf->enable();
        volRenderer->renderAll();
        //perf->disable();

        if (heightmapTable.numEntries > 0) {
            TRACE("in display: render heightmap\n");
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
    TRACE("leaving display\n");
}

#ifndef XINETD
void 
NanoVis::mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            left_last_x = x;
            left_last_y = y;
            left_down = true;
            right_down = false;
        } else {
            left_down = false;
            right_down = false;
        }
    } else {
        //TRACE("right mouse\n");

        if (state == GLUT_DOWN) {
            //TRACE("right mouse down\n");
            right_last_x = x;
            right_last_y = y;
            left_down = false;
            right_down = true;
        } else {
            //TRACE("right mouse up\n");
            left_down = false;
            right_down = false;
        }
    }
}

void
NanoVis::updateRot(int delta_x, int delta_y)
{
    Vector3 angle;

    angle = cam->rotate();
    angle.x += delta_x;
    angle.y += delta_y;

    if (angle.x > 360.0) {
        angle.x -= 360.0;
    } else if(angle.x < -360.0) {
        angle.x += 360.0;
    }
    if (angle.y > 360.0) {
        angle.y -= 360.0;
    } else if(angle.y < -360.0) {
        angle.y += 360.0;
    }
    cam->rotate(angle);
}

void 
NanoVis::updateTrans(int delta_x, int delta_y, int delta_z)
{
    cam->x(cam->x() + delta_x * 0.03);
    cam->y(cam->y() + delta_y * 0.03);
    cam->z(cam->z() + delta_z * 0.03);
}

#ifdef notdef
void NanoVis::initParticle()
{
    flowVisRenderer->initialize();
    licRenderer->makePatterns();
}

static
void addVectorField(const char* filename, const char* vf_name, 
                    const char* plane_name1, const char* plane_name2, 
                    const Vector4& color1, const Vector4& color2)
{
    Rappture::Outcome result;
    Rappture::Buffer buf;

    buf.load(filename);
    int n = NanoVis::n_volumes;
    if (load_vector_stream2(result, n, buf.size(), buf.bytes())) {
        Volume *volPtr = NanoVis::volume[n];
        if (volPtr != NULL) {
            volPtr->numSlices(256-n);
            // volPtr->numSlices(512-n);
            volPtr->disableCutplane(0);
            volPtr->disableCutplane(1);
            volPtr->disableCutplane(2);
            volPtr->transferFunction(NanoVis::getTransfunc("default"));

            float dx0 = -0.5;
            float dy0 = -0.5*volPtr->height/volPtr->width;
            float dz0 = -0.5*volPtr->depth/volPtr->width;
            volPtr->move(Vector3(dx0, dy0, dz0));
            //volPtr->data(true);
            volPtr->data(false);
            NanoVis::flowVisRenderer->addVectorField(vf_name, volPtr, 
                volPtr->location(),
                1.0f,
                volPtr->height / (float)volPtr->width,
                volPtr->depth  / (float)volPtr->width,
                1.0f);
            NanoVis::flowVisRenderer->activateVectorField(vf_name);

            //////////////////////////////////
            // ADD Particle Injection Plane1
            NanoVis::flowVisRenderer->addPlane(vf_name, plane_name1);
            NanoVis::flowVisRenderer->setPlaneAxis(vf_name, plane_name1, 0);
            NanoVis::flowVisRenderer->setPlanePos(vf_name, plane_name1, 0.9);
            NanoVis::flowVisRenderer->setParticleColor(vf_name, plane_name1, color1);
            // ADD Particle Injection Plane2
            NanoVis::flowVisRenderer->addPlane(vf_name, plane_name2);
            NanoVis::flowVisRenderer->setPlaneAxis(vf_name, plane_name2, 0);
            NanoVis::flowVisRenderer->setPlanePos(vf_name, plane_name2, 0.2);
            NanoVis::flowVisRenderer->setParticleColor(vf_name, plane_name2, color2);
            NanoVis::flowVisRenderer->initialize(vf_name);

            NanoVis::flowVisRenderer->activatePlane(vf_name, plane_name1);
            NanoVis::flowVisRenderer->activatePlane(vf_name, plane_name2);

            NanoVis::licRenderer->
                setVectorField(volPtr->id,
                               *(volPtr->get_location()),
                               1.0f / volPtr->aspectRatioWidth,
                               1.0f / volPtr->aspectRatioHeight,
                               1.0f / volPtr->aspectRatioDepth,
                               volPtr->wAxis.max());
        }
    }
    //NanoVis::initParticle();
}
#endif

void 
NanoVis::keyboard(unsigned char key, int x, int y)
{
#ifdef EVENTLOG
    if (log) {
        float param[3];
        param[0] = cam->x();
        param[1] = cam->y();
        param[2] = cam->z();
        Event* tmp = new Event(EVENT_MOVE, param, NvGetTimeInterval());
        tmp->write(event_log);
        delete tmp;
    }
#endif

    switch (key) {
    case 'a' :
        {
            TRACE("flowvis active\n");
            char cmd[] = {
                "foreach flow [flow names] {\n"
                "    $flow configure -hide no -slice yes\n"
                "}\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->active(true);
            licRenderer->active(true);
#endif
        }
        break;
    case 'd' :
        {
            TRACE("flowvis deactived\n");
            char cmd[] = {
                "foreach flow [flow names] {\n"
                "    $flow configure -hide yes -slice no\n"
                "}\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->active(false);
            licRenderer->active(false);
#endif
        }
        break;
    case '1' :
        {
            TRACE("add vector field\n");
            char cmd[] = {
                "flow create flow1\n"
                "flow1 data file data/flowvis_dx_files/jwire/J-wire-vec.dx 3\n"
                "flow1 particles add plane1 -color { 0 0 1 1 }\n"
                "flow1 particles add plane2 -color { 0 1 1 1 }\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            addVectorField("data/flowvis_dx_files/jwire/J-wire-vec.dx",
                           "vf_name2", "plane_name1", "plane_name2", Vector4(0, 0, 1, 1), Vector4(0, 1, 1, 1));
#endif
        }
        break;
    case '2' :
        {
            char cmd[] = {
                "flow create flow2\n"
                "flow2 data file data/flowvis_dx_files/3DWireLeakage/SiO2/SiO2.dx 3\n"
                "flow2 particles add plane1 -color { 1 0 0 1 }\n"
                "flow2 particles add plane2 -color { 1 1 0 1 }\n"
            };
            Tcl_Eval(interp, cmd);
            TRACE("add vector field\n");
#ifdef notdef
            addVectorField("data/flowvis_dx_files/3DWireLeakage/SiO2/SiO2.dx",
                           "vf_name1", "plane_name1", "plane_name2", Vector4(1, 0, 0, 1), Vector4(1, 1, 0, 1));
#endif
        }
        break;
    case '3':
        {
            TRACE("activate\n");
            char cmd[] = {
                "flow1 particles add plane2 -hide no\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            NanoVis::flowVisRenderer->activatePlane("vf_name1", "plane_name2"); 
#endif
        }
        break;
    case '4' :
        {
            TRACE("deactivate\n");
            char cmd[] = {
                "flow1 particles add plane2 -hide yes\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->deactivatePlane("vf_name1", "plane_name2"); 
#endif
        }
        break;
    case '5' :
        {
            TRACE("vector field deleted (vf_name2)\n");
            char cmd[] = {
                "flow delete flow2\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->removeVectorField("vf_name2");
#endif
        }
        break;
    case '6' :
        {
            TRACE("add device shape\n");
            char cmd[] = {
                "flow1 box add box1 -corner1 {0 0 0} -corner2 {30 3 3} -color { 1 0 0 1 }\n"
                "flow1 box add box2 -corner1 {0 -1 -1} -corner2 {30 4 4} -color { 0 1 0 1 }\n"
                "flow1 box add box3 -corner1 {10 -1.5 -1} -corner2 {20 4.5 4.5} -color { 0 0 1 1 }\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            NvDeviceShape shape;
            shape.min.set(0, 0, 0);
            shape.max.set(30, 3, 3);
            shape.color.set(1, 0, 0, 1);
            flowVisRenderer->addDeviceShape("vf_name1", "device1", shape);
            shape.min.set(0, -1, -1);
            shape.max.set(30, 4, 4);
            shape.color.set(0, 1, 0, 1);
            flowVisRenderer->addDeviceShape("vf_name1", "device2", shape);
            shape.min.set(10, -1.5, -1);
            shape.max.set(20, 4.5, 4.5);
            shape.color.set(0, 0, 1, 1);
            flowVisRenderer->addDeviceShape("vf_name1", "device3", shape);
            flowVisRenderer->activateDeviceShape("vf_name1");
#endif
        }
        break;
    case '7' :
        {
            TRACE("hide shape \n");
            char cmd[] = {
                "flow1 box configure box1 -hide yes\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->deactivateDeviceShape("vf_name1");
#endif
        }
        break;
    case '8' :
        {
            TRACE("show shape\n");
            char cmd[] = {
                "flow1 box configure box1 -hide no\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->activateDeviceShape("vf_name1");
#endif
        }
        break;
    case '9' :
        {
            TRACE("show a shape \n");
            char cmd[] = {
                "flow1 box configure box3 -hide no\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->activateDeviceShape("vf_name1", "device3");
#endif
        }
        break;
    case '0' :
        {
            TRACE("delete a shape \n");
            char cmd[] = {
                "flow1 box delete box3\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->deactivateDeviceShape("vf_name1", "device3");
#endif
        }
        break;
    case 'r' :
        {
            TRACE("reset \n");
            char cmd[] = {
                "flow reset\n"
            };
            Tcl_Eval(interp, cmd);
#ifdef notdef
            flowVisRenderer->reset();
            licRenderer->reset();
#endif
        }
        break;
    }
}

void 
NanoVis::motion(int x, int y)
{
    int old_x, old_y;

    if (left_down) {
        old_x = left_last_x;
        old_y = left_last_y;
    } else if (right_down) {
        old_x = right_last_x;
        old_y = right_last_y;
    }

    int delta_x = x - old_x;
    int delta_y = y - old_y;

    //more coarse event handling
    //if(abs(delta_x)<10 && abs(delta_y)<10)
    //return;

    if (left_down) {
        left_last_x = x;
        left_last_y = y;

        updateRot(-delta_y, -delta_x);
    } else if (right_down) {
        //TRACE("right mouse motion (%d,%d)\n", x, y);

        right_last_x = x;
        right_last_y = y;

        updateTrans(0, 0, delta_x);
    }

#ifdef EVENTLOG
    Vector3 angle = cam->rotate();
    Event* tmp = new Event(EVENT_ROTATE, &angle, NvGetTimeInterval());
    tmp->write(event_log);
    delete tmp;
#endif
    glutPostRedisplay();
}

void 
NanoVis::render()
{

#ifdef notdef
    if ((licRenderer != NULL) && (licRenderer->active())) {
        licRenderer->convolve();
    }
#else
    if (licRenderer != NULL) {
        licRenderer->convolve();
    }
#endif

#ifdef notdef
    if ((flowVisRenderer != NULL) && (flowVisRenderer->active())) {
        flowVisRenderer->advect();
    }
#endif
    update();
    display();
    glutSwapBuffers();
}

void 
NanoVis::resize(int x, int y)
{
    glViewport(0, 0, x, y);
}

#endif /*XINETD*/

void 
NanoVis::xinetdListen()
{
    flags &= ~REDRAW_PENDING;

    TRACE("Enter xinetdListen\n");

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
        TRACE("in xinetdListen: EOF=%d\n", feof(NanoVis::stdin));
        while (!feof(NanoVis::stdin)) {
            int c = fgetc(NanoVis::stdin);
            char ch;
            if (c <= 0) {
                if (errno == EWOULDBLOCK) {
                    break;
                }
                doExit(100);
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
        const char *string;
        int nBytes;

        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        TRACE("errorInfo=(%s)\n", string);
        nBytes = strlen(string);
        struct iovec iov[3];
        iov[0].iov_base = (char *)"NanoVis Server Error: ";
        iov[0].iov_len = strlen((char *)iov[0].iov_base);
        iov[1].iov_base = (char *)string;
        iov[1].iov_len = nBytes;
        iov[2].iov_len = 1;
        iov[2].iov_base = (char *)'\n';
        if (writev(1, iov, 3) < 0) {
            ERROR("write failed: %s\n", strerror(errno));
        }
        TRACE("Leaving xinetd_listen on ERROR\n");
        return;
    }
    if (feof(NanoVis::stdin)) {
        doExit(90);
    }

    update();

    bindOffscreenBuffer();  //enable offscreen render

    display();

#ifdef XINETD
    readScreen();
#else
    displayOffscreenBuffer(); //display the final rendering on screen
    readScreen();
    glutSwapBuffers();
#endif

    if (feof(NanoVis::stdin)) {
        doExit(90);
    }
#if DO_RLE
    do_rle();
    int sizes[2] = {  offsets_size*sizeof(offsets[0]), rle_size };
    TRACE("Writing %d,%d\n", sizes[0], sizes[1]); 
    write(1, &sizes, sizeof(sizes));
    write(1, offsets, offsets_size*sizeof(offsets[0]));
    write(1, rle, rle_size);    //unsigned byte
#else
    ppmWrite("\nnv>image -type image -bytes");
#endif
    TRACE("Leaving xinetd_listen OK\n");
}

int 
main(int argc, char **argv)
{
    const char *path;
    char *newPath;
    struct timeval tv;

    newPath = NULL;
    path = NULL;
    NanoVis::stdin = stdin;

    fprintf(stdout, "NanoVis %s\n", NANOVIS_VERSION);
    fflush(stdout);

    openlog("nanovis", LOG_CONS | LOG_PERROR | LOG_PID, LOG_USER);
    gettimeofday(&tv, NULL);
    stats.start = tv;

    /* Initialize GLUT here so it can parse and remove GLUT-specific 
     * command-line options before we parse the command-line below. */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(NanoVis::winWidth, NanoVis::winHeight);
    glutInitWindowPosition(10, 10);
    NanoVis::renderWindow = glutCreateWindow("nanovis");
    glutIdleFunc(NanoVis::idle);

#ifndef XINETD
    glutMouseFunc(NanoVis::mouse);
    glutMotionFunc(NanoVis::motion);
    glutKeyboardFunc(NanoVis::keyboard);
    glutReshapeFunc(NanoVis::resize);
    glutDisplayFunc(NanoVis::render);
#else
    glutDisplayFunc(NanoVis::display);
    glutReshapeFunc(NanoVis::resizeOffscreenBuffer);
#endif

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

#ifdef XINETD
        path = argv[0];
        p = strrchr((char *)path, '/');
        if (p != NULL) {
            *p = '\0';
            p = strrchr((char *)path, '/');
        }
        if (p == NULL) {
            TRACE("path not specified\n");
            return 1;
        }
        *p = '\0';
        newPath = new char[(strlen(path) + 15) * 2 + 1];
        sprintf(newPath, "%s/lib/shaders:%s/lib/resources", path, path);
        path = newPath;
#else
        char buff[256];
        getcwd(buff, 255);
        p = strrchr(buff, '/');
        if (p != NULL) {
            *p = '\0';
        }
        newPath = new char[(strlen(buff) + 15) * 2 + 1];
        sprintf(newPath, "%s/lib/shaders:%s/lib/resources", buff, buff);
        path = newPath;
#endif
    }

    R2FilePath::getInstance()->setWorkingDirectory(argc, (const char**) argv);
    vrFilePath::getInstance()->setWorkingDirectory(argc, (const char**) argv);

#ifdef XINETD
#ifdef notdef
    signal(SIGPIPE, SIG_IGN);
#endif
    NvInitService();
#endif

    NanoVis::init(path);
    if (newPath != NULL) {
        delete [] newPath;
    }
    NanoVis::initGL();
#ifdef EVENTLOG
    NvInitEventLog();
#endif
    NanoVis::interp = initTcl();
    NanoVis::resizeOffscreenBuffer(NanoVis::winWidth, NanoVis::winHeight);

    glutMainLoop();
    doExit(80);
}

int
NanoVis::render2dContour(HeightMap* heightmap, int width, int height)
{
    int old_width = winWidth;
    int old_height = winHeight;

    resizeOffscreenBuffer(width, height);

    /*
      planeRenderer->setScreenSize(width, height);

      // generate data for the legend
      float data[512];
      for (int i=0; i < 256; i++) {
          data[i] = data[i+256] = (float)(i/255.0);
      }
      plane[0] = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
      int index = planeRenderer->addPlane(plane[0], tf);
      planeRenderer->setActivePlane(index);

      bindOffscreenBuffer();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

      //planeRenderer->render();
      // INSOO : is going to implement here for the topview of the heightmap
      heightmap->render(renderContext);

      // INSOO
      glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);
      //glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, screen_buffer); // INSOO's
      */


    // HELP ME
    // GEORGE
    // I am not sure what I should do
    //char prefix[200];
    //sprintf(prefix, "nv>height_top_view %s %g %g", volArg, min, max);
    //ppmWrite(prefix);
    //write(1, "\n", 1);
    //planeRenderer->removePlane(index);

    // CURRENT
    bindOffscreenBuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen
    //glEnable(GL_TEXTURE_2D);
    //glEnable(GL_DEPTH_TEST);
    //heightmap->renderTopview(renderContext, width, height);
    //NanoVis::display();
    if (HeightMap::updatePending) {
        setHeightmapRanges();
    }

    //cam->initialize();

    heightmap->renderTopview(renderContext, width, height);

    readScreen();

    // INSOO TEST CODE
    bmpWriteToFile(1, "/tmp");

    resizeOffscreenBuffer(old_width, old_height);

    return TCL_OK;
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
