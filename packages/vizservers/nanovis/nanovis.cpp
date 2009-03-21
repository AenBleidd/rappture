
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
#include <fcntl.h>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <math.h>
#include <memory.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "Nv.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "Util.h"
#include <NvLIC.h>
#include <Trace.h>

#include "nanovis.h"
#include "define.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"
#include "RpEncode.h"

#ifdef notdef
//transfer function headers
#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"
#endif
#include "ZincBlendeVolume.h"
#include "NvLoadFile.h"
#include "NvColorTableRenderer.h"
#include "NvEventLog.h"
#include "NvZincBlendeReconstructor.h"
#include "HeightMap.h"
#include "Grid.h"
#include "VolumeInterpolator.h"
#include <RenderContext.h>

#include <BMPImageLoaderImpl.h>
#include <ImageLoaderFactory.h>

//#define  _LOCAL_ZINC_TEST_

// R2 headers
#include <R2/R2FilePath.h>
#include <R2/R2Fonts.h>

#define SIZEOF_BMP_HEADER   54

extern void NvInitCG(); // in Shader.cpp

// Indicates "up" axis:  x=1, y=2, z=3, -x=-1, -y=-2, -z=-3
enum AxisDirections { 
    X_POS = 1, Y_POS = 2, Z_POS = 3, X_NEG = -1, Y_NEG = -2, Z_NEG = -3
};

#define KEEPSTATS	1

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
NvCamera* NanoVis::cam = NULL;
int NanoVis::n_volumes = 0;
vector<Volume*> NanoVis::volume;
vector<HeightMap*> NanoVis::heightMap;
VolumeRenderer* NanoVis::vol_renderer = 0;
PointSetRenderer* NanoVis::pointset_renderer = 0;
vector<PointSet*> NanoVis::pointSet;
PlaneRenderer* NanoVis::plane_render = 0;
Texture2D* NanoVis::plane[10];
NvColorTableRenderer* NanoVis::color_table_renderer = 0;
NvParticleRenderer* NanoVis::particleRenderer = 0;
graphics::RenderContext* NanoVis::renderContext = 0;
NvLIC* NanoVis::licRenderer = 0;
R2Fonts* NanoVis::fonts;

FILE *NanoVis::stdin = NULL;
FILE *NanoVis::logfile = NULL;
FILE *NanoVis::recfile = NULL;

bool NanoVis::lic_on = false;
bool NanoVis::particle_on = false;
bool NanoVis::vector_on = false;
bool NanoVis::axis_on = true;
bool NanoVis::config_pending = false;
bool NanoVis::debug_flag = false;
bool NanoVis::lic_slice_x_visible = false;
bool NanoVis::lic_slice_y_visible = false;
bool NanoVis::lic_slice_z_visible = false;

Tcl_Interp *NanoVis::interp;
Tcl_DString NanoVis::cmdbuffer;

//frame buffer for final rendering
NVISid NanoVis::final_color_tex = 0;
NVISid NanoVis::final_depth_rb = 0;
NVISid NanoVis::final_fbo = 0;
int NanoVis::render_window = 0;       /* GLUT handle for the render window */
int NanoVis::win_width = NPIX;        /* Width of the render window */
int NanoVis::win_height = NPIX;       /* Height of the render window */

unsigned char* NanoVis::screen_buffer = NULL;

/* FIXME: This variable is always true. */
bool volume_mode = true; 

// in Command.cpp
extern Tcl_Interp *initTcl();

// in dxReader.cpp
extern void load_vector_stream(int index, std::istream& fin);

float vert[NMESH*NMESH*3];              //particle positions in main memory
float slice_vector[NMESH*NMESH*4];      //per slice vectors in main memory

// maps transfunc name to TransferFunction object
static Tcl_HashTable tftable;

// pointers to 2D planes, currently handle up 10


PerfQuery* perf;                        //perfromance counter

CGprogram m_passthru_fprog;
CGparameter m_passthru_scale_param, m_passthru_bias_param;

// Variables for mouse events

// Default camera rotation angles.
const float def_rot_x = 90.0f;
const float def_rot_y = 180.0f;
const float def_rot_z = -135.0f;

// Default camera target.
const float def_target_x = 0.0f;
const float def_target_y = 0.0f;
const float def_target_z = 100.0f; 

// Default camera location.
const float def_eye_x = -0.0f;
const float def_eye_y = -0.0f;
const float def_eye_z = -2.5f;


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
float NanoVis::lic_slice_x = 0.5f;
float NanoVis::lic_slice_y = 0.5f;
float NanoVis::lic_slice_z = 0.5f;
int NanoVis::lic_axis = 2; // z axis

using namespace std;

#define RM_VOLUME 1
#define RM_POINT 2

int renderMode = RM_VOLUME;

/*
 * ----------------------------------------------------------------------
 * USAGE: debug("string", ...)
 *
 * Use this anywhere within the package to send a debug message
 * back to the client.  The string can have % fields as used by
 * the printf() package.  Any remaining arguments are treated as
 * field substitutions on that.
 * ----------------------------------------------------------------------
 */
void
debug(char *str)
{
    ssize_t nWritten;

    nWritten = write(0, str, strlen(str));
}

void
debug(char *str, double v1)
{
    char buffer[512];
    sprintf(buffer, str, v1);
    debug(buffer);
}

void
debug(char *str, double v1, double v2)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2);
    debug(buffer);
}

void
debug(char *str, double v1, double v2, double v3)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2, v3);
    debug(buffer);
}

void removeAllData()
{
    //
}


#if KEEPSTATS

