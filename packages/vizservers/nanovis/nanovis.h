/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#ifndef NANOVIS_H
#define NANOVIS_H

#include <tcl.h>

#include <GL/glew.h>

#include <math.h>
#include <stddef.h> // For size_t
#include <stdio.h>

#include <vector>
#include <iostream>

#include <rappture.h>

#include "define.h"
#include "global.h"
#include "config.h"

#define NANOVIS_VERSION		"1.0"

//defines for the image based flow visualization
#define	NPN 256 	//resolution of background pattern
#define NMESH 256	//resolution of flow mesh
#define DM  ((float) (1.0/(NMESH-1.0)))	//distance in world coords between mesh lines
#define NPIX  512	//display size
#define SCALE 3.0	//scale for background pattern. small value -> fine texture

namespace graphics {
    class RenderContext;
}

class VolumeRenderer;
class PointSetRenderer;
class NvParticleRenderer;
class NvFlowVisRenderer;
class PlaneRenderer;
class VelocityArrowsSlice;
class NvLIC;
class PointSet;
class Texture2D;
class NvColorTableRenderer;
class HeightMap;
class NvVectorField;
class Grid;
class R2Fonts;
class NvCamera;
class TransferFunction;
class Volume;
class FlowCmd;
class FlowIterator;

struct Vector2 {
    float x, y;
    float mag()
    {
	return sqrt(x*x + y*y);
    }
};

struct RegGrid2 {
    int width, height;
    Vector2 *field;

    RegGrid2(int w, int h)
    {
	width = w;
	height = h;
	field = new Vector2[w*h];
    }

    void put(Vector2& v, int x ,int y)
    {
	field[x+y*width] = v;
    }
    
    Vector2& get(int x, int y)
    {
	return field[x+y*width];
    }
};

class NanoVis
{
public:
    enum NanoVisFlags { 
	REDRAW_PENDING = (1 << 0), 
	MAP_FLOWS = (1 << 1),
	MAP_VOLUMES = (1 << 2),
	MAP_HEIGHTMAPS = (1 << 3),
    };

    static VolumeRenderer *vol_renderer;
    static NvFlowVisRenderer *flowVisRenderer;
    static VelocityArrowsSlice *velocityArrowsSlice;
    static NvLIC *licRenderer;
    static PlaneRenderer *plane_renderer;
#if PLANE_CMD
    static Texture2D *plane[]; ///< Pointers to 2D planes
#endif
#ifdef USE_POINTSET_RENDERER
    static PointSetRenderer *pointset_renderer;
    static std::vector<PointSet *> pointSet;
#endif

    static Texture2D *legendTexture;
    static NvColorTableRenderer *color_table_renderer;

    static graphics::RenderContext *renderContext;
    static std::vector<HeightMap *> heightMap;
    static unsigned char *screen_buffer;
    static Tcl_HashTable volumeTable;
    static Tcl_HashTable heightmapTable;
    static std::vector<NvVectorField *> flow;
    static Grid *grid;
    static R2Fonts *fonts;
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
    static int render_window; 

    static bool debug_flag;

    static Tcl_Interp *interp;
    static Tcl_DString cmdbuffer;

    static TransferFunction *get_transfunc(const char *name);
    static TransferFunction *DefineTransferFunction(const char *name, 
                                                    size_t n, float *data);
    static void SetVolumeRanges();
    static void SetHeightmapRanges();
    static void init(const char *path);
    static void initGL();
    static void init_lic();
    static void init_offscreen_buffer();
    static void initParticle();
    static void resize_offscreen_buffer(int w, int h);

    // For development
    static void resize(int w, int h);
    static void render();

    static void ppm_write(const char *prefix);
    static void sendDataToClient(const char *command, const char *data,
                                 size_t dlen);
    static void bmp_write(const char *prefix);
    static void bmp_write_to_file(int frame_number, const char* directory_name);
    static void display();
    static void idle();
    static void update();
    static void display_offscreen_buffer();
    static int render_legend(TransferFunction *tf, double min, double max, 
                             int width, int height, const char *volArg);
    static Volume *load_volume(const char *tag, int width, int height, 
                               int depth, int n, float* data, double vmin, double vmax, 
                               double nzero_min);
    static void xinetd_listen();
    static int render_2d_contour(HeightMap *heightmap, int width, int height);
    static void pan(float dx, float dy);

#ifndef XINETD
    static void keyboard(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void update_rot(int delta_x, int delta_y);
    static void update_trans(int delta_x, int delta_y, int delta_z);
#endif

    static FILE *stdin, *logfile, *recfile;

    static void read_screen()
    {
        glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, 
                     screen_buffer);
    }
    static void offscreen_buffer_capture()
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _final_fbo);
    }

    static unsigned int flags;
    static Tcl_HashTable flowTable;
    static double magMin, magMax;
    static float xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;
    static float xOrigin, yOrigin, zOrigin;

    static FlowCmd *FirstFlow(FlowIterator *iterPtr);
    static FlowCmd *NextFlow(FlowIterator *iterPtr);
    static void InitFlows();
    static int GetFlow(Tcl_Interp *interp, Tcl_Obj *objPtr,
                       FlowCmd **flowPtrPtr);
    static int CreateFlow(Tcl_Interp *interp, Tcl_Obj *objPtr);
    static void DeleteFlows(Tcl_Interp *interp);
    static bool MapFlows();
    static void RenderFlows();
    static void ResetFlows();
    static bool UpdateFlows();
    static void AdvectFlows();

    static void EventuallyRedraw(unsigned int flag = 0);
    static void remove_volume(Volume *volPtr);
    static Tcl_HashTable tfTable;

private:
    //frame buffer for final rendering
    static GLuint _final_fbo, _final_color_tex, _final_depth_rb;
};

extern Volume *load_volume_stream(Rappture::Outcome &status, const char *tag, 
                                  std::iostream& fin);

extern Volume *load_volume_stream_odx(Rappture::Outcome& status, 
                                      const char *tag, const char *buf, int nBytes);

extern Volume *load_volume_stream2(Rappture::Outcome & status, const char *tag, 
                                   std::iostream& fin);

extern Volume *load_vector_stream(Rappture::Outcome& result, const char *tag, 
                                  size_t length, char *bytes);

extern Volume *load_vector_stream2(Rappture::Outcome& result, const char *tag, 
                                   size_t length, char *bytes);

#endif
