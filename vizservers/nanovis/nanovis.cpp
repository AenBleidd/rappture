
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

#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

#include "Nv.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "Util.h"
#include <NvLIC.h>
#include <Trace.h>

#include "nanovis.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"
#include "RpEncode.h"

//transfer function headers
#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"
#include "ZincBlendeVolume.h"
#include "NvLoadFile.h"
#include "NvColorTableRenderer.h"
#include "NvEventLog.h"
#include "NvZincBlendeReconstructor.h"
#include "HeightMap.h"
#include "Grid.h"
#include "VolumeInterpolator.h"
#include <RenderContext.h>

#include <imgLoaders/BMPImageLoaderImpl.h>
#include <imgLoaders/ImageLoaderFactory.h>

//#define  _LOCAL_ZINC_TEST_

// R2 headers
#include <R2/R2FilePath.h>
#include <R2/R2Fonts.h>

extern void NvInitCG(); // in Shader.cpp

// Indicates "up" axis:  x=1, y=2, z=3, -x=-1, -y=-2, -z=-3
enum AxisDirections { 
    X_POS = 1, Y_POS = 2, Z_POS = 3, X_NEG = -1, Y_NEG = -2, Z_NEG = -3
};

// STATIC MEMBER DATA
int NanoVis::updir = Y_POS;
NvCamera* NanoVis::cam = NULL;
bool NanoVis::axis_on = true;
//bool NanoVis::axis_on = false;
int NanoVis::win_width = NPIX;                  //size of the render window
int NanoVis::win_height = NPIX;                 //size of the render window
int NanoVis::n_volumes = 0;
unsigned char* NanoVis::screen_buffer = NULL;
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

bool NanoVis::lic_on = false;
bool NanoVis::particle_on = false;
bool NanoVis::vector_on = false;

// pointers to volumes, currently handle up to 10 volumes
/*FIXME: Is the above comment true? Is there a 10 volume limit */
vector<Volume*> NanoVis::volume;

//if true nanovis renders volumes in 3D, if not renders 2D plane
/* FIXME: This variable is always true. */
bool volume_mode = true; 

// color table for built-in transfer function editor
float color_table[256][4];      

/*
#ifdef XINETD
FILE* xinetd_log;
#endif

FILE* event_log;
//log
void init_event_log();
void end_event_log();
double cur_time;        //in seconds
double get_time_interval();
*/

int render_window;              //the handle of the render window;

// in Command.cpp
extern void xinetd_listen();
extern void initTcl(); 

//ParticleSystem* psys;
//float psys_x=0.4, psys_y=0, psys_z=0;

//static Lic* lic;

//frame buffer for final rendering
static NVISid final_fbo, final_color_tex, final_depth_rb;

//bool advect=false;
float vert[NMESH*NMESH*3];              //particle positions in main memory
float slice_vector[NMESH*NMESH*4];      //per slice vectors in main memory

// maps transfunc name to TransferFunction object
static Tcl_HashTable tftable;

// pointers to 2D planes, currently handle up 10


PerfQuery* perf;                        //perfromance counter

CGprogram m_passthru_fprog;
CGparameter m_passthru_scale_param, m_passthru_bias_param;

R2Fonts* NanoVis::fonts;

// Variables for mouse events

// Object rotation angles
static float live_rot_x = 90.;          
static float live_rot_y = 180.;
static float live_rot_z = -135;

// Object translation location from the origin
static float live_obj_x = -0.0; 
static float live_obj_y = -0.0;
static float live_obj_z = -2.5;


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
float NanoVis::lic_slice_x = 1.0f;
float NanoVis::lic_slice_y = 0.0f; 
float NanoVis::lic_slice_z = 0.5f; 
int NanoVis::lic_axis = 2; 

/*
CGprogram m_copy_texcoord_fprog;
CGprogram m_one_volume_fprog;
CGparameter m_vol_one_volume_param;
CGparameter m_tf_one_volume_param;
CGparameter m_mvi_one_volume_param;
CGparameter m_mv_one_volume_param;
CGparameter m_render_param_one_volume_param;
*/

