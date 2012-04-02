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

    static void xinetdListen();
    static void init(const char *path);
    static void initGL();
    static void initOffscreenBuffer();
    static void resizeOffscreenBuffer(int w, int h);
    static void displayOffscreenBuffer();
    static void display();
    static void idle();
    static void update();

    static void pan(float dx, float dy);
    static void zoom(float z);

    static void eventuallyRedraw(unsigned int flag = 0);

    static void setVolumeRanges();
    static void setHeightmapRanges();

    static void initParticle();

    static void ppmWrite(const char *prefix);
    static void sendDataToClient(const char *command, const char *data,
                                 size_t dlen);
    static void bmpWrite(const char *prefix);
    static void bmpWriteToFile(int frame_number, const char* directory_name);
 
    static TransferFunction *getTransfunc(const char *name);
    static TransferFunction *defineTransferFunction(const char *name, 
                                                    size_t n, float *data);

    static int render2dContour(HeightMap *heightmap, int width, int height);

    static int renderLegend(TransferFunction *tf, double min, double max, 
                            int width, int height, const char *volArg);

    static Volume *loadVolume(const char *tag, int width, int height, int depth,
                              int n, float* data, double vmin, double vmax, 
                              double nonZeroMin);

    static void removeVolume(Volume *volPtr);

#ifndef XINETD
    static void keyboard(unsigned char key, int x, int y);
    static void mouse(int button, int state, int x, int y);
    static void motion(int x, int y);
    static void updateRot(int delta_x, int delta_y);
    static void updateTrans(int delta_x, int delta_y, int delta_z);
    static void resize(int w, int h);
    static void render();
#endif

    static void readScreen()
    {
        glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, 
                     screenBuffer);
    }
    static void bindOffscreenBuffer()
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _finalFbo);
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
    static bool debugFlag;
    static bool axisOn;
    static int winWidth;	//size of the render window
    static int winHeight;	//size of the render window
    static int renderWindow;
    static unsigned char *screenBuffer;
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

    static VolumeRenderer *volRenderer;
    static NvFlowVisRenderer *flowVisRenderer;
    static VelocityArrowsSlice *velocityArrowsSlice;
    static NvLIC *licRenderer;
    static PlaneRenderer *planeRenderer;
#if PLANE_CMD
    static int numPlanes;
    static Texture2D *plane[]; ///< Pointers to 2D planes
#endif
#ifdef USE_POINTSET_RENDERER
    static PointSetRenderer *pointSetRenderer;
    static std::vector<PointSet *> pointSet;
#endif

    static Tcl_HashTable tfTable;
    static Texture2D *legendTexture;
    static NvColorTableRenderer *colorTableRenderer;

    static std::vector<HeightMap *> heightMap;
    static Tcl_HashTable heightmapTable;

    static Tcl_Interp *interp;
    static Tcl_DString cmdbuffer;

private:
    static float _licSliceX;
    static float _licSliceY;
    static float _licSliceZ;
    static int _licAxis;     /* 0:x, 1:y, 2:z */

    //frame buffer for final rendering
    static GLuint _finalFbo, _finalColorTex, _finalDepthRb;
};

#endif