static int
WriteStats(const char *who, int code) 
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
DoExit(int code)
{
    if (NanoVis::debug_flag) {
        fprintf(stderr, "in DoExit\n");
    }
    removeAllData();
    NvExit();
#if KEEPSTATS
    WriteStats("nanovis", code);
#endif
    exit(code);
}

//report errors related to CG shaders
void
cgErrorCallback(void)
{
    CGerror lastError = cgGetError();
    if(lastError) {
        const char *listing = cgGetLastListing(g_context);
        Trace("\n---------------------------------------------------\n");
        Trace("%s\n\n", cgGetErrorString(lastError));
        Trace("%s\n", listing);
        Trace("-----------------------------------------------------\n");
        Trace("Cg error, exiting...\n");
        cgDestroyContext(g_context);
        DoExit(-1);
    }
}


CGprogram 
LoadCgSourceProgram(CGcontext context, const char *fileName, CGprofile profile, 
                    const char *entryPoint)
{
    const char *path = R2FilePath::getInstance()->getPath(fileName);
    if (path == NULL) {
        fprintf(stderr, "can't find program \"%s\"\n", fileName);
        assert(path != NULL);
    }
    printf("cg program compiling: %s\n", path);
    fflush(stdout);
    CGprogram program;
    program = cgCreateProgramFromFile(context, CG_SOURCE, path, profile, 
				      entryPoint, NULL);
    cgGLLoadProgram(program);

    CGerror LastError = cgGetError();
    if (LastError)
	{
	    printf("Error message: %s\n", cgGetLastListing(context));
	}

    delete [] path;

    return program;
}

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    struct timeval tv;
    double start, finish;
    int result;

    if (NanoVis::debug_flag) {
        fprintf(stderr, "in ExecuteCommand(%s)\n", Tcl_DStringValue(dsPtr));
    }

    gettimeofday(&tv, NULL);
    start = CVT2SECS(tv);

    if (NanoVis::logfile != NULL) {
        fprintf(NanoVis::logfile, "%s", Tcl_DStringValue(dsPtr));
        fflush(NanoVis::logfile);
    }
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
    if (NanoVis::debug_flag) {
        fprintf(stderr, "leaving ExecuteCommand status=%d\n", result);
    }
    return result;
}

void
NanoVis::pan(float dx, float dy)
{
    /* Move the camera and its target by equal amounts along the x and y
     * axes. */
    fprintf(stderr, "x=%f, y=%f\n", dx, dy);

    cam->x(def_eye_x + dx);
    cam->y(def_eye_y + dy);
    fprintf(stderr, "set eye to %f %f\n", cam->x(), cam->y());

    cam->xAim(def_target_x + dx);
    cam->yAim(def_target_y + dy);
    fprintf(stderr, "set aim to %f %f\n", cam->xAim(), cam->yAim());
}


/* Load a 3D volume
 * index:	the index into the volume array, which stores pointers 
 *		to 3D volume instances
 * data:	pointer to an array of floats.
 * n_component: the number of scalars for each space point. All component 
 *		scalars for a point are placed consequtively in data array
 *		width, height and depth: number of points in each dimension
 */
Volume *
NanoVis::load_volume(int index, int width, int height, int depth,
                     int n_component, float* data, double vmin,
                     double vmax, double nzero_min)
{
    while (n_volumes <= index) {
        volume.push_back(NULL);
        n_volumes++;
    }

    Volume* vol = volume[index];
    if (vol != NULL) {
        volume[index] = NULL;

        if (vol->pointsetIndex != -1) {
            if (((unsigned  int) vol->pointsetIndex) < pointSet.size() &&
		pointSet[vol->pointsetIndex] != NULL) {
                delete pointSet[vol->pointsetIndex];
                pointSet[vol->pointsetIndex] = 0;
            }
        }
        delete vol;
    }
    volume[index] = new Volume(0.f, 0.f, 0.f, width, height, depth, 1.,
			       n_component, data, vmin, vmax, nzero_min);
    return volume[index];
}

// Gets a colormap 1D texture by name.
TransferFunction*
NanoVis::get_transfunc(const char *name) 
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&tftable, name);
    if (hPtr == NULL) {
        return NULL;
    }
    return (TransferFunction*)Tcl_GetHashValue(hPtr);
}

// Creates of updates a colormap 1D texture by name.
TransferFunction*
NanoVis::DefineTransferFunction(const char *name, size_t n, float *data)
{
    int isNew;
    Tcl_HashEntry *hPtr;
    TransferFunction *tf;

    hPtr = Tcl_CreateHashEntry(&tftable, name, &isNew);
    if (isNew) {
        tf = new TransferFunction(n, data);
        Tcl_SetHashValue(hPtr, (ClientData)tf);
    } else {
        /* 
         * You can't delete the transfer function because many 
         * objects may be holding its pointer.  We must update it.
         */
        tf = (TransferFunction *)Tcl_GetHashValue(hPtr);
        tf->update(n, data);
    }
    return tf;
}

