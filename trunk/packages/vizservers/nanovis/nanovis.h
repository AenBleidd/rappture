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

#include "config.h"

#define NANOVIS_VERSION		"1.0"

//defines for the image based flow visualization
#define NMESH 256	//resolution of flow mesh
#define NPIX  512	//display size

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

class NanoVis
{
public:
    enum NanoVisFlags { 
	REDRAW_PENDING = (1 << 0), 
	MAP_FLOWS = (1 << 1),
	MAP_VOLUMES = (1 << 2),
	MAP_HEIGHTMAPS = (1 << 3),
    };

    static void init(const char *path);
    static void xinetd_listen();
    static void initGL();
    static void init_offscreen_buffer();
    static void resize_offscreen_buffer(int w, int h);
    static void resize(int w, int h);
    static void render();
    static void display();
    static void idle();
    static void update();
    static void display_offscreen_buffer();
    static void pan(float dx, float dy);
    static void zoom(float z);

    static void EventuallyRedraw(unsigned int flag = 0);

    static void SetVolumeRanges();
    static void SetHeightmapRanges();

    static void init_lic();
    static void initParticle();

    static void ppm_write(const char *prefix);
    static void sendDataToClient(const char *command, const char *data,
                                 size_t dlen);
    static void bmp_write(const char *prefix);
    static void bmp_write_to_file(int frame_number, const char* directory_name);
 
    static TransferFunction *get_transfunc(const char *name);
    static TransferFunction *DefineTransferFunction(const char *name, 
                                                    size_t n, float *data);

    static int render_2d_contour(HeightMap *heightmap, int width, int height);

    static int render_legend(TransferFunction *tf, double min, double max, 
                             int width, int height, const char *volArg);

    static Volume *load_volume(const char *tag, int width, int height, 
                               int depth, int n, float* data, double vmin, double vmax, 
                               double nzero_min);
    static void remove_volume(Volume *volPtr);

#ifndef XINETD
    static void keyboard(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void update_rot(int delta_x, int delta_y);
    static void update_trans(int delta_x, int delta_y, int delta_z);
#endif

    static void read_screen()
    {
        glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, 
                     screen_buffer);
    }
    static void offscreen_buffer_capture()
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _final_fbo);
    }

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

    static FILE *stdin, *logfile, *recfile;

    static unsigned int flags;
    static bool debug_flag;
    static bool axis_on;
    static bool config_pending;	// Indicates if the limits need to be recomputed.
    static int win_width;	//size of the render window
    static int win_height;	//size of the render window
    static int render_window;
    static unsigned char *screen_buffer;
    static Grid *grid;
    static R2Fonts *fonts;
    static int updir;
    static NvCamera *cam;
    static graphics::RenderContext *renderContext;

    static Tcl_HashTable volumeTable;

    static std::vector<NvVectorField *> flow;
    static Tcl_HashTable flowTable;
    static double magMin, magMax;
    static float xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;
    static float xOrigin, yOrigin, zOrigin;

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

    static Tcl_HashTable tfTable;
    static Texture2D *legendTexture;
    static NvColorTableRenderer *color_table_renderer;

    static std::vector<HeightMap *> heightMap;
    static Tcl_HashTable heightmapTable;

    static float lic_slice_x;
    static float lic_slice_y;
    static float lic_slice_z;
    static int lic_axis;	/* 0:x, 1:y, 2:z */

    static Tcl_Interp *interp;
    static Tcl_DString cmdbuffer;

private:
    //frame buffer for final rendering
    static GLuint _final_fbo, _final_color_tex, _final_depth_rb;
};

#endif
