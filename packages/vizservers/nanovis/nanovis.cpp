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
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <sys/time.h>
#include <cassert>
#include <cstdio>

#include <GL/glew.h>
#include <GL/glut.h>

#include <graphics/RenderContext.h>
#include <vrmath/Vector3f.h>

#include <util/FilePath.h>
#include <util/Fonts.h>

#include <BMPImageLoaderImpl.h>
#include <ImageLoaderFactory.h>

#include "config.h"
#include "nanovis.h"
#include "nanovisServer.h"
#include "define.h"

#include "Flow.h"
#include "Grid.h"
#include "HeightMap.h"
#include "Camera.h"
#include "LIC.h"
#include "NvShader.h"
#include "OrientationIndicator.h"
#include "PlaneRenderer.h"
#include "PPMWriter.h"
#include "Texture2D.h"
#include "Trace.h"
#include "TransferFunction.h"
#include "Unirect.h"
#include "VelocityArrowsSlice.h"
#include "Volume.h"
#include "VolumeInterpolator.h"
#include "VolumeRenderer.h"

using namespace nv;
using namespace nv::graphics;
using namespace nv::util;
using namespace vrmath;

// STATIC MEMBER DATA

Tcl_Interp *NanoVis::interp = NULL;

unsigned int NanoVis::flags = 0;
bool NanoVis::debugFlag = false;

int NanoVis::winWidth = NPIX;
int NanoVis::winHeight = NPIX;
int NanoVis::renderWindow = 0;
unsigned char *NanoVis::screenBuffer = NULL;
Texture2D *NanoVis::legendTexture = NULL;
Fonts *NanoVis::fonts;
int NanoVis::updir = Y_POS;
Camera *NanoVis::cam = NULL;
RenderContext *NanoVis::renderContext = NULL;

NanoVis::TransferFunctionHashmap NanoVis::tfTable;
NanoVis::VolumeHashmap NanoVis::volumeTable;
NanoVis::FlowHashmap NanoVis::flowTable;
NanoVis::HeightMapHashmap NanoVis::heightMapTable;

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
Vector3f NanoVis::sceneMin, NanoVis::sceneMax;

VolumeRenderer *NanoVis::volRenderer = NULL;
VelocityArrowsSlice *NanoVis::velocityArrowsSlice = NULL;
LIC *NanoVis::licRenderer = NULL;
PlaneRenderer *NanoVis::planeRenderer = NULL;
OrientationIndicator *NanoVis::orientationIndicator = NULL;
Grid *NanoVis::grid = NULL;

// Image based flow visualization slice location
// FLOW
float NanoVis::_licSlice = 0.5f;
int NanoVis::_licAxis = 2; // z axis

//frame buffer for final rendering
GLuint NanoVis::_finalFbo = 0;
GLuint NanoVis::_finalColorTex = 0;
GLuint NanoVis::_finalDepthRb = 0;

// Default camera location.
float def_eye_x = 0.0f;
float def_eye_y = 0.0f;
float def_eye_z = 2.5f;

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
    if (legendTexture != NULL) {
        TRACE("Deleting legendTexture");
        delete legendTexture;
    }
    TRACE("Deleting flows");
    deleteFlows(interp);
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
        screenBuffer = NULL;
    }
    if (fonts != NULL) {
        TRACE("Deleting fonts");
        delete fonts;
    }
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

    collectBounds();
    cam->resetClippingRange(sceneMin, sceneMax);

    TRACE("set cam z to %f", cam->z());
}

void
NanoVis::resetCamera(bool resetOrientation)
{
    TRACE("Resetting all=%d", resetOrientation ? 1 : 0);

    collectBounds();
    cam->reset(sceneMin, sceneMax, resetOrientation);

    def_eye_x = cam->x();
    def_eye_y = cam->y();
    def_eye_z = cam->z();
}

/** \brief Load a 3D volume
 *
 * \param name Volume ID
 * \param width Number of samples in X direction
 * \param height Number of samples in Y direction
 * \param depth Number of samples in Z direction
 * \param numComponents the number of scalars for each space point. All component 
 * scalars for a point are placed consequtively in data array
 * width, height and depth: number of points in each dimension
 * \param data Array of floats
 * \param vmin Min value of field
 * \param vmax Max value of field
 * \param nonZeroMin Minimum non-zero value of field
 * \param data pointer to an array of floats.
 */