int
NanoVis::render_legend(TransferFunction *tf, double min, double max,
		       int width, int height, const char* volArg)
{
    if (debug_flag) {
    	fprintf(stderr, "in render_legend\n");
    }
    int old_width = win_width;
    int old_height = win_height;

    plane_render->set_screen_size(width, height);
    resize_offscreen_buffer(width, height);

    // generate data for the legend
    float data[512];
    for (int i=0; i < 256; i++) {
        data[i] = data[i+256] = (float)(i/255.0);
    }
    plane[0] = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
    int index = plane_render->add_plane(plane[0], tf);
    plane_render->set_active_plane(index);

    offscreen_buffer_capture();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen
    plane_render->render();

    // INSOO
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);
    //glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, screen_buffer); // INSOO's

    if (debug_flag) {
        fprintf(stderr, "ppm legend image not written (debug mode)\n");
    } else {
        char prefix[200];
	ssize_t nWritten;

        sprintf(prefix, "nv>legend %s %g %g", volArg, min, max);
        ppm_write(prefix);
        nWritten = write(0, "\n", 1);
	assert(nWritten == 1);
    }
    plane_render->remove_plane(index);
    resize_offscreen_buffer(old_width, old_height);

    if (debug_flag) {
    	fprintf(stderr, "leaving render_legend\n");
    }
    return TCL_OK;
}

//initialize frame buffer objects for offscreen rendering
void
NanoVis::init_offscreen_buffer()
{
    if (debug_flag) {
        fprintf(stderr, "in init_offscreen_buffer\n");
    }
    // Initialize a fbo for final display.
    glGenFramebuffersEXT(1, &final_fbo);
    
    glGenTextures(1, &final_color_tex);
    glBindTexture(GL_TEXTURE_2D, final_color_tex);
    
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef NV40
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, win_width, win_height, 0,
                 GL_RGB, GL_INT, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_width, win_height, 0,
                 GL_RGB, GL_INT, NULL);
#endif
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    glGenRenderbuffersEXT(1, &final_depth_rb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 
			     win_width, win_height);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			      GL_TEXTURE_2D, final_color_tex, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				 GL_RENDERBUFFER_EXT, final_depth_rb);

    GLenum status;
    if (!CheckFBO(&status)) {
        PrintFBOStatus(status, "final_fbo");
    	DoExit(3);
    }

    // Check framebuffer completeness at the end of initialization.
    //CHECK_FRAMEBUFFER_STATUS();
    
    //assert(glGetError()==0);
    if (debug_flag) {
        fprintf(stderr, "leaving init_offscreen_buffer\n");
    }
}


//resize the offscreen buffer 
void 
NanoVis::resize_offscreen_buffer(int w, int h)
{
    if ((w == win_width) && (h == win_height)) {
        return;
    }
    if (debug_flag) {
        fprintf(stderr, "in resize_offscreen_buffer(%d, %d)\n", w, h);
    }

    win_width = w;
    win_height = h;
    
    if (fonts) {
        fonts->resize(w, h);
    }
    //fprintf(stderr, "screen_buffer size: %d\n", sizeof(screen_buffer));
    if (debug_flag) {
	fprintf(stderr, "screen_buffer size: %d %d\n", w, h);
    }
    
    if (screen_buffer) {
        delete[] screen_buffer;
        screen_buffer = NULL;
    }
    
    screen_buffer = new unsigned char[4*win_width*win_height];
    assert(screen_buffer != NULL);
    
    //delete the current render buffer resources
    glDeleteTextures(1, &final_color_tex);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glDeleteRenderbuffersEXT(1, &final_depth_rb);

    if (debug_flag) {
	fprintf(stderr, "before deleteframebuffers\n");
    }
    glDeleteFramebuffersEXT(1, &final_fbo);

    if (debug_flag) {
        fprintf(stderr, "reinitialize FBO\n");
    }
    //Reinitialize final fbo for final display
    glGenFramebuffersEXT(1, &final_fbo);

    glGenTextures(1, &final_color_tex);
    glBindTexture(GL_TEXTURE_2D, final_color_tex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef NV40
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, win_width, win_height, 0,
                 GL_RGB, GL_INT, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_width, win_height, 0,
                 GL_RGB, GL_INT, NULL);
#endif
    if (debug_flag) {
        fprintf(stderr, "before bindframebuffer\n");
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    if (debug_flag) {
        fprintf(stderr, "after bindframebuffer\n");
    }
    glGenRenderbuffersEXT(1, &final_depth_rb);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, 
			     win_width, win_height);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			      GL_TEXTURE_2D, final_color_tex, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				 GL_RENDERBUFFER_EXT, final_depth_rb);
    
    GLenum status;
    if (!CheckFBO(&status)) {
        PrintFBOStatus(status, "final_fbo");
	DoExit(3);
    }

    //CHECK_FRAMEBUFFER_STATUS();
    if (debug_flag) {
	fprintf(stderr, "change camera\n");
    }
    //change the camera setting
    cam->set_screen_size(0, 0, win_width, win_height);
    plane_render->set_screen_size(win_width, win_height);

    if (debug_flag) {
        fprintf(stderr, "leaving resize_offscreen_buffer(%d, %d)\n", w, h);
    }
}

/*
 * FIXME: This routine is fairly expensive (60000 floating pt divides).
 *      I've put an ifdef around the call to it so that the released
 *      builds don't include it.  Define PROTOTYPE to 1 in config.h
 *      to turn it back on.
 */
void 
make_test_2D_data()
{
    int w = 300;
    int h = 200;
    float* data = new float[w*h];

    //procedurally make a gradient plane
    for(int j=0; j<h; j++){
        for(int i=0; i<w; i++){
            data[w*j+i] = float(i)/float(w);
        }
    }
    NanoVis::plane[0] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, data);
    delete[] data;
}

