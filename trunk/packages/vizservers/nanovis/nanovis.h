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

#include <math.h>
#include <stddef.h> // For size_t
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h> // For pid_t

#include <tcl.h>

#include <GL/glew.h>

#include <vector>
#include <iostream>
#include <tr1/unordered_map>

#include <vrmath/Vector3f.h>

#include "config.h"
#include "md5.h"

#define NANOVIS_VERSION		"1.2"

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

class Camera;
class Flow;
class Grid;
class HeightMap;
class LIC;
class OrientationIndicator;
class PlaneRenderer;
class Texture2D;
class TransferFunction;
class VelocityArrowsSlice;
class Volume;
class VolumeRenderer;

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

    typedef std::string TransferFunctionId;
    typedef std::string VolumeId;
    typedef std::string FlowId;
    typedef std::string HeightMapId;
    typedef std::tr1::unordered_map<TransferFunctionId, TransferFunction *> TransferFunctionHashmap;
    typedef std::tr1::unordered_map<VolumeId, Volume *> VolumeHashmap;
    typedef std::tr1::unordered_map<FlowId, Flow *> FlowHashmap;
    typedef std::tr1::unordered_map<HeightMapId, HeightMap *> HeightMapHashmap;

    static bool init(const char *path);
    static bool initGL();
    static bool initOffscreenBuffer();
    static bool resizeOffscreenBuffer(int w, int h);
    static void setBgColor(float color[3]);
    static void render();
    static void draw3dAxis();
    static void update();
    static void removeAllData();

    static const Camera *getCamera()
    {
        return cam;
    }
    static void pan(float dx, float dy);
    static void zoom(float z);
    static void resetCamera(bool resetOrientation = false);

    static void eventuallyRedraw(unsigned int flag = 0);

    static void setVolumeRanges();
    static void setHeightmapRanges();
 
    static TransferFunction *getTransferFunction(const TransferFunctionId& id);
    static TransferFunction *defineTransferFunction(const TransferFunctionId& id, 
                                                    size_t n, float *data);

    static int renderLegend(TransferFunction *tf, double min, double max, 
                            int width, int height, const char *volArg);

    static Volume *loadVolume(const char *tag, int width, int height, int depth,
                              int numComponents, float *data, double vmin, double vmax, 
                              double nonZeroMin);

    static void removeVolume(Volume *volume);

    static void readScreen()
    {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, 
                     screenBuffer);
    }
    static void bindOffscreenBuffer()
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _finalFbo);
    }

    static Flow *getFlow(const char *name);
    static Flow *createFlow(Tcl_Interp *interp, const char *name);
    static void deleteFlows(Tcl_Interp *interp);
    static void deleteFlow(const char *name);
    static bool mapFlows();
    static void getFlowBounds(vrmath::Vector3f& min,
                              vrmath::Vector3f& max,
                              bool onlyVisible = false);
    static void renderFlows();
    static void resetFlows();
    static bool updateFlows();
    static void advectFlows();

    static Tcl_Interp *interp;

    static unsigned int flags;
    static bool debugFlag;

    static int winWidth;	///< Width of the render window
    static int winHeight;	///< Height of the render window
    static int renderWindow;    //< GLUT handle for the render window
    static unsigned char *screenBuffer;
    static Texture2D *legendTexture;
    static util::Fonts *fonts;
    static int updir;
    static Camera *cam;
    static graphics::RenderContext *renderContext;

    static TransferFunctionHashmap tfTable; ///< maps transfunc name to TransferFunction object
    static VolumeHashmap volumeTable;
    static FlowHashmap flowTable;
    static HeightMapHashmap heightMapTable;

    static double magMin, magMax;
    static float xMin, xMax, yMin, yMax, zMin, zMax, wMin, wMax;
    static vrmath::Vector3f sceneMin, sceneMax;

    static VolumeRenderer *volRenderer;
    static VelocityArrowsSlice *velocityArrowsSlice;
    static LIC *licRenderer;
    static PlaneRenderer *planeRenderer;
    static OrientationIndicator *orientationIndicator;
    static Grid *grid;

private:
    static void collectBounds(bool onlyVisible = false);

    static float _licSlice;  ///< Slice position [0,1]
    static int _licAxis;     ///< Slice axis: 0:x, 1:y, 2:z

    //frame buffer for final rendering
    static GLuint _finalFbo, _finalColorTex, _finalDepthRb;
};

}

#endif