Volume *
NanoVis::loadVolume(const char *name, int width, int height, int depth, 
                    int numComponents, float *data, double vmin, double vmax, 
                    double nonZeroMin)
{
    VolumeHashmap::iterator itr = volumeTable.find(name);
    if (itr != volumeTable.end()) {
        WARN("volume \"%s\" already exists", name);
        removeVolume(itr->second);
    }

    Volume *volume = new Volume(0.f, 0.f, 0.f,
                                width, height, depth,
                                numComponents,
                                data, vmin, vmax, nonZeroMin);
    Volume::updatePending = true;
    volume->name(name);
    volumeTable[name] = volume;

    return volume;
}

// Gets a colormap 1D texture by name.
TransferFunction *
NanoVis::getTransferFunction(const TransferFunctionId& id) 
{
    TransferFunctionHashmap::iterator itr = tfTable.find(id);
    if (itr == tfTable.end()) {
        TRACE("No transfer function named \"%s\" found", id.c_str());
        return NULL;
    } else {
        return itr->second;
    }
}

// Creates of updates a colormap 1D texture by name.
TransferFunction *
NanoVis::defineTransferFunction(const TransferFunctionId& id,
                                size_t n, float *data)
{
    TransferFunction *tf = getTransferFunction(id);
    if (tf == NULL) {
        TRACE("Creating new transfer function \"%s\"", id.c_str());
        tf = new TransferFunction(id.c_str(), n, data);
        tfTable[id] = tf;
    } else {
        TRACE("Updating existing transfer function \"%s\"", id.c_str());
        tf->update(n, data);
    }
    return tf;
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

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screenBuffer);
    {
        char prefix[200];

        TRACE("Sending ppm legend image %s min:%g max:%g", volArg, min, max);
        sprintf(prefix, "nv>legend %s %g %g", volArg, min, max);
#ifdef USE_THREADS
        queuePPM(g_queue, prefix, screenBuffer, width, height);
#else
        writePPM(g_fdOut, prefix, screenBuffer, width, height);
#endif
    }
    planeRenderer->removePlane(index);
    resizeOffscreenBuffer(old_width, old_height);

    delete legendTexture;
    legendTexture = NULL;
    TRACE("Leave");
    return TCL_OK;
}

//initialize frame buffer objects for offscreen rendering
bool
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
        return false;
    }

    TRACE("Leave");
    return true;
}

//resize the offscreen buffer 
bool
NanoVis::resizeOffscreenBuffer(int w, int h)
{
    TRACE("Enter (%d, %d)", w, h);
    if ((w == winWidth) && (h == winHeight)) {
        return true;
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

    screenBuffer = new unsigned char[3*winWidth*winHeight];
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
        return false;
    }

    TRACE("change camera");
    //change the camera setting
    cam->setScreenSize(0, 0, winWidth, winHeight);
    planeRenderer->setScreenSize(winWidth, winHeight);

    TRACE("Leave (%d, %d)", w, h);
    return true;
}

static
void cgErrorCallback(void)
{
    if (!NvShader::printErrorInfo()) {
        TRACE("Cg error, exiting...");
        exit(1);
    }
}

bool NanoVis::init(const char* path)
{
    // print OpenGL driver information
    TRACE("-----------------------------------------------------------");
    TRACE("OpenGL version: %s", glGetString(GL_VERSION));
    TRACE("OpenGL vendor: %s", glGetString(GL_VENDOR));
    TRACE("OpenGL renderer: %s", glGetString(GL_RENDERER));
    TRACE("-----------------------------------------------------------");

    if (path == NULL) {
        ERROR("No path defined for shaders or resources");
        return false;
    }
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        ERROR("Can't init GLEW: %s", glewGetErrorString(err));
        return false;
    }
    TRACE("Using GLEW %s", glewGetString(GLEW_VERSION));

    // OpenGL 2.1 includes VBOs, PBOs, MRT, NPOT textures, point parameters, point sprites,
    // GLSL 1.2, and occlusion queries.
    if (!GLEW_VERSION_2_1) {
        ERROR("OpenGL version 2.1 or greater is required");
        return false;
    }

    // NVIDIA driver may report OpenGL 2.1, but not support PBOs in 
    // indirect GLX contexts
    if (!GLEW_ARB_pixel_buffer_object) {
        ERROR("Pixel buffer objects are not supported by driver, please check that the user running nanovis has permissions to create a direct rendering context (e.g. user has read/write access to /dev/nvidia* device nodes in Linux).");
        return false;
    }

    // Additional extensions not in 2.1 core

    // Framebuffer objects were promoted in 3.0
    if (!GLEW_EXT_framebuffer_object) {
        ERROR("EXT_framebuffer_oject extension is required");
        return false;
    }
    // Rectangle textures were promoted in 3.1
    // FIXME: can use NPOT in place of rectangle textures
    if (!GLEW_ARB_texture_rectangle) {
        ERROR("ARB_texture_rectangle extension is required");
        return false;
    }