void NanoVis::initParticle()
{
    //random placement on a slice
    size_t n = particleRenderer->psys_width * particleRenderer->psys_height * 4;
    float* data = new float[n];
    memset(data, 0, sizeof(float)* n);

    int index;
    //bool particle;
    for (int i=0; i<particleRenderer->psys_width; i++) {
        for (int j=0; j<particleRenderer->psys_height; j++) {
            index = i + particleRenderer->psys_height*j;
            //particle = rand() % 256 > 150; 
            //if(particle) 
            {
                //assign any location (x,y,z) in range [0,1]
                // TEMP
                //data[4*index] = lic_slice_x;
                if (j % 2) data[4*index] = 0.95;
		else data[4*index] = 0.05;
                data[4*index+1]= j/float(particleRenderer->psys_height);
                data[4*index+2]= i/float(particleRenderer->psys_width);
                //data[4*index+3]= 30; //shorter life span, quicker iterations
                data[4*index+3]= rand() / ((float) RAND_MAX) * 0.5  + 0.5f; //shorter life span, quicker iterations
            }
	    /*
	      else
	      {
	      data[4*index] = 0;
	      data[4*index+1]= 0;
	      data[4*index+2]= 0;
	      data[4*index+3]= 0;
	      }
	    */
        }
    }

    particleRenderer->initialize((Particle*)data);
    licRenderer->make_patterns();

    delete[] data;
}

void CgErrorCallback(void)
{
    CGerror lastError = cgGetError();

    if(lastError) {
        const char *listing = cgGetLastListing(g_context);
        printf("\n---------------------------------------------------\n");
        printf("%s\n\n", cgGetErrorString(lastError));
        printf("%s\n", listing);
        printf("-----------------------------------------------------\n");
        printf("Cg error, exiting...\n");
        cgDestroyContext(g_context);
        fflush(stdout);
        DoExit(-1);
    }
}

void NanoVis::init(const char* path)
{
    // print system information
    fprintf(stderr, 
            "-----------------------------------------------------------\n");
    fprintf(stderr, "OpenGL driver: %s %s\n", glGetString(GL_VENDOR), 
            glGetString(GL_VERSION));
    fprintf(stderr, "Graphics hardware: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, 
            "-----------------------------------------------------------\n");
    if (path == NULL) {
        fprintf(stderr, "No path defined for shaders or resources\n");
        fflush(stderr);
        DoExit(1);
    }
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        getchar();
        //assert(false);
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if (!R2FilePath::getInstance()->setPath(path)) {
        fprintf(stderr, "can't set file path to %s\n", path);
        fflush(stderr);
        DoExit(1);
    }
    
    NvInitCG();
    NvShader::setErrorCallback(CgErrorCallback);
    
    fonts = new R2Fonts();
    fonts->addFont("verdana", "verdana.fnt");
    fonts->setFont("verdana");
    
    color_table_renderer = new NvColorTableRenderer();
    color_table_renderer->setFonts(fonts);
    particleRenderer = new NvParticleRenderer(NMESH, NMESH, g_context);

#if PROTOTYPE
    licRenderer = new NvLIC(NMESH, NPIX, NPIX, lic_axis, Vector3(lic_slice_x, lic_slice_y, lic_slice_z), g_context);
#endif

    ImageLoaderFactory::getInstance()->addLoaderImpl("bmp", new BMPImageLoaderImpl());
    grid = new Grid();
    grid->setFont(fonts);

    pointset_renderer = new PointSetRenderer();
}

/*----------------------------------------------------*/
void
NanoVis::initGL(void)
{
    if (debug_flag) {
	fprintf(stderr, "in initGL\n");
    }
    //buffer to store data read from the screen
    if (screen_buffer) {
	delete[] screen_buffer;
	screen_buffer = NULL;
    }
    screen_buffer = new unsigned char[4*win_width*win_height];
    assert(screen_buffer != NULL);

    //create the camera with default setting
    cam = new NvCamera(0, 0, win_width, win_height,
		       def_eye_x, def_eye_y, def_eye_z,          /* location. */
		       def_target_x, def_target_y, def_target_z, /* target. */
		       def_rot_x, def_rot_y, def_rot_z);         /* angle. */

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0,0,0,1);
    //glClearColor(0.7,0.7,0.7,1);
    glClear(GL_COLOR_BUFFER_BIT);

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
    Tcl_InitHashTable(&tftable, TCL_STRING_KEYS);

    //check if performance query is supported
    if(check_query_support()){
	//create queries to count number of rendered pixels
	perf = new PerfQuery();
    }

    init_offscreen_buffer();    //frame buffer object for offscreen rendering

    //create volume renderer and add volumes to it
    vol_renderer = new VolumeRenderer();

    // create
    renderContext = new graphics::RenderContext();
   
    //create an 2D plane renderer
    plane_render = new PlaneRenderer(g_context, win_width, win_height);
#if PROTOTYPE
    make_test_2D_data();
#endif  /* PROTOTYPE */
    plane_render->add_plane(plane[0], get_transfunc("default"));

    //assert(glGetError()==0);

    if (debug_flag) {
	fprintf(stderr, "leaving initGL\n");
    }
}

#if DO_RLE
char rle[512*512*3];
int rle_size;

short offsets[512*512*3];
int offsets_size;

void 
do_rle()
{
    int len = NanoVis::win_width*NanoVis::win_height*3;
    rle_size = 0;
    offsets_size = 0;

    int i=0;
    while(i<len){
        if (NanoVis::screen_buffer[i] == 0) {
            int pos = i+1;
            while ( (pos<len) && (NanoVis::screen_buffer[pos] == 0)) {
                pos++;
            }
            offsets[offsets_size++] = -(pos - i);
            i = pos;
        }

        else {
            int pos;
            for (pos = i; (pos<len) && (NanoVis::screen_buffer[pos] != 0);pos++){
                rle[rle_size++] = NanoVis::screen_buffer[pos];
            }
            offsets[offsets_size++] = (pos - i);
            i = pos;
        }

    }
}
#endif

