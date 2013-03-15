/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 *  nanovis.h: package header
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef NANOVIS_H
#define NANOVIS_H

#include <tcl.h>
#include <md5.h>
#include <GL/glew.h>

#include <math.h>
#include <stddef.h> // For size_t
#include <stdio.h>

#include <vector>
#include <iostream>

#include <vrmath/Vector3f.h>

#include "config.h"

#define NANOVIS_VERSION		"1.1"

//defines for the image based flow visualization
#define NMESH 256	//resolution of flow mesh
#define NPIX  512	//display size

namespace nv {
namespace graphics {
    class RenderContext;
}
namespace util {
    class Fonts;
}
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
class NvCamera;
class TransferFunction;
class Volume;
class FlowCmd;
class FlowIterator;

class NanoVis
{
public:
    enum AxisDirections { 
        X_POS = 1,
        Y_POS = 2,
        Z_POS = 3,
        X_NEG = -1,
        Y_NEG = -2,
        Z_NEG = -3
    };

    enum NanoVisFlags { 
	REDRAW_PENDING = (1 << 0), 
	MAP_FLOWS = (1 << 1)
    };

    static void processCommands();
    static void init(const char *path);
    static void initGL();
    static void initOffscreenBuffer();
    static void resizeOffscreenBuffer(int w, int h);
    static void setBgColor(float color[3]);
    static void render();
    static void draw3dAxis();
    static void idle();
    static void update();
    static void removeAllData();

    static const NvCamera *getCamera()
    {
        return cam;
    }
    static void pan(float dx, float dy);
    static void zoom(float z);
    static void resetCamera(bool resetOrientation = false);

    static void eventuallyRedraw(unsigned int flag = 0);

    static void setVolumeRanges();
    static void setHeightmapRanges();

#ifdef KEEPSTATS
    static int getStatsFile(Tcl_Obj *objPtr);
    static int writeToStatsFile(int f, const char *s, size_t length);
#endif
    static void ppmWrite(const char *prefix);
    static void sendDataToClient(const char *command, const char *data,
                                 size_t dlen);
    static void bmpWrite(const char *prefix);
    static void bmpWriteToFile(int frame_number, const char* directory_name);
 
    static TransferFunction *getTransfunc(const char *name);
    static TransferFunction *defineTransferFunction(const char *name, 
                                                    size_t n, float *data);

    static int renderLegend(TransferFunction *tf, double min, double max, 
                            int width, int height, const char *volArg);

    static Volume *loadVolume(const char *tag, int width, int height, int depth,
                              int n, float* data, double vmin, double vmax, 
                              double nonZeroMin);

    static void removeVolume(Volume *volPtr);

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
    static void GetFlowBounds(vrmath::Vector3f& min,
                              vrmath::Vector3f& max,
                              bool onlyVisible = false);
    static void RenderFlows();
    static void ResetFlows();
    static bool UpdateFlows();
    static void AdvectFlows();

    static FILE *stdin, *logfile, *recfile;
    static int statsFile;
    static unsigned int flags;
    static bool debugFlag;
    static bool axisOn;
    static int winWidth;	//size of the render window
    static int winHeight;	//size of the render window
    static int renderWindow;
    static unsigned char *screenBuffer;
    static Texture2D *legendTexture;
    static Grid *grid;
    static nv::util::Fonts *fonts;
    static int updir;
    static NvCamera *cam;
    static nv::graphics::RenderContext *renderContext;

    static Tcl_HashTable tfTable;
    static Tcl_HashTable volumeTable;
    static Tcl_HashTable flowTable;
    static Tcl_HashTable heightmapTable;

    static double magMin, magMax;
    static float xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;
    static vrmath::Vector3f sceneMin, sceneMax;

    static NvColorTableRenderer *colorTableRenderer;
    static VolumeRenderer *volRenderer;
#ifdef notdef
    static NvFlowVisRenderer *flowVisRenderer;
#endif
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

    static Tcl_Interp *interp;
    static struct timeval startTime;           /* Start of elapsed time. */
private:
    static void collectBounds(bool onlyVisible = false);

    static float _licSlice;  ///< Slice position [0,1]
    static int _licAxis;     ///< Slice axis: 0:x, 1:y, 2:z

    //frame buffer for final rendering
    static GLuint _finalFbo, _finalColorTex, _finalDepthRb;
};

#endif