#ifdef HAVE_FLOAT_TEXTURES
    // Float color buffers and textures were promoted in 3.0
    if (!GLEW_ARB_texture_float ||
        !GLEW_ARB_color_buffer_float) {
        ERROR("ARB_texture_float and ARB_color_buffer_float extensions are required");
        return false;
    }
#endif
    // FIXME: should use ARB programs or (preferably) a GLSL profile for portability
    if (!GLEW_NV_vertex_program3 ||
        !GLEW_NV_fragment_program2) {
        ERROR("NV_vertex_program3 and NV_fragment_program2 extensions are required");
        return false;
    }

    if (!FilePath::getInstance()->setPath(path)) {
        ERROR("can't set file path to %s", path);
        return false;
    }

    ImageLoaderFactory::getInstance()->addLoaderImpl("bmp", new BMPImageLoaderImpl());

    NvShader::initCg();
    NvShader::setErrorCallback(cgErrorCallback);

    fonts = new Fonts();
    fonts->addFont("verdana", "verdana.fnt");
    fonts->setFont("verdana");

    velocityArrowsSlice = new VelocityArrowsSlice;
    licRenderer = new LIC(NMESH, NPIX, NPIX, _licAxis, _licSlice);

    grid = new Grid();
    grid->setFont(fonts);
    return true;
}