// used internally to build up the BMP file header
// Writes an integer value into the header data structure at pos
static inline void
bmp_header_add_int(unsigned char* header, int& pos, int data)
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
NanoVis::bmp_write_to_file(int frame_number, const char *directory_name)
{
    unsigned char header[SIZEOF_BMP_HEADER];
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3*win_width) % 4 > 0) {
        pad = 4 - ((3*win_width) % 4);
    }

    // file size in bytes
    int fsize = (3*win_width+pad)*win_height + SIZEOF_BMP_HEADER;
    bmp_header_add_int(header, pos, fsize);

    // reserved value (must be 0)
    bmp_header_add_int(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmp_header_add_int(header, pos, SIZEOF_BMP_HEADER);

    // size of the BITMAPINFOHEADER
    bmp_header_add_int(header, pos, 40);

    // width of the image in pixels
    bmp_header_add_int(header, pos, win_width);

    // height of the image in pixels
    bmp_header_add_int(header, pos, win_height);

    // 1 plane + (24 bits/pixel << 16)
    bmp_header_add_int(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char* scr = screen_buffer;
    for (int row=0; row < win_height; row++) {
        for (int col=0; col < win_width; col++) {
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

        printf("Writing %s\n", filename);
        f = fopen(filename, "wb");
        if (f == 0) {
            Trace("cannot create file\n");
        }
    } else {
        f = fopen("/tmp/image.bmp", "wb");
        if (f == 0) {
            Trace("cannot create file\n");
        }
    }
    ssize_t nWritten;
    nWritten = fwrite(header, SIZEOF_BMP_HEADER, 1, f);
    assert(nWritten == SIZEOF_BMP_HEADER);
    nWritten = fwrite(screen_buffer, (3*win_width+pad)*win_height, 1, f);
    assert(nWritten == (3*win_width+pad)*win_height);
    fclose(f);
}

void
NanoVis::bmp_write(const char *prefix)
{
    unsigned char header[SIZEOF_BMP_HEADER];
    ssize_t nWritten;
    int pos = 0;

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3*win_width) % 4 > 0) {
        pad = 4 - ((3*win_width) % 4);
    }
    pad = 0;
    int fsize = (3*win_width+pad)*win_height + sizeof(header);

    char string[200];
    sprintf(string, "%s %d\n", prefix, fsize);
    nWritten = write(0, string, strlen(string));
    assert(nWritten == (ssize_t)strlen(string));
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    bmp_header_add_int(header, pos, fsize);

    // reserved value (must be 0)
    bmp_header_add_int(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmp_header_add_int(header, pos, SIZEOF_BMP_HEADER);

    // size of the BITMAPINFOHEADER
    bmp_header_add_int(header, pos, 40);

    // width of the image in pixels
    bmp_header_add_int(header, pos, win_width);

    // height of the image in pixels
    bmp_header_add_int(header, pos, win_height);

    // 1 plane + (24 bits/pixel << 16)
    bmp_header_add_int(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char* scr = screen_buffer;
    for (int row=0; row < win_height; row++) {
        for (int col=0; col < win_width; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    nWritten = write(0, header, SIZEOF_BMP_HEADER);
    assert(nWritten == SIZEOF_BMP_HEADER);
    nWritten = write(0, screen_buffer, (3*win_width+pad)*win_height);
    assert(nWritten == (3*win_width+pad)*win_height);
    stats.nFrames++;
    stats.nBytes += (3*win_width+pad)*win_height;
}

/*
 * ppm_write --
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
NanoVis::ppm_write(const char *prefix)
{
#define PPM_MAXVAL 255
    char header[200];

    // Generate the PPM binary file header
    sprintf(header, "P6 %d %d %d\n", win_width, win_height, PPM_MAXVAL);

    size_t header_length = strlen(header);
    size_t data_length = win_width * win_height * 3;

    char command[200];
    sprintf(command, "%s %lu\n", prefix, 
            (unsigned long)header_length + data_length);

    size_t wordsPerRow = (win_width * 24 + 31) / 32;
    size_t bytesPerRow = wordsPerRow * 4;
    size_t rowLength = win_width * 3;
    size_t nRecs = win_height + 2;

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
    unsigned char *srcRowPtr = screen_buffer;
    for (y = win_height + 1; y >= 2; y--) {
        iov[y].iov_base = srcRowPtr;
        iov[y].iov_len = rowLength;
        srcRowPtr += bytesPerRow;
    }
    writev(0, iov, nRecs);
    free(iov);
    stats.nFrames++;
    stats.nBytes += (bytesPerRow * win_height);
}

void
NanoVis::sendDataToClient(const char *command, const char *data, size_t dlen)
{
    /*
      char header[200];

      // Generate the PPM binary file header
      sprintf(header, "P6 %d %d %d\n", win_width, win_height, PPM_MAXVAL);

      size_t header_length = strlen(header);
      size_t data_length = win_width * win_height * 3;

      char command[200];
      sprintf(command, "%s %lu\n", prefix, 
      (unsigned long)header_length + data_length);
    */

    //    size_t wordsPerRow = (win_width * 24 + 31) / 32;
    //    size_t bytesPerRow = wordsPerRow * 4;
    //    size_t rowLength = win_width * 3;
    size_t nRecs = 2;

    struct iovec *iov;
    iov = (struct iovec *)malloc(sizeof(struct iovec) * nRecs);

    // Write the nanovisviewer command, then the image header and data.
    // Command
    // FIXME: shouldn't have to cast this
    iov[0].iov_base = (char *)command;
    iov[0].iov_len = strlen(command);
    // Data
    // FIXME: shouldn't have to cast this
    iov[1].iov_base = (char *)data;
    iov[1].iov_len = dlen;
    writev(0, iov, nRecs);
    free(iov);
    // stats.nFrames++;
    // stats.nBytes += (bytesPerRow * win_height);
}


/*----------------------------------------------------*/
void
NanoVis::idle()
{
    if (debug_flag) {
        fprintf(stderr, "in idle()\n");
    }
    glutSetWindow(render_window);

#ifdef XINETD
    xinetd_listen();
#else
    glutPostRedisplay();
#endif
    if (debug_flag) {
        fprintf(stderr, "leaving idle()\n");
    }
}

void 
NanoVis::display_offscreen_buffer()
{
    if (debug_flag) {
        fprintf(stderr, "in display_offscreen_buffer\n");
    }
    glEnable(GL_TEXTURE_2D);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBindTexture(GL_TEXTURE_2D, final_color_tex);

    glViewport(0, 0, win_width, win_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, win_width, 0, win_height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.,1.,1.);                //MUST HAVE THIS LINE!!!
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(win_width, 0);
        glTexCoord2f(1, 1); glVertex2f(win_width, win_height);
        glTexCoord2f(0, 1); glVertex2f(0, win_height);
    }
    glEnd();
    if (debug_flag) {
        fprintf(stderr, "leaving display_offscreen_buffer\n");
    }
}



#if 0
//oddeven sort on GPU
void
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
draw_3d_axis()
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    //draw axes
    GLUquadric *obj;

    obj = gluNewQuadric();

    glDepthFunc(GL_LESS);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

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

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    gluDeleteQuadric(obj);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
}

void NanoVis::update()
{
    if (vol_renderer->_volumeInterpolator->is_started()) {
        struct timeval clock;
        gettimeofday(&clock, NULL);
        double elapsed_time;

        elapsed_time = clock.tv_sec + clock.tv_usec/1000000.0 -
            vol_renderer->_volumeInterpolator->getStartTime();

        Trace("%lf %lf\n", elapsed_time, vol_renderer->_volumeInterpolator->getInterval());
        float fraction;
        float f;

        f = fmod((float) elapsed_time, (float)vol_renderer->_volumeInterpolator->getInterval());
        if (f == 0.0) {
            fraction = 0.0f;
        } else {
            fraction = f / vol_renderer->_volumeInterpolator->getInterval();
        }
        Trace("fraction : %f\n", fraction);
        vol_renderer->_volumeInterpolator->update(fraction);
    }
}

void
NanoVis::SetVolumeRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    if (debug_flag) {
        fprintf(stderr, "in SetVolumeRanges\n");
    }
    xMin = yMin = zMin = wMin = DBL_MAX;
    xMax = yMax = zMax = wMax = -DBL_MAX;
    for (unsigned int i = 0; i < volume.size(); i++) {
        Volume *volPtr;

        volPtr = volume[i];
        if (volPtr == NULL) {
            continue;
        }
        if (!volPtr->enabled) {
            continue;
        }
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
        grid->xAxis.SetScale(xMin, xMax);
    }
    if ((yMin < DBL_MAX) && (yMax > -DBL_MAX)) {
        grid->yAxis.SetScale(yMin, yMax);
    }
    if ((zMin < DBL_MAX) && (zMax > -DBL_MAX)) {
        grid->zAxis.SetScale(zMin, zMax);
    }
    if ((wMin < DBL_MAX) && (wMax > -DBL_MAX)) {
        Volume::valueMin = wMin;
        Volume::valueMax = wMax;
    }
    Volume::update_pending = false;
    if (debug_flag) {
        fprintf(stderr, "leaving SetVolumeRanges\n");
    }
}