/*
CGprogram m_vert_std_vprog;
CGparameter m_mvp_vert_std_param;
CGparameter m_mvi_vert_std_param;
*/

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
    write(0, str, strlen(str));
}

void
debug(char *str, double v1)
{
    char buffer[512];
    sprintf(buffer, str, v1);
    write(0, buffer, strlen(buffer));
}

void
debug(char *str, double v1, double v2)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2);
    write(0, buffer, strlen(buffer));
}

void
debug(char *str, double v1, double v2, double v3)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2, v3);
    write(0, buffer, strlen(buffer));
}

//report errors related to CG shaders
void cgErrorCallback(void)
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
        exit(-1);
    }
}


/* Load a 3D volume
 * index: the index into the volume array, which stores pointers to 3D volume instances
 * data: pointer to an array of floats.  
 * n_component: the number of scalars for each space point. 
 *              All component scalars for a point are placed consequtively in data array 
 * width, height and depth: number of points in each dimension
 */
void 
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

        if (vol->pointsetIndex != -1)
        {
            if (((unsigned  int) vol->pointsetIndex) < pointSet.size() && pointSet[vol->pointsetIndex] != NULL)
            {
                delete pointSet[vol->pointsetIndex];
                pointSet[vol->pointsetIndex] = 0;
            }
        }

        delete vol;
    }

    volume[index] = new Volume(0.f, 0.f, 0.f, width, height, depth, 1.,
                               n_component, data, vmin, vmax, nzero_min);
    assert(volume[index]!=0);
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
NanoVis::set_transfunc(const char *name, int nSlots, float *data) 
{
    int isNew;
    Tcl_HashEntry *hPtr;
    TransferFunction *tf;

    hPtr = Tcl_CreateHashEntry(&tftable, name, &isNew);
    if (isNew) {
        tf = new TransferFunction(nSlots, data);
        Tcl_SetHashValue(hPtr, (ClientData)tf);
    } else {
        tf = (TransferFunction*)Tcl_GetHashValue(hPtr);
        tf->update(data);
    }
    return tf;
}

void
NanoVis::zoom(double zoom)
{
    live_obj_z = -2.5 / zoom;
    cam->move(live_obj_x, live_obj_y, live_obj_z);
}

//Update the transfer function using local GUI in the non-server mode
void 
update_tf_texture()
{
    glutSetWindow(render_window);
    
    //fprintf(stderr, "tf update\n");
    TransferFunction *tf = NanoVis::get_transfunc("default");
    if (tf == NULL) {
        return;
    }
    
    float data[256*4];
    for(int i=0; i<256; i++) {
        data[4*i+0] = color_table[i][0];
        data[4*i+1] = color_table[i][1];
        data[4*i+2] = color_table[i][2];
        data[4*i+3] = color_table[i][3];
        //fprintf(stderr, "(%f,%f,%f,%f) ", data[4*i+0], data[4*i+1], data[4*i+2], data[4*i+3]);
    }
    
    tf->update(data);
    
#ifdef EVENTLOG
    float param[3] = {0,0,0};
    Event* tmp = new Event(EVENT_ROTATE, param, NvGetTimeInterval());
    tmp->write(event_log);
    delete tmp;
#endif
}

int 
NanoVis::render_legend(
    TransferFunction *tf, 
    double min, double max, 
    int width, int height, 
    const char* volArg)
{
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

    std::ostringstream result;
    result << "nv>legend " << volArg;
    result << " " << min;
    result << " " << max;
    bmp_write(result.str().c_str());
    write(0, "\n", 1);

    plane_render->remove_plane(index);
    resize_offscreen_buffer(old_width, old_height);

    return TCL_OK;
}

//initialize frame buffer objects for offscreen rendering
void 
NanoVis::init_offscreen_buffer()
{
    //initialize final fbo for final display
    glGenFramebuffersEXT(1, &final_fbo);
    glGenTextures(1, &final_color_tex);
    glGenRenderbuffersEXT(1, &final_depth_rb);
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    
    //initialize final color texture
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
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                              GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, final_color_tex, 0);
    
    // initialize final depth renderbuffer
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                             GL_DEPTH_COMPONENT24, win_width, win_height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                 GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, final_depth_rb);
    
    // Check framebuffer completeness at the end of initialization.
    CHECK_FRAMEBUFFER_STATUS();

    //assert(glGetError()==0);
}


