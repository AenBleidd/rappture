/*
 * ----------------------------------------------------------------------
 *  nanovis.h: package header
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef __NANOVIS_H__
#define __NANOVIS_H__

#include <tcl.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "define.h"
#include "global.h"
#include "socket/Socket.h"
#include "NvCamera.h"
#include "ConvexPolygon.h"
#include "Texture3D.h"
#include "Texture2D.h"
#include "Texture1D.h"
#include "TransferFunction.h"
#include "Mat4x4.h"
#include "Volume.h"
#include "NvParticleRenderer.h"
#include "PerfQuery.h"
#include "Event.h"
#include "VolumeRenderer.h"
#include "PlaneRenderer.h"
#include "NvColorTableRenderer.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "HeightMap.h"
#include "Grid.h"

#include "config.h"


//defines for the image based flow visualization
#define	NPN 256 	//resolution of background pattern
#define NMESH 256	//resolution of flow mesh
#define DM  ((float) (1.0/(NMESH-1.0)))	//distance in world coords between mesh lines
#define NPIX  512	//display size
#define SCALE 3.0	//scale for background pattern. small value -> fine texture

struct Vector2 {
    float x, y;
    float mag(void) {
	return sqrt(x*x+y*y);
    }
};

struct RegGrid2{
    int width, height;
    Vector2* field;
    
    RegGrid2(int w, int h){
	width = w;
	height = h;
	field = new Vector2[w*h];
    }

    void put(Vector2& v, int x ,int y) {
	field[x+y*width] = v;
    }
    
    Vector2& get(int x, int y) {
	return field[x+y*width];
    }
};

class NvLIC;

class NanoVis {
    //frame buffer for final rendering
    static NVISid final_fbo, final_color_tex, final_depth_rb;
public:
    static VolumeRenderer* vol_renderer;
    static PointSetRenderer* pointset_renderer;
    static NvParticleRenderer* particleRenderer;
    static NvLIC* licRenderer;
    static vector<PointSet*> pointSet;
    static PlaneRenderer* plane_render;

    /**
     *  pointers to 2D planes, currently handle up 10
     */
    static Texture2D* plane[10];
    static NvColorTableRenderer* color_table_renderer;
    static graphics::RenderContext* renderContext;
    static vector<HeightMap*> heightMap;
    static unsigned char* screen_buffer;
    static vector<Volume*> volume;
    static Grid* grid;
    static R2Fonts* fonts;
    static int n_volumes;
    static int updir;
    static NvCamera *cam;

    static float lic_slice_x;
    static float lic_slice_y;
    static float lic_slice_z;
    static int lic_axis;	/* 0:x, 1:y, 2:z */

    static bool axis_on;
    static bool config_pending;	// Indicates if the limits need to be recomputed.
    static int win_width;	//size of the render window
    static int win_height;	//size of the render window
 

    static bool lic_on;
    static bool particle_on;
    static bool vector_on;

    static Tcl_Interp *interp;
    static Tcl_DString cmdbuffer;

    static TransferFunction* get_transfunc(const char *name);
    static TransferFunction* DefineTransferFunction(const char *name, 
	size_t n, float *data);
    static void SetVolumeRanges(void);
    static void SetHeightmapRanges(void);
    static void init(const char* path);
    static void initGL(void);
    static void init_lic(void);
    static void init_offscreen_buffer(void);
    static void initParticle();
    static void resize_offscreen_buffer(int w, int h);
    static void ppm_write(const char *prefix);
    static void bmp_write(const char *prefix);
    static void bmp_write_to_file(int frame_number, const char* directory_name);
    static void display(void);
    static void update(void);
    static void display_offscreen_buffer();
    static void zoom(double zoom);
    static int render_legend(TransferFunction *tf, double min, double max, 
	int width, int height, const char* volArg);
    static Volume *load_volume(int index, int width, int height, int depth, 
	int n, float* data, double vmin, double vmax, double nzero_min);
    static void xinetd_listen(void);
    static int render_2d_contour(HeightMap* heightmap, int width, int height);

#ifndef XINETD
    static void keyboard(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void update_rot(int delta_x, int delta_y);
    static void update_trans(int delta_x, int delta_y, int delta_z);
#endif

    static FILE *stdin, *logfile;

    static void read_screen(void) {
	glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, 
		     screen_buffer);
    }
    static void offscreen_buffer_capture(void) {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
    }
};


#endif	/* __NANOVIS_H__ */