void
NanoVis::SetHeightmapRanges()
{
    double xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;

    if (debug_flag) {
        fprintf(stderr, "in SetHeightmapRanges\n");
    }
    xMin = yMin = zMin = wMin = DBL_MAX;
    xMax = yMax = zMax = wMax = -DBL_MAX;
    for (unsigned int i = 0; i < heightMap.size(); i++) {
        HeightMap *hmPtr;

        hmPtr = heightMap[i];
        if (hmPtr == NULL) {
            continue;
        }
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
        grid->xAxis.SetScale(xMin, xMax);
    }
    if ((yMin < DBL_MAX) && (yMax > -DBL_MAX)) {
        grid->yAxis.SetScale(yMin, yMax);
    }
    if ((zMin < DBL_MAX) && (zMax > -DBL_MAX)) {
        grid->zAxis.SetScale(zMin, zMax);
    }
    if ((wMin < DBL_MAX) && (wMax > -DBL_MAX)) {
        HeightMap::valueMin = grid->yAxis.min();
        HeightMap::valueMax = grid->yAxis.max();
    }
    for (unsigned int i = 0; i < heightMap.size(); i++) {
        HeightMap *hmPtr;

        hmPtr = heightMap[i];
        if (hmPtr == NULL) {
            continue;
        }
        hmPtr->MapToGrid(grid);
    }
    HeightMap::update_pending = false;
    if (debug_flag) {
        fprintf(stderr, "leaving SetHeightmapRanges\n");
    }
}