bool
NanoVis::initGL()
{
    TRACE("in initGL");
    //buffer to store data read from the screen
    if (screenBuffer) {
        delete[] screenBuffer;
        screenBuffer = NULL;
    }
    screenBuffer = new unsigned char[3*winWidth*winHeight];
    assert(screenBuffer != NULL);

    //create the camera with default setting
    cam = new Camera(0, 0, winWidth, winHeight,
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

    //frame buffer object for offscreen rendering
    if (!initOffscreenBuffer()) {
        return false;
    }

    volRenderer = new VolumeRenderer();

    renderContext = new RenderContext();

    planeRenderer = new PlaneRenderer(winWidth, winHeight);

    orientationIndicator = new OrientationIndicator();

    //assert(glGetError()==0);

    TRACE("leaving initGL");
    return true;
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
    VolumeHashmap::iterator itr;
    for (itr = volumeTable.begin();
         itr != volumeTable.end(); ++itr) {
        Volume *volume = itr->second;
        if (xMin > volume->xAxis.min()) {
            xMin = volume->xAxis.min();
        }
        if (xMax < volume->xAxis.max()) {
            xMax = volume->xAxis.max();
        }
        if (yMin > volume->yAxis.min()) {
            yMin = volume->yAxis.min();
        }
        if (yMax < volume->yAxis.max()) {
            yMax = volume->yAxis.max();
        }
        if (zMin > volume->zAxis.min()) {
            zMin = volume->zAxis.min();
        }
        if (zMax < volume->zAxis.max()) {
            zMax = volume->zAxis.max();
        }
        if (wMin > volume->wAxis.min()) {
            wMin = volume->wAxis.min();
        }
        if (wMax < volume->wAxis.max()) {
            wMax = volume->wAxis.max();
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
    HeightMapHashmap::iterator itr;
    for (itr = heightMapTable.begin();
         itr != heightMapTable.end(); ++itr) {
        HeightMap *heightMap = itr->second;
        if (xMin > heightMap->xAxis.min()) {
            xMin = heightMap->xAxis.min();
        }
        if (xMax < heightMap->xAxis.max()) {
            xMax = heightMap->xAxis.max();
        }
        if (yMin > heightMap->yAxis.min()) {
            yMin = heightMap->yAxis.min();
        }
        if (yMax < heightMap->yAxis.max()) {
            yMax = heightMap->yAxis.max();
        }
        if (zMin > heightMap->zAxis.min()) {
            zMin = heightMap->zAxis.min();
        }
        if (zMax < heightMap->zAxis.max()) {
            zMax = heightMap->zAxis.max();
        }
        if (wMin > heightMap->wAxis.min()) {
            wMin = heightMap->wAxis.min();
        }
        if (wMax < heightMap->wAxis.max()) {
            wMax = heightMap->wAxis.max();
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
    for (HeightMapHashmap::iterator itr = heightMapTable.begin();
         itr != heightMapTable.end(); ++itr) {
        itr->second->mapToGrid(grid);
    }
    HeightMap::updatePending = false;
    TRACE("Leave");
}

void
NanoVis::collectBounds(bool onlyVisible)
{
    if (flags & MAP_FLOWS) {
        mapFlows();
        grid->xAxis.setScale(xMin, xMax);
        grid->yAxis.setScale(yMin, yMax);
        grid->zAxis.setScale(zMin, zMax);
    }

    sceneMin.set(FLT_MAX, FLT_MAX, FLT_MAX);
    sceneMax.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (VolumeHashmap::iterator itr = volumeTable.begin();
         itr != volumeTable.end(); ++itr) {
        Volume *volume = itr->second;

        if (onlyVisible && !volume->visible())
            continue;

        Vector3f bmin, bmax;
        volume->getWorldSpaceBounds(bmin, bmax);
        if (bmin.x > bmax.x)
            continue;

        if (sceneMin.x > bmin.x) {
            sceneMin.x = bmin.x;
        }
        if (sceneMax.x < bmax.x) {
            sceneMax.x = bmax.x;
        }
        if (sceneMin.y > bmin.y) {
            sceneMin.y = bmin.y;
        }
        if (sceneMax.y < bmax.y) {
            sceneMax.y = bmax.y;
        }
        if (sceneMin.z > bmin.z) {
            sceneMin.z = bmin.z;
        }
        if (sceneMax.z < bmax.z) {
            sceneMax.z = bmax.z;
        }
    }

    for (HeightMapHashmap::iterator itr = heightMapTable.begin();
         itr != heightMapTable.end(); ++itr) {
        HeightMap *heightMap = itr->second;

        if (onlyVisible && !heightMap->isVisible())
            continue;

        Vector3f bmin, bmax;
        heightMap->getWorldSpaceBounds(bmin, bmax);
        if (bmin.x > bmax.x)
            continue;

        if (sceneMin.x > bmin.x) {
            sceneMin.x = bmin.x;
        }
        if (sceneMax.x < bmax.x) {
            sceneMax.x = bmax.x;
        }
        if (sceneMin.y > bmin.y) {
            sceneMin.y = bmin.y;
        }
        if (sceneMax.y < bmax.y) {
            sceneMax.y = bmax.y;
        }
        if (sceneMin.z > bmin.z) {
            sceneMin.z = bmin.z;
        }
        if (sceneMax.z < bmax.z) {
            sceneMax.z = bmax.z;
        }
    }

    Vector3f flowMin, flowMax;
    getFlowBounds(flowMin, flowMax, onlyVisible);
    if (flowMin.x < flowMax.x) {
        if (sceneMin.x > flowMin.x) {
            sceneMin.x = flowMin.x;
        }
        if (sceneMax.x < flowMax.x) {
            sceneMax.x = flowMax.x;
        }
        if (sceneMin.y > flowMin.y) {
            sceneMin.y = flowMin.y;
        }
        if (sceneMax.y < flowMax.y) {
            sceneMax.y = flowMax.y;
        }
        if (sceneMin.z > flowMin.z) {
            sceneMin.z = flowMin.z;
        }
        if (sceneMax.z < flowMax.z) {
            sceneMax.z = flowMax.z;
        }
    }

    // TODO: Get Grid bounds

    if (sceneMin.x > sceneMax.x) {
        sceneMin.set(-0.5, -0.5, -0.5);
        sceneMax.set( 0.5,  0.5,  0.5);
    }

    TRACE("Scene bounds: (%g,%g,%g) - (%g,%g,%g)",
          sceneMin.x, sceneMin.y, sceneMin.z,
          sceneMax.x, sceneMax.y, sceneMax.z);
}

void
NanoVis::setBgColor(float color[3])
{
    TRACE("Setting bgcolor to %g %g %g", color[0], color[1], color[2]);
    glClearColor(color[0], color[1], color[2], 1);
}

void 
NanoVis::removeVolume(Volume *volume)
{
    VolumeHashmap::iterator itr = volumeTable.find(volume->name());
    if (itr != volumeTable.end()) {
        volumeTable.erase(itr);
    }
    delete volume;
}

Flow *
NanoVis::getFlow(const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr == flowTable.end()) {
        TRACE("Can't find flow '%s'", name);
        return NULL;
    }
    return itr->second;
}

Flow *
NanoVis::createFlow(Tcl_Interp *interp, const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr != flowTable.end()) {
        ERROR("Flow '%s' already exists", name);
        return NULL;
    }
    Flow *flow = new Flow(interp, name);
    flowTable[name] = flow;
    return flow;
}

/**
 * \brief Delete flow object and hash table entry
 *
 * This is called by the flow command instance delete callback
 */
void
NanoVis::deleteFlow(const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr != flowTable.end()) {
        delete itr->second;
        flowTable.erase(itr);
    }
}

/**
 * \brief Delete all flow object commands
 *
 * This will also delete the flow objects and hash table entries
 */
void
NanoVis::deleteFlows(Tcl_Interp *interp)
{
    FlowHashmap::iterator itr;
    for (itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Tcl_DeleteCommandFromToken(interp, itr->second->getCommandToken());
    }
    flowTable.clear();
}

bool
NanoVis::mapFlows()
{
    TRACE("Enter");

    flags &= ~MAP_FLOWS;

    /* 
     * Step 1. Get the overall min and max magnitudes of all the 
     *         flow vectors.
     */
    magMin = DBL_MAX, magMax = -DBL_MAX;

    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        double min, max;
        if (!flow->isDataLoaded()) {
            continue;
        }
        Rappture::Unirect3d *data = flow->data();
        min = data->magMin();
        max = data->magMax();
        if (min < magMin) {
            magMin = min;
        } 
        if (max > magMax) {
            magMax = max;
        }
        if (data->xMin() < xMin) {
            xMin = data->xMin();
        }
        if (data->yMin() < yMin) {
            yMin = data->yMin();
        }
        if (data->zMin() < zMin) {
            zMin = data->zMin();
        }
        if (data->xMax() > xMax) {
            xMax = data->xMax();
        }
        if (data->yMax() > yMax) {
            yMax = data->yMax();
        }
        if (data->zMax() > zMax) {
            zMax = data->zMax();
        }
    }

    TRACE("magMin=%g magMax=%g", NanoVis::magMin, NanoVis::magMax);

    /* 
     * Step 2. Generate the vector field from each data set. 
     */
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        if (!flow->isDataLoaded()) {
            continue; // Flow exists, but no data has been loaded yet.
        }
        if (flow->visible()) {
            flow->initializeParticles();
        }
        if (!flow->scaleVectorField()) {
            return false;
        }
        // FIXME: This doesn't work when there is more than one flow.
        licRenderer->setOffset(flow->getRelativePosition());
        velocityArrowsSlice->slicePos(flow->getRelativePosition());
    }
    advectFlows();
    return true;
}