//resize the offscreen buffer 
void 
NanoVis::resize_offscreen_buffer(int w, int h)
{
    win_width = w;
    win_height = h;
    
    if (fonts) {
        fonts->resize(w, h);
    }
    
    //fprintf(stderr, "screen_buffer size: %d\n", sizeof(screen_buffer));
    printf("screen_buffer size: %d %d\n", w, h);
    
    if (screen_buffer) {
        delete[] screen_buffer;
        screen_buffer = NULL;
    }
    
    screen_buffer = new unsigned char[4*win_width*win_height];
    assert(screen_buffer != NULL);
    
    //delete the current render buffer resources
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    glDeleteTextures(1, &final_color_tex);
    glDeleteFramebuffersEXT(1, &final_fbo);
    
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glDeleteRenderbuffersEXT(1, &final_depth_rb);
    
    //change the camera setting
    cam->set_screen_size(0, 0, win_width, win_height);
    plane_render->set_screen_size(win_width, win_height);
    
    //Reinitialize final fbo for final display
    glGenFramebuffersEXT(1, &final_fbo);
    glGenTextures(1, &final_color_tex);
    glGenRenderbuffersEXT(1, &final_depth_rb);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    
    //initialize final color texture
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
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                              GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, final_color_tex, 0);
        
    // initialize final depth renderbuffer
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                             GL_DEPTH_COMPONENT24, win_width, win_height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                 GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, final_depth_rb);

    // Check framebuffer completeness at the end of initialization.
    CHECK_FRAMEBUFFER_STATUS();
    //assert(glGetError()==0);
    fprintf(stdin,"  after assert\n");
}

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
    float* data = new float[particleRenderer->psys_width * particleRenderer->psys_height * 4];
    bzero(data, sizeof(float)*4* particleRenderer->psys_width * particleRenderer->psys_height);

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
                data[4*index] = lic_slice_x;
                data[4*index+1]= j/float(particleRenderer->psys_height);
                data[4*index+2]= i/float(particleRenderer->psys_width);
                data[4*index+3]= 30; //shorter life span, quicker iterations    
                //data[4*index+3]= 1.0f; //shorter life span, quicker iterations        
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
        exit(-1);
    }
}