/*----------------------------------------------------*/
void
NanoVis::display()
{
    if (debug_flag) {
        fprintf(stderr, "in display\n");
    }
    //assert(glGetError()==0);
    if (HeightMap::update_pending) {
        SetHeightmapRanges();
    }
    if (Volume::update_pending) {
        SetVolumeRanges();
    }
    if (debug_flag) {
        fprintf(stderr, "in display: glClear\n");
    }
    //start final rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

    if (volume_mode) {
        if (debug_flag) {
	    fprintf(stderr, "in display: volume_mode\n");
        }
        //3D rendering mode
	// TBD..
        //glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);

        //camera setting activated
        cam->activate();

        //set up the orientation of items in the scene.
        glPushMatrix();
        switch (updir) {
        case 1:  // x
            glRotatef(90, 0, 0, 1);
            glRotatef(90, 1, 0, 0);
            break;

        case 2:  // y
            // this is the default
            break;

        case 3:  // z
            glRotatef(-90, 1, 0, 0);
            glRotatef(-90, 0, 0, 1);
            break;

        case -1:  // -x
            glRotatef(-90, 0, 0, 1);
            break;

        case -2:  // -y
            glRotatef(180, 0, 0, 1);
            glRotatef(-90, 0, 1, 0);
            break;

        case -3:  // -z
            glRotatef(90, 1, 0, 0);
            break;
        }

        // TBD : This will be removed after being sure that all the functions work well.
        //glPushMatrix();

        //now render things in the scene
        if (axis_on) {
            draw_3d_axis();
        }
        if (grid->isVisible()) {
            grid->render();
        }
        if ((licRenderer != NULL) && (licRenderer->isActivated())) {
            licRenderer->render();
        }
        if ((particleRenderer != NULL) && (particleRenderer->isActivated())) {
            particleRenderer->render();
        }
        //soft_display_verts();
        //perf->enable();
        //perf->disable();
        //fprintf(stderr, "particle pixels: %d\n", perf->get_pixel_count());
        //perf->reset();

        //perf->enable();
        vol_renderer->render_all();
        //perf->disable();

        if (debug_flag) {
	    fprintf(stderr, "in display: render heightmap\n");
        }
        for (unsigned int i = 0; i < heightMap.size(); ++i) {
            if (heightMap[i]->isVisible()) {
                heightMap[i]->render(renderContext);
            }
        }
        glPopMatrix();
    } else {
        //2D rendering mode
        perf->enable();
        plane_render->render();
        perf->disable();
    }

    perf->reset();
    if (debug_flag) {
        fprintf(stderr, "leaving display\n");
    }

}

#ifndef XINETD
void 
NanoVis::mouse(int button, int state, int x, int y)
{
    if(button==GLUT_LEFT_BUTTON){
        if (state==GLUT_DOWN) {
            left_last_x = x;
            left_last_y = y;
            left_down = true;
            right_down = false;
        } else {
            left_down = false;
            right_down = false;
        }
    } else {
        //fprintf(stderr, "right mouse\n");

        if(state==GLUT_DOWN){
            //fprintf(stderr, "right mouse down\n");
            right_last_x = x;
            right_last_y = y;
            left_down = false;
            right_down = true;
        } else {
            //fprintf(stderr, "right mouse up\n");
            left_down = false;
            right_down = false;
        }
    }
}

void
NanoVis::update_rot(int delta_x, int delta_y)
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
NanoVis::update_trans(int delta_x, int delta_y, int delta_z)
{
    cam->x(cam->x() + delta_x * 0.03);
    cam->y(cam->y() + delta_y * 0.03);
    cam->z(cam->z() + delta_z * 0.03);
}

void 
NanoVis::keyboard(unsigned char key, int x, int y)
{
#ifdef EVENTLOG
    if(log){
	float param[3];
	param[0] = cam->x();
	param[1] = cam->y();
	param[2] = cam->z();
	Event* tmp = new Event(EVENT_MOVE, param, NvGetTimeInterval());
	tmp->write(event_log);
	delete tmp;
    }
#endif
}

void 
NanoVis::motion(int x, int y)
{
    int old_x, old_y;

    if(left_down){
        old_x = left_last_x;
        old_y = left_last_y;
    } else if(right_down){
        old_x = right_last_x;
        old_y = right_last_y;
    }

    int delta_x = x - old_x;
    int delta_y = y - old_y;

    //more coarse event handling
    //if(abs(delta_x)<10 && abs(delta_y)<10)
    //return;

    if(left_down){
        left_last_x = x;
        left_last_y = y;

        update_rot(-delta_y, -delta_x);
    } else if (right_down){
        //fprintf(stderr, "right mouse motion (%d,%d)\n", x, y);

        right_last_x = x;
        right_last_y = y;

        update_trans(0, 0, delta_x);
    }

#ifdef EVENTLOG
    Vector3 angle = cam->rotate();
    Event* tmp = new Event(EVENT_ROTATE, &angle, NvGetTimeInterval());
    tmp->write(event_log);
    delete tmp;
#endif
    glutPostRedisplay();
}

#endif /*XINETD*/

void 
NanoVis::render()
{
    if ((NanoVis::licRenderer != NULL) && 
	(NanoVis::licRenderer->isActivated())) {
	NanoVis::licRenderer->convolve();
    }

    if ((NanoVis::particleRenderer != NULL) && 
	(NanoVis::particleRenderer->isActivated())) {
	NanoVis::particleRenderer->advect();
    }

    NanoVis::update();
    display();
    glutSwapBuffers();
}

void 
NanoVis::resize(int x, int y)
{
    glViewport(0, 0, x, y);
}

