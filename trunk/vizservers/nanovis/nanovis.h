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
#include "Camera.h"
#include "ConvexPolygon.h"
#include "Texture3D.h"
#include "Texture2D.h"
#include "Texture1D.h"
#include "TransferFunction.h"
#include "Mat4x4.h"
#include "Volume.h"
#include "ParticleSystem.h"
#include "PerfQuery.h"
#include "Event.h"
#include "Lic.h"
#include "VolumeRenderer.h"
#include "PlaneRenderer.h"
#include "NvColorTableRenderer.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "HeightMap.h"
#include "Grid.h"

#include "config.h"

#include <tcl.h>

//defines for the image based flow visualization
#define	NPN 256 	//resolution of background pattern
#define NMESH 256	//resolution of flow mesh
#define DM  ((float) (1.0/(NMESH-1.0)))	//distance in world coords between mesh lines
#define NPIX  512	//display size
#define SCALE 3.0	//scale for background pattern. small value -> fine texture

typedef struct Vector2 {
    float x, y;
    float mag(void) {
	return sqrt(x*x+y*y);
    }
};

typedef struct RegGrid2{
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

class NanoVis {
public:
#ifdef notdef
    static VolumeRenderer* g_vol_render;
    static PointSetRenderer* g_pointset_renderer;
    static NvColorTableRenderer* g_color_table_renderer;

    static vector<PointSet*> g_pointSet;

    static PlaneRenderer* plane_render;
    static Texture2D* plane[10];
#endif
    static graphics::RenderContext* renderContext;
    static vector<HeightMap*> g_heightMap;
    static unsigned char* screen_buffer;
    static vector<Volume*> volume;
    static int n_volumes;
    static int updir;
    static Camera *cam;

    static bool axis_on;

    static int win_width;			//size of the render window
    static int win_height;			//size of the render window

    static TransferFunction* get_transfunc(const char *name);
    static TransferFunction* set_transfunc(const char *name, int nSlots, 
					   float *data);
    static void initGL(void);
    static void init_lic(void);
    static void init_offscreen_buffer(void);
    static void resize_offscreen_buffer(int w, int h);
    static void offscreen_buffer_capture(void);
    static void bmp_write(const char* cmd);
    static void bmp_write_to_file();
    static void display(void);
    static void display_offscreen_buffer();
    static void read_screen();
    static void zoom(double zoom);
    static int render_legend(TransferFunction *tf, double min, double max, 
	int width, int height, const char* volArg);
    static void load_volume(int index, int width, int height, int depth, 
	int n, float* data, double vmin, double vmax, double nzero_min);
#ifndef XINETD
    static void keyboard(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void update_rot(int delta_x, int delta_y);
    static void update_trans(int delta_x, int delta_y, int delta_z);
#endif
};
#endif	/* __NANOVIS_H__ */