void
NanoVis::getFlowBounds(Vector3f& min,
                       Vector3f& max,
                       bool onlyVisible)
{
    TRACE("Enter");

    min.set(FLT_MAX, FLT_MAX, FLT_MAX);
    max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        itr->second->getBounds(min, max, onlyVisible);
    }
}

void
NanoVis::renderFlows()
{
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->render();
        }
    }
    flags &= ~REDRAW_PENDING;
}

void
NanoVis::resetFlows()
{
    if (licRenderer->active()) {
        NanoVis::licRenderer->reset();
    }
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->resetParticles();
        }
    }
}    

void
NanoVis::advectFlows()
{
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->advect();
        }
    }
}

void
NanoVis::render()
{
    TRACE("Enter");

    if (flags & MAP_FLOWS) {
#ifdef notdef
        xMin = yMin = zMin = wMin = FLT_MAX, magMin = DBL_MAX;
        xMax = yMax = zMax = wMax = -FLT_MAX, magMax = -DBL_MAX;
#endif
        mapFlows();
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

    //clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    orientationIndicator->render();
    grid->render();
    if ((licRenderer != NULL) && (licRenderer->active())) {
        licRenderer->render();
    }
    if ((velocityArrowsSlice != NULL) && (velocityArrowsSlice->enabled())) {
        velocityArrowsSlice->render();
    }
    renderFlows();
    volRenderer->renderAll();

    HeightMapHashmap::iterator itr;
    for (itr = heightMapTable.begin();
         itr != heightMapTable.end(); ++itr) {
        HeightMap *heightMap = itr->second;
        if (heightMap->isVisible()) {
            heightMap->render(renderContext);
        }
    }
    glPopMatrix();

    CHECK_FRAMEBUFFER_STATUS();
    TRACE("Leave");
}