void NanoVis::init(const char* path)
{
    // print system information
    fprintf(stderr, "-----------------------------------------------------------\n");
    fprintf(stderr, "OpenGL driver: %s %s\n", glGetString(GL_VENDOR), glGetString(GL_VERSION));
    fprintf(stderr, "Graphics hardware: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, "-----------------------------------------------------------\n");

    // init GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        //glew init failed, exit.
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        getchar();
        //assert(false);
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if (path)
    {
        R2FilePath::getInstance()->setPath(path);
    }

    NvInitCG();
    NvShader::setErrorCallback(CgErrorCallback);

/*
    fonts = new R2Fonts();
    fonts->addFont("verdana", "verdana.fnt");
    fonts->setFont("verdana");
*/

    color_table_renderer = new NvColorTableRenderer();
    color_table_renderer->setFonts(fonts);
    
    particleRenderer = new NvParticleRenderer(NMESH, NMESH, g_context);
    licRenderer = new NvLIC(NMESH, NPIX, NPIX, 0.5, g_context);

    ImageLoaderFactory::getInstance()->addLoaderImpl("bmp", new BMPImageLoaderImpl());

    grid = new Grid();
    grid->setFont(fonts);

    pointset_renderer = new PointSetRenderer();
}

/*----------------------------------------------------*/
void 
NanoVis::initGL(void) 
{ 
   //buffer to store data read from the screen
   if (screen_buffer) {
       delete[] screen_buffer;
       screen_buffer = NULL;
   }
   screen_buffer = new unsigned char[4*win_width*win_height];
   assert(screen_buffer != NULL);

   //create the camera with default setting
   cam = new NvCamera(0, 0, win_width, win_height, 
                   live_obj_x, live_obj_y, live_obj_z,
                   0., 0., 100.,
                   (int)live_rot_x, (int)live_rot_y, (int)live_rot_z);

   glEnable(GL_TEXTURE_2D);
   glShadeModel(GL_FLAT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glClearColor(0,0,0,1);
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
   
#ifdef notdef
   //I added this to debug : Wei
   float tmp_data[4*124];
   memset(tmp_data, 0, 4*4*124);
   TransferFunction* tmp_tf = new TransferFunction(124, tmp_data);
   vol_renderer->add_volume(volume[0], tmp_tf);
   volume[0]->get_cutplane(0)->enabled = false;
   volume[0]->get_cutplane(1)->enabled = false;
   volume[0]->get_cutplane(2)->enabled = false;

   //volume[1]->move(Vector3(0.5, 0.6, 0.7));
   //vol_renderer->add_volume(volume[1], tmp_tf);
#endif

   //create an 2D plane renderer
   plane_render = new PlaneRenderer(g_context, win_width, win_height);
   make_test_2D_data();

   plane_render->add_plane(plane[0], get_transfunc("default"));

   //assert(glGetError()==0);

#ifdef notdef
   init_particle_system();
   NanoVis::init_lic(); 
#endif
}

void 
NanoVis::read_screen()
{
    glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, 
                 screen_buffer);
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
// writes an integer value into the header data structure at pos
static void
bmp_header_add_int(unsigned char* header, int& pos, int data)
{
    header[pos++] = data & 0xff;
    header[pos++] = (data >> 8) & 0xff;
    header[pos++] = (data >> 16) & 0xff;
    header[pos++] = (data >> 24) & 0xff;
}

// INSOO
// FOR DEBUGGING
void
NanoVis::bmp_write_to_file(int frame_number)
{
    unsigned char header[54];
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
    int fsize = (3*win_width+pad)*win_height + sizeof(header);
    bmp_header_add_int(header, pos, fsize);

    // reserved value (must be 0)
    bmp_header_add_int(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmp_header_add_int(header, pos, sizeof(header));

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
        sprintf(filename, "/tmp/flow_animation1/image%03d.bmp", frame_number);
        printf("Writing %s\n", filename);
        f = fopen(filename, "wb");
    }
    else {
        f = fopen("/tmp/image.bmp", "wb");
    }
    fwrite((void*) header, sizeof(header), 1, f);
    fwrite((void*) screen_buffer, (3*win_width+pad)*win_height, 1, f);
    fclose(f);
}

void
NanoVis::bmp_write(const char* cmd)
{
    unsigned char header[54];
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
    int fsize = (3*win_width+pad)*win_height + sizeof(header);
    bmp_header_add_int(header, pos, fsize);

    // reserved value (must be 0)
    bmp_header_add_int(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmp_header_add_int(header, pos, sizeof(header));

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

    std::ostringstream result;
    result << cmd << " " << fsize << "\n";
    write(0, result.str().c_str(), result.str().size());

    write(0, header, sizeof(header));
    write(0, screen_buffer, (3*win_width+pad)*win_height);

}

#ifdef notdef
//draw vectors 
void draw_arrows()
{
    glColor4f(0.,0.,1.,1.);
    for(int i=0; i<NMESH; i++){
        for(int j=0; j<NMESH; j++){
            Vector2 v = grid.get(i, j);
            
            int x1 = i*DM;
            int y1 = j*DM;
            
            int x2 = x1 + v.x;
            int y2 = y1 + v.y;
            
            glBegin(GL_LINES);
            glVertex2d(x1, y1);
            glVertex2d(x2, y2);
            glEnd();
        }
    }
}
#endif


/*----------------------------------------------------*/
static void 
idle()
{
    glutSetWindow(render_window);
  
#ifdef notdef
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 300000000;
      nanosleep(&ts, 0);
#endif
    
#ifdef XINETD
    xinetd_listen();
#else
    glutPostRedisplay();
#endif
}

void 
NanoVis::display_offscreen_buffer()
{
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
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(win_width, 0);
    glTexCoord2f(1, 1); glVertex2f(win_width, win_height);
    glTexCoord2f(0, 1); glVertex2f(0, win_height);
    glEnd();
}

void 
NanoVis::offscreen_buffer_capture()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
}

/* 
 * Is this routine being used? --gah
 */
void 
draw_bounding_box(float x0, float y0, float z0,
                  float x1, float y1, float z1,
                  float r, float g, float b, float line_width)
{
    glDisable(GL_TEXTURE_2D);
    
    glColor4d(r, g, b, 1.0);
    glLineWidth(line_width);
    
    glBegin(GL_LINE_LOOP);
    
    glVertex3d(x0, y0, z0);
    glVertex3d(x1, y0, z0);
    glVertex3d(x1, y1, z0);
    glVertex3d(x0, y1, z0);
    
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    
    glVertex3d(x0, y0, z1);
    glVertex3d(x1, y0, z1);
    glVertex3d(x1, y1, z1);
    glVertex3d(x0, y1, z1);
    
    glEnd();
    
    
    glBegin(GL_LINE_LOOP);
    
    glVertex3d(x0, y0, z0);
    glVertex3d(x0, y0, z1);
    glVertex3d(x0, y1, z1);
    glVertex3d(x0, y1, z0);
    
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    
    glVertex3d(x1, y0, z0);
    glVertex3d(x1, y0, z1);
    glVertex3d(x1, y1, z1);
    glVertex3d(x1, y1, z0);
    
    glEnd();
    
    glEnable(GL_TEXTURE_2D);
}


#ifdef notdef

static int 
particle_distance_sort(const void* a, const void* b)
{
    if((*((Particle*)a)).aux > (*((Particle*)b)).aux) {
        return -1;
    } else {
        return 1;
    }
}

void soft_read_verts()
{
    glReadPixels(0, 0, psys->psys_width, psys->psys_height, GL_RGB, GL_FLOAT, vert);
    //fprintf(stderr, "soft_read_vert");
    
    //cpu sort the distance  
    Particle* p;
    p = (Particle*)malloc(sizeof(Particle)*psys->psys_width*psys->psys_height);
    for (int i=0; i<psys->psys_width * psys->psys_height; i++) {
        float x = vert[3*i];
        float y = vert[3*i+1];
        float z = vert[3*i+2];
        
        float dis = (x-live_obj_x)*(x-live_obj_x) + (y-live_obj_y)*(y-live_obj_y) + (z-live_obj_z)*(z-live_obj_z); 
        p[i].x = x;
        p[i].y = y;
        p[i].z = z;
        p[i].aux = dis;
    }
    
    qsort(p, psys->psys_width * psys->psys_height, sizeof(Particle), particle_distance_sort);
    
    for(int i=0; i<psys->psys_width * psys->psys_height; i++){
        vert[3*i] = p[i].x;
        vert[3*i+1] = p[i].y;
        vert[3*i+2] = p[i].z;
    }
    
    free(p);
}
#endif

#ifdef notdef
//display the content of a texture as a screen aligned quad
void 
display_texture(NVISid tex, int width, int height)
{
    glPushMatrix();
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, tex);
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    cgGLBindProgram(m_passthru_fprog);
    cgGLEnableProfile(CG_PROFILE_FP30);
    
    cgGLSetParameter4f(m_passthru_scale_param, 1.0, 1.0, 1.0, 1.0);
    cgGLSetParameter4f(m_passthru_bias_param, 0.0, 0.0, 0.0, 0.0);
    
    draw_quad(width, height, width, height);
    cgGLDisableProfile(CG_PROFILE_FP30);
    
    glPopMatrix();
    
    //assert(glGetError()==0);
}
#endif


//draw vertices in the main memory
#ifdef notdef
void 
soft_display_verts()
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    glPointSize(0.5);
    glColor4f(0,0.8,0.8,0.6);
    glBegin(GL_POINTS);
    for(int i=0; i<psys->psys_width * psys->psys_height; i++){
        glVertex3f(vert[3*i], vert[3*i+1], vert[3*i+2]);
    }
    glEnd();
    //fprintf(stderr, "soft_display_vert");
}
#endif

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
    glUniform3fARB(oddevenMergeSort.getUniformLocation("Param2"), float(psys_width), float(psys_height), float(ppass));
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
#ifdef notdef
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f); glVertex2f(-1.0f,-1.0f); 
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f); glVertex2f(1.0f,-1.0f);
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f); glVertex2f(1.0f,1.0f);        
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f); glVertex2f(-1.0f,1.0f);    
#endif
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f); glVertex2f(0.,0.);       
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f); glVertex2f(float(psys_width), 0.);
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f); glVertex2f(float(psys_width), float(psys_height));    
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f); glVertex2f(0., float(psys_height));        
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