void 
NanoVis::xinetd_listen(void)
{
    if (debug_flag) {
        fprintf(stderr, "in xinetd_listen\n");
    }
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

    int status = TCL_OK;

    //
    //  Read and execute as many commands as we can from stdin...
    //
    bool isComplete = false;
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
        if (debug_flag) {
            fprintf(stderr, "in xinetd_listen: check eof %d\n", 
                    feof(NanoVis::stdin));
        }
        while (!feof(NanoVis::stdin)) {
            int c = fgetc(NanoVis::stdin);
            char ch;
            if (c <= 0) {
                if (errno == EWOULDBLOCK) {
                    break;
                }
                DoExit(0);
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
            status = ExecuteCommand(interp, &cmdbuffer);
            // non-blocking for next read -- we might not get anything
            fcntl(0, F_SETFL, flags | O_NONBLOCK);
            isComplete = false;
        }
    }
    fcntl(0, F_SETFL, flags);

    if (status != TCL_OK) {
        const char *string;
        int nBytes;

        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        nBytes = strlen(string);
        struct iovec iov[3];
        iov[0].iov_base = (char *)"NanoVis Server Error: ";
        iov[0].iov_len = strlen((char *)iov[0].iov_base);
        iov[1].iov_base = (char *)string;
        iov[1].iov_len = nBytes;
        iov[2].iov_len = 1;
        iov[2].iov_base = (char *)'\n';
        writev(0, iov, 3);
        if (debug_flag) {
            fprintf(stderr, "leaving xinetd_listen\n");
        }
        return;
    }

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
    if (debug_flag) {
        fprintf(stderr, "ppm image not written (debug mode)\n");
        bmp_write_to_file(1, "/tmp");
    } else {
        NanoVis::ppm_write("nv>image -type image -bytes");
    }
#endif
    if (feof(NanoVis::stdin)) {
        DoExit(0);
    }
    if (debug_flag) {
        fprintf(stderr, "leaving xinetd_listen\n");
    }
}


/*----------------------------------------------------*/
int 
main(int argc, char** argv)
{
    const char *path;
    char *newPath;
    struct timeval tv;

    newPath = NULL;
    path = NULL;
    NanoVis::stdin = stdin;
    NanoVis::logfile = stderr;

    gettimeofday(&tv, NULL);
    stats.start = tv;

    /* Initialize GLUT here so it can parse and remove GLUT-specific 
     * command-line options before we parse the command-line below. */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(NanoVis::win_width, NanoVis::win_height);
    glutInitWindowPosition(10, 10);
    NanoVis::render_window = glutCreateWindow("nanovis");
    glutIdleFunc(NanoVis::idle);

#ifndef XINETD
    glutMouseFunc(NanoVis::mouse);
    glutMotionFunc(NanoVis::motion);
    glutKeyboardFunc(NanoVis::keyboard);
    glutReshapeFunc(NanoVis::resize);
    glutDisplayFunc(NanoVis::render);
#else
    glutDisplayFunc(NanoVis::display);
    glutReshapeFunc(NanoVis::resize_offscreen_buffer);
#endif

    while (1) {
        static struct option long_options[] = {
            {"infile",  required_argument, NULL,	   0},
            {"logfile", required_argument, NULL,	   1},
            {"path",    required_argument, NULL,	   2},
            {"debug",   no_argument,       NULL,	   3},
            {"record",  required_argument, NULL,	   4},
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
            NanoVis::debug_flag = true;
            break;
        case 0:
        case 'i':
            NanoVis::stdin = fopen(optarg, "r");
            if (NanoVis::stdin == NULL) {
                perror(optarg);
                return 2;
            }
            break;
        case 1:
        case 'l':
            NanoVis::logfile = fopen(optarg, "w");
            if (NanoVis::logfile == NULL) {
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
        p = strrchr(path, '/');
        if (p != NULL) {
            *p = '\0';
            p = strrchr(path, '/');
        }
        if (p == NULL) {
            fprintf(stderr, "path not specified\n");
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

#ifdef XINETD
    signal(SIGPIPE,SIG_IGN);
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
    Tcl_DStringInit(&NanoVis::cmdbuffer);
    NanoVis::interp = initTcl();
    NanoVis::resize_offscreen_buffer(NanoVis::win_width, NanoVis::win_height);

    glutMainLoop();
    DoExit(0);
}

int
NanoVis::render_2d_contour(HeightMap* heightmap, int width, int height)
{
    int old_width = win_width;
    int old_height = win_height;

    resize_offscreen_buffer(width, height);

    /*
      plane_render->set_screen_size(width, height);

      // generate data for the legend
      float data[512];
      for (int i=0; i < 256; i++) {
      data[i] = data[i+256] = (float)(i/255.0);
      }
      plane[0] = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
      int index = plane_render->add_plane(plane[0], tf);
      plane_render->set_active_plane(index);

      offscreen_buffer_capture();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

      //plane_render->render();
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
    //ppm_write(prefix);
    //write(0, "\n", 1);
    //plane_render->remove_plane(index);

    // CURRENT
    offscreen_buffer_capture();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen
    //glEnable(GL_TEXTURE_2D);
    //glEnable(GL_DEPTH_TEST);
    //heightmap->render_topview(renderContext, width, height);
    //NanoVis::display();
    if (HeightMap::update_pending) {
        SetHeightmapRanges();
    }

    //cam->activate();

    heightmap->render_topview(renderContext, width, height);

    NanoVis::read_screen();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    // INSOO TEST CODE
    bmp_write_to_file(1, "/tmp");

    resize_offscreen_buffer(old_width, old_height);

    return TCL_OK;
}