#ifdef notdef
void 
draw_axis()
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    
    //red x
    glColor3f(1,0,0);
    glBegin(GL_LINES);
    {
        glVertex3f(0,0,0);
        glVertex3f(1.5,0,0);
    }
    glEnd();
    
    //blue y
    glColor3f(0,0,1);
    glBegin(GL_LINES);
    {
        glVertex3f(0,0,0);
        glVertex3f(0,1.5,0);
    }
    glEnd();
    
    //green z
    glColor3f(0,1,0);
    glBegin(GL_LINES);
    {
        glVertex3f(0,0,0);
        glVertex3f(0,0,1.5);
    }
    glEnd();
    
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
}
#endif

void NanoVis::update()
{
    if (vol_renderer->_volumeInterpolator->is_started()) {
        struct timeval clock;
        gettimeofday(&clock, NULL);
        double elapsed_time = clock.tv_sec + clock.tv_usec/1000000.0 - vol_renderer->_volumeInterpolator->getStartTime();
        
        Trace("%lf %lf\n", elapsed_time, vol_renderer->_volumeInterpolator->getInterval());
        float fraction;
        float f;

        f = fmod((float) elapsed_time, (float) vol_renderer->_volumeInterpolator->getInterval());
        if (f == 0.0) {
            fraction = 0.0f;
        } else {
            fraction = f / vol_renderer->_volumeInterpolator->getInterval();
        }

        Trace("fraction : %f\n", fraction);
        vol_renderer->_volumeInterpolator->update(fraction);
    }
}

/*----------------------------------------------------*/
void 
NanoVis::display()
{
    //assert(glGetError()==0);
    
    //start final rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

    if (volume_mode) {
        //3D rendering mode
        glEnable(GL_TEXTURE_2D);
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
        if (licRenderer && licRenderer->isActivated()) {
            licRenderer->render();
        }
        if (particleRenderer && particleRenderer->isActivated()) {
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

#ifdef XINETD
    float cost  = perf->get_pixel_count();
    write(3, &cost, sizeof(cost));
#endif
    perf->reset();

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
    live_rot_x += delta_x;
    live_rot_y += delta_y;
    
    if (live_rot_x > 360.0) {
        live_rot_x -= 360.0;    
    } else if(live_rot_x < -360.0) {
        live_rot_x += 360.0;
    }
    if (live_rot_y > 360.0) {
        live_rot_y -= 360.0;    
    } else if(live_rot_y < -360.0) {
        live_rot_y += 360.0;
    }
    cam->rotate(live_rot_x, live_rot_y, live_rot_z);
}

void 
NanoVis::update_trans(int delta_x, int delta_y, int delta_z)
{
    live_obj_x += delta_x*0.03;
    live_obj_y += delta_y*0.03;
    live_obj_z += delta_z*0.03;
}

void 
NanoVis::keyboard(unsigned char key, int x, int y)
{
   bool log = false;

   switch (key) {
   case 'q':
       exit(0);
       break;
   case '+':
       lic_slice_z+=0.05;
       lic->set_offset(lic_slice_z);
       break;
   case '-':
       lic_slice_z-=0.05;
       lic->set_offset(lic_slice_z);
       break;
   case ',':
       lic_slice_x+=0.05;
       //init_particles();
       break;
   case '.':
       lic_slice_x-=0.05;
       //init_particles();
       break;
   case '1':
       //advect = true;
       break;
   case '2':
       //psys_x+=0.05;
       break;
   case '3':
       //psys_x-=0.05;
       break;
   case 'w': //zoom out
       live_obj_z-=0.05;
       log = true;
       cam->move(live_obj_x, live_obj_y, live_obj_z);
       break;
   case 's': //zoom in
       live_obj_z+=0.05;
       log = true;
       cam->move(live_obj_x, live_obj_y, live_obj_z);
       break;
   case 'a': //left
       live_obj_x-=0.05;
       log = true;
       cam->move(live_obj_x, live_obj_y, live_obj_z);
       break;
   case 'd': //right
       live_obj_x+=0.05;
       log = true;
       cam->move(live_obj_x, live_obj_y, live_obj_z);
       break;
   case 'i':
       //init_particles();
       break;
   case 'v':
       vol_renderer->switch_volume_mode();
       break;
   case 'b':
       vol_renderer->switch_slice_mode();
       break;
   case 'n':
       resize_offscreen_buffer(win_width*2, win_height*2);
       break;
   case 'm':
       resize_offscreen_buffer(win_width/2, win_height/2);
       break;
   default:
       break;
   }    
#ifdef EVENTLOG
   if(log){
       float param[3] = {live_obj_x, live_obj_y, live_obj_z};
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
    float param[3] = {live_rot_x, live_rot_y, live_rot_z};
    Event* tmp = new Event(EVENT_ROTATE, param, NvGetTimeInterval());
    tmp->write(event_log);
    delete tmp;
#endif
    glutPostRedisplay();
}

#endif /*XINETD*/

#ifdef notdef

#ifdef XINETD
void 
init_service()
{
    //open log and map stderr to log file
    xinetd_log = fopen("/tmp/log.txt", "w");
    close(2);
    dup2(fileno(xinetd_log), 2);
    dup2(2,1);
    
    //flush junk 
    fflush(stdout);
    fflush(stderr);
}

void 
end_service()
{
    //close log file
    fclose(xinetd_log);
}
#endif  /*XINETD*/

void 
init_event_log() 
{
    event_log = fopen("event.txt", "w");
    assert(event_log!=0);
    
    struct timeval time;
    gettimeofday(&time, NULL);
    cur_time = time.tv_sec*1000. + time.tv_usec/1000.;
}

void 
end_event_log()
{
    fclose(event_log);
}

double 
get_time_interval()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    double new_time = time.tv_sec*1000. + time.tv_usec/1000.;
    
    double interval = new_time - cur_time;
    cur_time = new_time;
    return interval;
}
#endif 

void removeAllData()
{
    //
}


/*----------------------------------------------------*/
int 
main(int argc, char** argv) 
{

    char *path;
    path = NULL;
    while(1) {
        int c;
        int option_index = 0;
        struct option long_options[] = {
            // name, has_arg, flag, val
            { 0,0,0,0 },
        };

        c = getopt_long(argc, argv, "+p:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch(c) {
        case 'p':
            path = optarg;
            break;
        default:
            fprintf(stderr,"Don't know what option '%c'.\n", c);
            return 1;
        }
    }

    R2FilePath::getInstance()->setWorkingDirectory(argc, (const char**) argv);
    
#ifdef XINETD
    signal(SIGPIPE,SIG_IGN);
    NvInitService();
#endif
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    
    glutInitWindowSize(NanoVis::win_width, NanoVis::win_height);
    
    glutInitWindowPosition(10, 10);
    render_window = glutCreateWindow(argv[0]);
    glutDisplayFunc(NanoVis::display);
    
#ifndef XINETD
    glutMouseFunc(NanoVis::mouse);
    glutMotionFunc(NanoVis::motion);
    glutKeyboardFunc(NanoVis::keyboard);
#endif
    
    glutIdleFunc(idle);
    glutReshapeFunc(NanoVis::resize_offscreen_buffer);
    
    NanoVis::init(path);
    NanoVis::initGL();
    initTcl();
    
#ifdef EVENTLOG
    NvInitEventLog();
#endif
    //event loop
    glutMainLoop();
    
    removeAllData();
    
    NvExit();
    
    return 0;
}

