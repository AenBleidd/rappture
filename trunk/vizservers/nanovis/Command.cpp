
/*
 * ----------------------------------------------------------------------
 * Command.cpp
 *
 *	This modules creates the Tcl interface to the nanovis server.
 *	The communication protocol of the server is the Tcl language.
 *	Commands given to the server by clients are executed in a
 *	safe interpreter and the resulting image rendered offscreen
 *	is returned as BMP-formatted image data.
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

/*
 * TODO:  In no particular order...
 *	  o Convert to Tcl_CmdObj interface.
 *	  o Use Tcl command option parser to reduce size of procedures, remove
 *	    lots of extra error checking code.
 *	  o Convert GetVolumeIndices to GetVolumes.  Goal is to remove
 *	    all references of Nanovis::volume[] from this file.  Don't 
 *	    want to know how volumes are stored. Same for heightmaps.
 *	  o Rationalize volume id scheme. Right now it's the index in 
 *	    the vector. 1) Use a list instead of a vector. 2) carry
 *	    an id field that's a number that gets incremented each new volume.
 *	  o Create R2, matrix, etc. libraries.
 */

#include "Command.h"
#include "Trace.h"

#include "nanovis.h"

#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"
#include "RpEncode.h"

#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"

#include "Nv.h"
#include "PointSetRenderer.h"
#include "PointSet.h"
#include "ZincBlendeVolume.h"
#include "NvLoadFile.h"
#include "NvColorTableRenderer.h"
#include "NvEventLog.h"
#include "NvZincBlendeReconstructor.h"
#include "HeightMap.h"
#include "Grid.h"
#include "NvCamera.h"
#include <RenderContext.h>
#include <NvLIC.h>

// FOR testing new functions
//#define  _LOCAL_ZINC_TEST_
#ifdef _LOCAL_ZINC_TEXT_
#include "Test.h"
#endif
// EXTERN DECLARATIONS
// in Nv.cpp

extern Grid* NanoVis::grid;

// in nanovis.cpp
extern vector<PointSet*> g_pointSet;

extern float live_rot_x;		//object rotation angles
extern float live_rot_y;
extern float live_rot_z;
extern float live_obj_x;	//object translation location from the origin
extern float live_obj_y;
extern float live_obj_z;

extern PlaneRenderer* plane_render;
extern Texture2D* plane[10];

extern Rappture::Outcome load_volume_stream(int index, std::iostream& fin);
extern Rappture::Outcome load_volume_stream2(int index, std::iostream& fin);
extern void load_volume(int index, int width, int height, int depth, 
	int n_component, float* data, double vmin, double vmax, 
	double nzero_min);
extern void load_vector_stream(int index, std::istream& fin);

// Tcl interpreter for incoming messages
static Tcl_Interp *interp;
static Tcl_DString cmdbuffer;

// default transfer function
static const char def_transfunc[] = "transfunc define default {\n\
  0.0  1 1 1\n\
  0.2  1 1 0\n\
  0.4  0 1 0\n\
  0.6  0 1 1\n\
  0.8  0 0 1\n\
  1.0  1 0 1\n\
} {\n\
  0.00  1.0\n\
  0.05  0.0\n\
  0.15  0.0\n\
  0.20  1.0\n\
  0.25  0.0\n\
  0.35  0.0\n\
  0.40  1.0\n\
  0.45  0.0\n\
  0.55  0.0\n\
  0.60  1.0\n\
  0.65  0.0\n\
  0.75  0.0\n\
  0.80  1.0\n\
  0.85  0.0\n\
  0.95  0.0\n\
  1.00  1.0\n\
}";

static Tcl_CmdProc AxisCmd;
static Tcl_CmdProc CameraCmd;
static Tcl_CmdProc CutplaneCmd;
static Tcl_CmdProc GridCmd;
static Tcl_CmdProc LegendCmd;
static Tcl_CmdProc PlaneEnableCmd;
static Tcl_CmdProc PlaneLinkCmd;
static Tcl_CmdProc PlaneNewCmd;
static Tcl_CmdProc ScreenCmd;
static Tcl_CmdProc ScreenShotCmd;
static Tcl_CmdProc TransfuncCmd;
static Tcl_CmdProc UniRect2dCmd;
static Tcl_CmdProc UpCmd;
static Tcl_CmdProc VolumeCmd;
static Tcl_CmdProc FlowCmd;

static int GetVolumeIndices(Tcl_Interp *interp, int argc, const char *argv[],
	vector<unsigned int>* vectorPtr);
static int GetVolumes(Tcl_Interp *interp, int argc, const char *argv[],
	vector<Volume *>* vectorPtr);
static int GetVolume(Tcl_Interp *interp, const char *string, 
	Volume **volPtrPtr);
static int GetVolumeIndex(Tcl_Interp *interp, const char *string, 
	unsigned int *indexPtr);
static int GetHeightMap(Tcl_Interp *interp, const char *string, 
	HeightMap **hmPtrPtr);
static int GetIndices(Tcl_Interp *interp, int argc, const char *argv[], 
	vector<unsigned int>* vectorPtr);
static int GetAxis(Tcl_Interp *interp, const char *string, int *valPtr);
static int GetColor(Tcl_Interp *interp, const char *string, float *rgbPtr);
static int GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, 
	int nBytes);
static HeightMap *CreateHeightMap(ClientData clientData, Tcl_Interp *interp, 
	int argc, const char *argv[]);

static int
GetFloat(Tcl_Interp *interp, const char *string, float *valuePtr)
{
    double value;

    if (Tcl_GetDouble(interp, string, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    *valuePtr = (float)value;
    return TCL_OK;
}

static int
GetCullMode(Tcl_Interp *interp, const char *string, 
	    graphics::RenderContext::CullMode *modePtr)
{
    if (strcmp(string, "none") == 0) {
	*modePtr = graphics::RenderContext::NO_CULL;
    } else if (strcmp(string, "front") == 0) {
	*modePtr = graphics::RenderContext::FRONT;
    } else if (strcmp(string, "back") == 0) {
	*modePtr = graphics::RenderContext::BACK;
    } else {
	Tcl_AppendResult(interp, "invalid cull mode \"", string, 
		"\": should be front, back, or none\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetShadingModel(Tcl_Interp *interp, const char *string, 
		graphics::RenderContext::ShadingModel *modelPtr)
{
    if (strcmp(string,"flat") == 0) {
	*modelPtr = graphics::RenderContext::FLAT;
    } else if (strcmp(string,"smooth") == 0) {
	*modelPtr = graphics::RenderContext::SMOOTH;
    } else {
	Tcl_AppendResult(interp, "bad shading model \"", string, 
			 "\": should be flat or smooth", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetPolygonMode(Tcl_Interp *interp, const char *string, 
	       graphics::RenderContext::PolygonMode *modePtr)
{
    if (strcmp(string,"wireframe") == 0) {
	*modePtr = graphics::RenderContext::LINE;
    } else if (strcmp(string,"fill") == 0) {
	*modePtr = graphics::RenderContext::FILL;
    } else {
	Tcl_AppendResult(interp, "invalid polygon mode \"", string, 
	"\": should be wireframe or fill\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   camera aim <x0> <y0> <z0>
 *   camera angle <xAngle> <yAngle> <zAngle>
 *   camera zoom <factor>
 *
 * Clients send these commands to manipulate the camera.  The "angle"
 * option controls the angle of the camera around the focal point.
 * The "zoom" option sets the zoom factor, moving the camera in
 * and out.
 * ----------------------------------------------------------------------
 */
static int
CameraCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " option arg arg...\"", (char*)NULL);
	return TCL_ERROR;
    }

    char c = argv[1][0];
    if ((c == 'a') && (strcmp(argv[1],"angle") == 0)) {
        if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " angle xangle yangle zangle\"", (char*)NULL);
	    return TCL_ERROR;
        }
        double xangle, yangle, zangle;
	if ((Tcl_GetDouble(interp, argv[2], &xangle) != TCL_OK) || 
	    (Tcl_GetDouble(interp, argv[3], &yangle) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[4], &zangle) != TCL_OK)) {
	    return TCL_ERROR;
	}
	NanoVis::cam->rotate(xangle, yangle, zangle);
    } else if ((c == 'a') && (strcmp(argv[1], "aim") == 0)) {
        if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " aim x y z\"", (char*)NULL);
	    return TCL_ERROR;
        }
	
        double x0, y0, z0;
	if ((Tcl_GetDouble(interp, argv[2], &x0) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[3], &y0) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[4], &z0) != TCL_OK)) {
	    return TCL_ERROR;
	}
	NanoVis::cam->aim(x0, y0, z0);
    } else if ((c == 'z') && (strcmp(argv[1],"zoom") == 0)) {
        if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " zoom factor\"", (char*)NULL);
	    return TCL_ERROR;
        }
	
        double zoom;
	if (Tcl_GetDouble(interp, argv[2], &zoom) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::zoom(zoom);
    } else {
	Tcl_AppendResult(interp, "bad camera option \"", argv[1],
		     "\": should be aim, angle, or zoom", (char*)NULL);
	return TCL_ERROR;
    }

    /*
    Trace("Camera pos(%f %f %f), rot(%f %f %f), aim(%f %f %f)\n", 
            NanoVis::cam->location.x, NanoVis::cam->location.y, NanoVis::cam->location.z,
            NanoVis::cam->angle.x, NanoVis::cam->angle.y, NanoVis::cam->angle.z,
            NanoVis::cam->target.x, NanoVis::cam->target.y, NanoVis::cam->target.z);

    NanoVis::cam->aim(0, 0, 100);
    NanoVis::cam->rotate(-51.868206, 88.637497, 0.000000);
    NanoVis::cam->move(-0.000000, -0.000000, -0.819200);
    */

    return TCL_OK;
}

static int
ScreenShotCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	      const char *argv[])
{
#ifdef XINETD
    NanoVis::resize_offscreen_buffer(1024, 1024);
    NanoVis::cam->set_screen_size(30, 90, 1024 - 60, 1024 - 120);
    NanoVis::offscreen_buffer_capture();  //enable offscreen render
    NanoVis::display();

    // INSOO
    // TBD
    /*
    Volume* vol = NanoVis::volume[0];
    TransferFunction* tf;
    tf = NanoVis::vol_renderer->get_volume_shading(vol);
    if (tf != NULL) 
    {
        float data[512];

        for (int i=0; i < 256; i++) {
            data[i] = data[i+256] = (float)(i/255.0);
        }
        Texture2D* plane = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
        NanoVis::color_table_renderer->render(1024, 1024, plane, tf, vol->range_min(),
		vol->range_max());
        delete plane;
    
    }
    */
#endif

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   cutplane state on|off <axis> ?<volume>...?
 *   cutplane position <relvalue> <axis> ?<volume>...?
 *
 * Clients send these commands to manipulate the cutplanes in one or
 * more data volumes.  The "state" command turns a cutplane on or
 * off.  The "position" command changes the position to a relative
 * value in the range 0-1.  The <axis> can be x, y, or z.  These
 * options are applied to the volumes represented by one or more
 * <volume> indices.  If no volumes are specified, then all volumes
 * are updated.
 * ----------------------------------------------------------------------
 */
static int
CutplaneCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	    const char *argv[])
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option ?arg arg...?\"", (char*)NULL);
        return TCL_ERROR;
    }

    char c = argv[1][0];
    if ((c == 's') && (strcmp(argv[1],"state") == 0)) {
        if (argc < 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " state on|off axis ?volume ...? \"", (char*)NULL);
            return TCL_ERROR;
        }

        int state;
        if (Tcl_GetBoolean(interp, argv[2], &state) != TCL_OK) {
            return TCL_ERROR;
        }

        int axis;
        if (GetAxis(interp, (char*) argv[3], &axis) != TCL_OK) {
            return TCL_ERROR;
        }

        vector<Volume *> ivol;
        if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }
	if (state) {
	    vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->enable_cutplane(axis);
	    } 
	} else {
	    vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->disable_cutplane(axis);
	    } 
        }
    } else if ((c == 'p') && (strcmp(argv[1],"position") == 0)) {
        if (argc < 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " position relval axis ?volume ...? \"", (char*)NULL);
            return TCL_ERROR;
        }

        float relval;
        if (GetFloat(interp, argv[2], &relval) != TCL_OK) {
            return TCL_ERROR;
        }

        // keep this just inside the volume so it doesn't disappear
        if (relval < 0.01f) { 
	    relval = 0.01f; 
	} else if (relval > 0.99f) { 
	    relval = 0.99f; 
	}

        int axis;
        if (GetAxis(interp, argv[3], &axis) != TCL_OK) {
            return TCL_ERROR;
        }

        vector<Volume *> ivol;
        if (GetVolumes(interp, argc - 4, argv + 4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }
	vector<Volume *>::iterator iter;
	for (iter = ivol.begin(); iter != ivol.end(); iter++) {
            (*iter)->move_cutplane(axis, relval);
        }
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
			 "\": should be position or state", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   legend <volumeIndex> <width> <height>
 *
 * Clients use this to generate a legend image for the specified
 * transfer function.  The legend image is a color gradient from 0
 * to one, drawn in the given transfer function.  The resulting image
 * is returned in the size <width> x <height>.
 * ----------------------------------------------------------------------
 */
static int
LegendCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " transfunc width height\"", (char*)NULL);
        return TCL_ERROR;
    }

    Volume *volPtr;
    if (GetVolume(interp, argv[1], &volPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    TransferFunction *tf;
    tf = NanoVis::vol_renderer->get_volume_shading(volPtr);
    if (tf == NULL) {
        Tcl_AppendResult(interp, "no transfer function defined for volume \"", 
		argv[1], "\"", (char*)NULL);
        return TCL_ERROR;
    }

    int width, height;
    if ((Tcl_GetInt(interp, argv[2], &width) != TCL_OK) ||
	(Tcl_GetInt(interp, argv[3], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    NanoVis::render_legend(tf, volPtr->range_min(), volPtr->range_max(), 
	width, height, argv[1]);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   screen <width> <height>
 *
 * Clients send this command to set the size of the rendering area.
 * Future images are generated at the specified width/height.
 * ----------------------------------------------------------------------
 */
static int
ScreenCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    int w, h;

    if (argc != 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " width height\"", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &w) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &h) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::resize_offscreen_buffer(w, h);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   transfunc define <name> <colormap> <alphamap>
 *     where <colormap> = { <v> <r> <g> <b> ... }
 *           <alphamap> = { <v> <w> ... }
 *
 * Clients send these commands to manipulate the transfer functions.
 * ----------------------------------------------------------------------
 */
static int
TransfuncCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	     const char *argv[])
{
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " option arg arg...\"", (char*)NULL);
	return TCL_ERROR;
    }

    char c = argv[1][0];
    if ((c == 'd') && (strcmp(argv[1],"define") == 0)) {
        if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" define name colormap alphamap\"", (char*)NULL);
            return TCL_ERROR;
        }

        // decode the data and store in a series of fields
        Rappture::Field1D rFunc, gFunc, bFunc, wFunc;
        int cmapc, wmapc, i;
        const char **cmapv;
	const char **wmapv;

	wmapv = cmapv = NULL;
        if (Tcl_SplitList(interp, argv[3], &cmapc, &cmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((cmapc % 4) != 0) {
	    Tcl_AppendResult(interp, "bad colormap in transfunc: should be ",
                "{ v r g b ... }", (char*)NULL);
	    Tcl_Free((char*)cmapv);
	    return TCL_ERROR;
        }

        if (Tcl_SplitList(interp, argv[4], &wmapc, &wmapv) != TCL_OK) {
	    Tcl_Free((char*)cmapv);
	    return TCL_ERROR;
        }
        if ((wmapc % 2) != 0) {
	    Tcl_AppendResult(interp, "wrong # elements in alphamap: should be ",
			" { v w ... }", (char*)NULL);
	    Tcl_Free((char*)cmapv);
	    Tcl_Free((char*)wmapv);
	    return TCL_ERROR;
        }
        for (i=0; i < cmapc; i += 4) {
	    int j;
            double vals[4];

            for (j=0; j < 4; j++) {
                if (Tcl_GetDouble(interp, cmapv[i+j], &vals[j]) != TCL_OK) {
		    Tcl_Free((char*)cmapv);
		    Tcl_Free((char*)wmapv);
		    return TCL_ERROR;
                }
                if ((vals[j] < 0.0) || (vals[j] > 1.0)) {
		    Tcl_AppendResult(interp, "bad value \"", cmapv[i+j],
                        "\": should be in the range 0-1", (char*)NULL);
		    Tcl_Free((char*)cmapv);
		    Tcl_Free((char*)wmapv);
		    return TCL_ERROR;
                }
            }
            rFunc.define(vals[0], vals[1]);
            gFunc.define(vals[0], vals[2]);
            bFunc.define(vals[0], vals[3]);
        }
        for (i=0; i < wmapc; i += 2) {
            double vals[2];
	    int j;

            for (j=0; j < 2; j++) {
                if (Tcl_GetDouble(interp, wmapv[i+j], &vals[j]) != TCL_OK) {
		    Tcl_Free((char*)cmapv);
		    Tcl_Free((char*)wmapv);
		    return TCL_ERROR;
                }
                if ((vals[j] < 0.0) || (vals[j] > 1.0)) {
		    Tcl_AppendResult(interp, "bad value \"", wmapv[i+j],
                        "\": should be in the range 0-1", (char*)NULL);
		    Tcl_Free((char*)cmapv);
		    Tcl_Free((char*)wmapv);
		    return TCL_ERROR;
                }
            }
            wFunc.define(vals[0], vals[1]);
        }
        Tcl_Free((char*)cmapv);
        Tcl_Free((char*)wmapv);

        // sample the given function into discrete slots
        const int nslots = 256;
        float data[4*nslots];
        for (i=0; i < nslots; i++) {
            double xval = double(i)/(nslots-1);
            data[4*i]   = rFunc.value(xval);
            data[4*i+1] = gFunc.value(xval);
            data[4*i+2] = bFunc.value(xval);
            data[4*i+3] = wFunc.value(xval);
        }

        // find or create this transfer function
        TransferFunction *tf;
	tf = NanoVis::get_transfunc(argv[2]);
	if (tf != NULL) {
	    tf->update(data);
	} else {
	    tf = NanoVis::set_transfunc(argv[2], nslots, data);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be define", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   up axis
 *
 * Clients use this to set the "up" direction for all volumes.  Volumes
 * are oriented such that this direction points upward.
 * ----------------------------------------------------------------------
 */
static int
UpCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " x|y|z|-x|-y|-z\"", (char*)NULL);
        return TCL_ERROR;
    }

    int sign = 1;

    char *axisName = (char*)argv[1];
    if (*axisName == '-') {
        sign = -1;
        axisName++;
    }

    int axis;
    if (GetAxis(interp, axisName, &axis) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::updir = (axis+1)*sign;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   volume axis label x|y|z <value> ?<volumeId> ...?
 *   volume data state on|off ?<volumeId> ...?
 *   volume outline state on|off ?<volumeId> ...?
 *   volume outline color on|off ?<volumeId> ...?
 *   volume shading transfunc <name> ?<volumeId> ...?
 *   volume shading diffuse <value> ?<volumeId> ...?
 *   volume shading specular <value> ?<volumeId> ...?
 *   volume shading opacity <value> ?<volumeId> ...?
 *   volume state on|off ?<volumeId> ...?
 *
 * Clients send these commands to manipulate the volumes.
 * ----------------------------------------------------------------------
 */
static int
VolumeCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }

    char c = argv[1][0];
    if ((c == 'a') && (strcmp(argv[1],"axis") == 0)) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " axis option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = argv[2][0];
        if ((c == 'l') && (strcmp(argv[2],"label") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " axis label x|y|z string ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
            int axis;
            if (GetAxis(interp, (char*)argv[3], &axis) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc - 5, argv + 5, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
	    vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_label(axis, argv[4]);
            }
        } else {
	    Tcl_AppendResult(interp, "bad option \"", argv[2],
		"\": should be label", (char*)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 'd') && (strcmp(argv[1],"data") == 0)) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " data option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = argv[2][0];
        if ((c == 's') && (strcmp(argv[2],"state") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " data state on|off ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
            int state;
            if (Tcl_GetBoolean(interp, argv[3], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
	    if (state) {
		vector<Volume *>::iterator iter;
		for (iter = ivol.begin(); iter != ivol.end(); iter++) {
		    (*iter)->enable_data();
		}
	    } else {
		vector<Volume *>::iterator iter;
		for (iter = ivol.begin(); iter != ivol.end(); iter++) {
		    (*iter)->disable_data();
		}
	    }
        } else if (c == 'f' && strcmp(argv[2], "follows") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " data follows size", (char*)NULL);
                return TCL_ERROR;
            }
            printf("Data Loading\n");
            fflush(stdout);

            int nbytes;
            if (Tcl_GetInt(interp, argv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }

	    Rappture::Buffer buf;
	    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
		return TCL_ERROR;
	    }
            int n = NanoVis::n_volumes;
            char header[6];
            memcpy(header, buf.bytes(), sizeof(char) * 5);
            header[5] = '\0';

#ifdef _LOCAL_ZINC_TEST_
            //FILE* fp = fopen("/home/nanohub/vrinside/nv/data/HOON/QDWL_100_100_50_strain_8000i.nd_zatom_12_1", "rb");
            FILE* fp;

	    fp = fopen("/home/nanohub/vrinside/nv/data/HOON/GaAs_AlGaAs_2QD_B4.nd_zc_1_wf", "rb");
            if (fp == NULL) {
                printf("cannot open the file\n");
                fflush(stdout);
                return TCL_ERROR;
            }
            unsigned char* b = (unsigned char*)malloc(buf.size());
            fread(b, buf.size(), 1, fp);
            fclose(fp);
#endif	/*_LOCAL_ZINC_TEST_*/
            printf("Checking header[%s]\n", header);
            fflush(stdout);
            if (strcmp(header, "<HDR>") == 0) {
                Volume* vol = NULL;

                printf("ZincBlende stream is in\n");
                fflush(stdout);
                //std::stringstream fdata(std::ios_base::out|std::ios_base::in|std::ios_base::binary);
                //fdata.write(buf.bytes(),buf.size());
                //vol = NvZincBlendeReconstructor::getInstance()->loadFromStream(fdata);
                
#ifdef _LOCAL_ZINC_TEST_
                vol = NvZincBlendeReconstructor::getInstance()->loadFromMemory(b);
#else
                vol = NvZincBlendeReconstructor::getInstance()->loadFromMemory((void*) buf.bytes());
#endif	/*_LOCAL_ZINC_TEST_*/

                printf("finish loading\n");
                fflush(stdout);
                if (vol) {
                    while (NanoVis::n_volumes <= n) {
                        NanoVis::volume.push_back((Volume*) NULL);
                        NanoVis::n_volumes++;
                    }

                    if (NanoVis::volume[n] != NULL) {
                        delete NanoVis::volume[n];
                        NanoVis::volume[n] = NULL;
                    }

                    float dx0 = -0.5;
                    float dy0 = -0.5*vol->height/vol->width;
                    float dz0 = -0.5*vol->depth/vol->width;
                    vol->move(Vector3(dx0, dy0, dz0));

                    NanoVis::volume[n] = vol;
                }
#ifdef __TEST_CODE__
/*
            } else if (strcmp(header, "<FET>") == 0) {
                printf("FET loading...\n");
                fflush(stdout);
                std::stringstream fdata;
                fdata.write(buf.bytes(),buf.size());
                err = load_volume_stream3(n, fdata);

                if (err) {
                    Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
                    return TCL_ERROR;
                }
*/
#endif	/*__TEST_CODE__*/
            } else {
		Rappture::Outcome err;

                printf("OpenDX loading...\n");
                fflush(stdout);
                std::stringstream fdata;
                fdata.write(buf.bytes(),buf.size());
                err = load_volume_stream(n, fdata);
                //err = load_volume_stream2(n, fdata);
                if (err) {
                    Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
                    return TCL_ERROR;
                }
            }

            //
            // BE CAREFUL:  Set the number of slices to something
            //   slightly different for each volume.  If we have
            //   identical volumes at exactly the same position
            //   with exactly the same number of slices, the second
            //   volume will overwrite the first, so the first won't
            //   appear at all.
            //
            if (NanoVis::volume[n] != NULL) {
                NanoVis::volume[n]->set_n_slice(256-n);
                NanoVis::volume[n]->disable_cutplane(0);
                NanoVis::volume[n]->disable_cutplane(1);
                NanoVis::volume[n]->disable_cutplane(2);

                NanoVis::vol_renderer->add_volume(NanoVis::volume[n],NanoVis::get_transfunc("default"));
            }

	    {
		Volume *volPtr;
		char info[1024];
		float vmin, vmax, min, max;

		volPtr = NanoVis::volume[n];
		vmin = min = volPtr->range_min();
		vmax = max = volPtr->range_max();
		for (int i = 0; i < NanoVis::n_volumes; i++) {
		    if (NanoVis::volume[i] == NULL) {
			continue;
		    }
		    volPtr = NanoVis::volume[i];
		    if (vmin > volPtr->range_min()) {
			vmin = volPtr->range_min();
		    }
		    if (vmax < volPtr->range_max()) {
			vmax = volPtr->range_max();
		    }
		}
		sprintf(info, "nv>data id %d min %g max %g vmin %g vmax %g\n", 
			n, min, max, vmin, vmax);
		write(0, info, strlen(info));
	    }
        } else {
	    Tcl_AppendResult(interp, "bad data option \"", argv[2],
		"\": should be follows or state", (char*)NULL);
	    return TCL_ERROR;
	}
    } else if (c == 'o' && strcmp(argv[1],"outline") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " outline option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = argv[2][0];
        if ((c == 's') && (strcmp(argv[2],"state") == 0)) {
            if (argc < 3) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " outline state on|off ?volume ...? \"", (char*)NULL);
                return TCL_ERROR;
            }

            int state;
            if (Tcl_GetBoolean(interp, argv[3], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
	    if (state) {
		vector<Volume *>::iterator iter;
		for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                    (*iter)->enable_outline();
		}
	    } else {
		vector<Volume *>::iterator iter;
		for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                    (*iter)->disable_outline();
		}
            }
        } else if (c == 'v' && strcmp(argv[2],"visible") == 0) {
	    int ivisible;

	    if (Tcl_GetBoolean(interp, argv[3], &ivisible) != TCL_OK) {
		return TCL_ERROR;
	    }		
            if (!ivisible) {
                for (int i = 0; i < NanoVis::n_volumes; ++i) {
                    if (NanoVis::volume[i]) {
			NanoVis::volume[i]->disable_outline();
		    }
                }
            } else {
                for (int i = 0; i < NanoVis::n_volumes; ++i) {
                    if (NanoVis::volume[i]) {
			NanoVis::volume[i]->enable_outline();
		    }
                }
            }
        } else if ((c == 'c') && (strcmp(argv[2],"color") == 0)) {
            if (argc < 3) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " outline color {R G B} ?volume ...? \"", (char*)NULL);
                return TCL_ERROR;
            }
            float rgb[3];
            if (GetColor(interp, argv[3], rgb) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc - 4, argv + 4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *>::iterator iter;
            for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_outline_color(rgb);
            }
        } 
        else {
	    Tcl_AppendResult(interp, "bad outline option \"", argv[2],
		"\": should be color, visible, or state", (char*)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 's') && (strcmp(argv[1],"shading") == 0)) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " shading option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = argv[2][0];
        if ((c == 't') && (strcmp(argv[2],"transfunc") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " shading transfunc name ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
            TransferFunction *tf;
	    tf = NanoVis::get_transfunc(argv[3]);
            if (tf == NULL) {
                Tcl_AppendResult(interp, "transfer function \"", argv[3],
                    "\" is not defined", (char*)NULL);
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                NanoVis::vol_renderer->shade_volume(*iter, tf);
                // TBD..
                // POINTSET
                /*
                if ((*iter)->pointsetIndex != -1) {
                    g_pointSet[(*iter)->pointsetIndex]->updateColor(tf->getData(), 256);
                }
                */
            }
        } else if ((c == 'd') && (strcmp(argv[2], "diffuse") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " shading diffuse value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            float diffuse;
            if (GetFloat(interp, argv[3], &diffuse) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *>::iterator iter;
            for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_diffuse(diffuse);
            }
        } else if ((c == 'o') && (strcmp(argv[2], "opacity") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " shading opacity value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
            float opacity;
            if (GetFloat(interp, argv[3], &opacity) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *>::iterator iter;
            for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_opacity_scale(opacity);
            }
        } else if ((c == 's') && (strcmp(argv[2], "specular") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " shading specular value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
            float specular;
            if (GetFloat(interp, argv[3], &specular) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            vector<Volume *>::iterator iter;
            for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_specular(specular);
            }
        } else if ((c == 'i') && (strcmp(argv[2], "isosurface") == 0)) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " shading isosurface on|off ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }
	    int iso_surface = 0;
	    if (Tcl_GetBoolean(interp, argv[3], &iso_surface) != TCL_OK) {
		return TCL_ERROR;
	    }
            vector<Volume *> ivol;
            if (GetVolumes(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
#ifndef notdef
            vector<Volume *>::iterator iter;
            for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->set_isosurface(iso_surface);
            }
#endif
	} else {
	    Tcl_AppendResult(interp, "bad shading option \"", argv[2], 
		"\": should be diffuse, opacity, specular, transfunc, or ",
		"isosurface", (char*)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 's') && (strcmp(argv[1], "state") == 0)) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " state on|off ?volume...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        int state;
        if (Tcl_GetBoolean(interp, argv[2], &state) != TCL_OK) {
            return TCL_ERROR;
        }
        vector<Volume *> ivol;
        if (GetVolumes(interp, argc-3, argv+3, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }
	if (state) {
	    vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->enable();
	    }
	} else {
	    vector<Volume *>::iterator iter;
	    for (iter = ivol.begin(); iter != ivol.end(); iter++) {
                (*iter)->disable();
            }
	}
    } else if ((c == 't') && (strcmp(argv[1],"test2") == 0)) {
        NanoVis::volume[1]->disable_data();
        NanoVis::volume[1]->disable();
        return TCL_OK;
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1], "\": should be ",
		"data, outline, shading, or state", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
FlowCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    Rappture::Outcome err;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			 " option ?arg arg?", (char *)NULL);
	return TCL_ERROR;
    }
    char c = argv[1][0];
    if ((c == 'v') && (strcmp(argv[1], "vectorid") == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" vectorid volume", (char *)NULL);
	    return TCL_ERROR;
	}
        Volume *volPtr;

        if (GetVolume(interp, argv[2], &volPtr) != TCL_OK) { 
	    return TCL_ERROR;
        }
        if (NanoVis::particleRenderer != NULL) {
            NanoVis::particleRenderer->setVectorField(volPtr->id, 1.0f, 
                volPtr->height / (float)volPtr->width,
                volPtr->depth  / (float)volPtr->width,
                volPtr->range_max());
	    NanoVis::initParticle();
        }
        if (NanoVis::licRenderer != NULL) {
            NanoVis::licRenderer->setVectorField(volPtr->id,
                1.0f / volPtr->aspect_ratio_width,
                1.0f / volPtr->aspect_ratio_height,
                1.0f / volPtr->aspect_ratio_depth,
                volPtr->range_max());
            NanoVis::licRenderer->set_offset(NanoVis::lic_slice_z);
        }
    } else if (c == 'l' && strcmp(argv[1], "lic") == 0) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " lic on|off\"", (char*)NULL);
            return TCL_ERROR;
        }
	int ibool;
	if (Tcl_GetBoolean(interp, argv[2], &ibool) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::lic_on = (bool)ibool;
    } else if ((c == 'p') && (strcmp(argv[1], "particle") == 0)) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" particle visible|slice|slicepos arg \"", (char*)NULL);
            return TCL_ERROR;
        }
	c = argv[2][0];
	if ((c == 'v') && (strcmp(argv[2], "visible") == 0)) {
	    if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" particle visible on|off\"", (char*)NULL);
		return TCL_ERROR;
	    }
	    int state;
	    if (Tcl_GetBoolean(interp, argv[2], &state) != TCL_OK) {
		return TCL_ERROR;
	    }
	    NanoVis::particle_on = state;
	} else if ((c == 's') && (strcmp(argv[2], "slice") == 0)) {
	    if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" particle slice volume\"", (char*)NULL);
		return TCL_ERROR;
	    }
	    int axis;
	    if (GetAxis(interp, argv[3], &axis) != TCL_OK) {
		return TCL_ERROR;
	    }
	    NanoVis::lic_axis = axis;
	} else if ((c == 's') && (strcmp(argv[2], "slicepos") == 0)) {
	    if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" particle slicepos value\"", (char*)NULL);
		return TCL_ERROR;
	    }
	    float pos;
	    if (GetFloat(interp, argv[2], &pos) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (pos < 0.0f) {
		pos = 0.0f;
	    } else if (pos > 1.0f) {
		pos = 1.0f;
	    }
	    switch (NanoVis::lic_axis) {
	    case 0 :
		NanoVis::lic_slice_x = pos;
		break;
	    case 1 :
		NanoVis::lic_slice_y = pos;
		break;
	    case 2 :
		NanoVis::lic_slice_z = pos;
		break;
	    }
	} else {
	    Tcl_AppendResult(interp, "unknown option \"", argv[2], 
		"\": should be \"", argv[0], " visible, slice, or slicepos\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 'r') && (strcmp(argv[1],"reset") == 0)) {
	NanoVis::initParticle();
    } else if ((c == 'c') && (strcmp(argv[1],"capture") == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" capture numframes\"", (char*)NULL);
            return TCL_ERROR;
        }
	int total_frame_count;

	if (Tcl_GetInt(interp, argv[2], &total_frame_count) != TCL_OK) {
	    return TCL_ERROR;
	}
        if (NanoVis::licRenderer) {
	    NanoVis::licRenderer->activate();
	}
        if (NanoVis::particleRenderer) {
	    NanoVis::particleRenderer->activate();
	}
	// Karl
	for (int frame_count = 0; frame_count < total_frame_count; 
	     frame_count++) {
	    
	    // Generate the latest frame and send it back to the client
	    if (NanoVis::licRenderer && 
		NanoVis::licRenderer->isActivated()) {
		NanoVis::licRenderer->convolve();               
	    }
	    if (NanoVis::particleRenderer && 
		NanoVis::particleRenderer->isActivated()) {
		NanoVis::particleRenderer->advect();
	    }
	    NanoVis::offscreen_buffer_capture();  //enable offscreen render
	    NanoVis::display();
	    
	    //		printf("Read Screen for Writing to file...\n");
	    
	    NanoVis::read_screen();
	    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	    
	    NanoVis::bmp_write_to_file(frame_count);
	    //		printf("Writing to file...\n");
	}
	// put your code... 
        if (NanoVis::licRenderer) {
	    NanoVis::licRenderer->deactivate();
	}
        if (NanoVis::particleRenderer) {
	    NanoVis::particleRenderer->deactivate();
	}
        NanoVis::initParticle();
    } else if (c == 'd' && strcmp(argv[1],"data") == 0) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" data follows ?args?", (char *)NULL);
	    return TCL_ERROR;
	}
	c = argv[2][0];
        if ((c == 'f') && (strcmp(argv[2],"follows") == 0)) {
	    if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
				 " data follows length", (char *)NULL);
		return TCL_ERROR;
	    }
            int nbytes;
            if (Tcl_GetInt(interp, argv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }
	    Rappture::Buffer buf;
	    if (GetDataStream(interp, buf, nbytes) != TCL_OK) {
		return TCL_ERROR;
	    }
            int n = NanoVis::n_volumes;
            std::stringstream fdata;
            fdata.write(buf.bytes(),buf.size());
            load_vector_stream(n, fdata);
	    Volume *volPtr = NanoVis::volume[n];

            //
            // BE CAREFUL:  Set the number of slices to something
            //   slightly different for each volume.  If we have
            //   identical volumes at exactly the same position
            //   with exactly the same number of slices, the second
            //   volume will overwrite the first, so the first won't
            //   appear at all.
            //
            if (volPtr != NULL) {
                volPtr->set_n_slice(256-n);
                volPtr->disable_cutplane(0);
                volPtr->disable_cutplane(1);
                volPtr->disable_cutplane(2);

                NanoVis::vol_renderer->add_volume(volPtr, 
			NanoVis::get_transfunc("default"));
            }
        }
    } else {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int 
HeightMapCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	     const char *argv[])
{
    if (argc < 2) {
	srand((unsigned)time(NULL));
	int size = 20 * 200;
	double sigma = 5.0;
	double mean = exp(0.0) / (sigma * sqrt(2.0));
	float* data = (float*) malloc(sizeof(float) * size);
	
	float x;
	for (int i = 0; i < size; ++i) {
	    x = - 10 + i%20;
	    data[i] = exp(- (x * x)/(2 * sigma * sigma)) / 
		(sigma * sqrt(2.0)) / mean * 2 + 1000;
	}

	HeightMap* heightMap = new HeightMap();
	float minx = 0.0f;
	float maxx = 1.0f;
	float miny = 0.5f;
	float maxy = 3.5f;
	heightMap->setHeight(minx, miny, maxx, maxy, 20, 200, data);
	heightMap->setColorMap(NanoVis::get_transfunc("default"));
	heightMap->setVisible(true);
	heightMap->setLineContourVisible(true);
	NanoVis::heightMap.push_back(heightMap);


	Vector3 min(minx, (float) heightMap->range_min(), miny);
	Vector3 max(maxx, (float) heightMap->range_max(), maxy);
	
	NanoVis::grid->setMinMax(min, max);
	NanoVis::grid->setVisible(true);
	
	return TCL_OK;
    }
    
    char c = argv[1][0];
    if ((c == 'c') && (strcmp(argv[1], "create") == 0)) {
	HeightMap *hMap;

	/* heightmap create xmin ymin xmax ymax xnum ynum values */
	hMap = CreateHeightMap(cdata, interp, argc - 2, argv + 2);
	if (hMap == NULL) {
	    return TCL_ERROR;
	}
	NanoVis::heightMap.push_back(hMap);
	/* FIXME: Convert this file to use Tcl_CmdObjProc */
	sprintf(interp->result, "%d", NanoVis::heightMap.size() - 1);
	return TCL_OK;
    } else if ((c == 'd') && (strcmp(argv[1],"data") == 0)) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" data follows|visible ?args?", (char *)NULL);
	    return TCL_ERROR;
	}
        //bytes
        char c;
	c = argv[2][0];
        if ((c == 'v') && (strcmp(argv[2],"visible") == 0)) {
	    if (argc < 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			" data visible bool ?indices?", (char *)NULL);
		return TCL_ERROR;
	    }
	    int ivisible;
	    vector<unsigned int> indices;

	    if (Tcl_GetBoolean(interp, argv[3], &ivisible) != TCL_OK) {
		return TCL_ERROR;
	    }
            if (GetIndices(interp, argc-4, argv+4, &indices) != TCL_OK) {
               return TCL_ERROR;
            }
	    bool visible;
	    visible = (bool)ivisible;
            for (unsigned int i = 0; i < indices.size(); ++i) {
                if ((indices[i] < NanoVis::heightMap.size()) && 
		    (NanoVis::heightMap[indices[i]] != NULL)) {
                    NanoVis::heightMap[indices[i]]->setVisible(visible);
                }
            }
        } else if ((c == 'f') && (strcmp(argv[2],"follows") == 0)) {
	    if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			" data follow length", (char *)NULL);
		return TCL_ERROR;
	    }
	    Rappture::Buffer buf;
            int nBytes;

            if (Tcl_GetInt(interp, argv[3], &nBytes) != TCL_OK) {
                return TCL_ERROR;
            }
	    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
		return TCL_ERROR;
	    }
	    buf.append("\0", 1);
	    if (Tcl_Eval(interp, buf.bytes()) != TCL_OK) {
		fprintf(stderr, "error in command: %s\n", 
			Tcl_GetStringResult(interp));
		fflush(stderr);
		return TCL_ERROR;
	    }
        } else {
	    Tcl_AppendResult(interp, "unknown option \"", argv[2], "\": ",
			     "should be visible or follows", (char *)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 'l') && (strcmp(argv[1], "linecontour") == 0)) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" linecontour visible|color ?args?", (char *)NULL);
	    return TCL_ERROR;
	}
        vector<unsigned int> indices;
	char c;
	c = argv[2][0];
        if ((c == 'v') && (strcmp(argv[2],"visible") == 0)) {
	    if (argc < 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			" linecontour visible boolean ?indices?", (char *)NULL);
		return TCL_ERROR;
	    }
	    int ivisible;
	    bool visible;
	    if (Tcl_GetBoolean(interp, argv[3], &ivisible) != TCL_OK) {
		return TCL_ERROR;
	    }
            visible = (bool)ivisible;
            if (GetIndices(interp, argc-4, argv+4, &indices) != TCL_OK) {
                return TCL_ERROR;
            }
            for (unsigned int i = 0; i < indices.size(); ++i) {
                if ((indices[i] < NanoVis::heightMap.size()) && 
		    (NanoVis::heightMap[indices[i]] != NULL)) {
                    NanoVis::heightMap[indices[i]]->setLineContourVisible(visible);
                }
            }
        } else if ((c == 'c') && (strcmp(argv[2],"color") == 0)) {
	    if (argc < 6) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			" linecontour color r g b ?indices?", (char *)NULL);
		return TCL_ERROR;
	    }
            float r, g, b;
            if ((GetFloat(interp, argv[3], &r) != TCL_OK) ||
                (GetFloat(interp, argv[4], &g) != TCL_OK) ||
                (GetFloat(interp, argv[5], &b) != TCL_OK)) {
                return TCL_ERROR;
	    }		
            vector<unsigned int> indices;
            if (GetIndices(interp, argc-6, argv+6, &indices) != TCL_OK) {
                return TCL_ERROR;
            }
            for (unsigned int i = 0; i < indices.size(); ++i) {
                if ((indices[i] < NanoVis::heightMap.size()) && 
		    (NanoVis::heightMap[indices[i]] != NULL)) {
                    NanoVis::heightMap[indices[i]]->setLineContourColor(r, g, b);
                }
            }
        } else {
	    Tcl_AppendResult(interp, "unknown option \"", argv[2], "\": ",
			     "should be visible or color", (char *)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 't') && (strcmp(argv[1], "transfunc") == 0)) {
	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" transfunc name ?indices?", (char *)NULL);
	    return TCL_ERROR;
	}
        TransferFunction *tf;

	tf = NanoVis::get_transfunc(argv[2]);
        if (tf == NULL) {
            Tcl_AppendResult(interp, "transfer function \"", argv[2],
                "\" is not defined", (char*)NULL);
            return TCL_ERROR;
        }
        vector<unsigned int> indices;
        if (GetIndices(interp, argc - 3, argv + 3, &indices) != TCL_OK) {
	        return TCL_ERROR;
	}
	for (unsigned int i = 0; i < indices.size(); ++i) {
	    if ((indices[i] < NanoVis::heightMap.size()) && 
		(NanoVis::heightMap[indices[i]] != NULL)) {
		NanoVis::heightMap[indices[i]]->setColorMap(tf);
	    }
        }
    } else if ((c == 'l') && (strcmp(argv[1], "legend") == 0)) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			     " legend index width height\"", (char*)NULL);
	    return TCL_ERROR;
	}
	HeightMap *hMap;
	if (GetHeightMap(interp, argv[2], &hMap) != TCL_OK) {
	    return TCL_ERROR;
	}
	TransferFunction *tf;
	tf = hMap->getColorMap();
	if (tf == NULL) {
	    Tcl_AppendResult(interp, 
		"no transfer function defined for heightmap \"", argv[1], "\"", 
		(char*)NULL);
	    return TCL_ERROR;
	}
	int width, height;
	if ((Tcl_GetInt(interp, argv[3], &width) != TCL_OK) || 
	    (Tcl_GetInt(interp, argv[4], &height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	NanoVis::render_legend(tf, hMap->range_min(), hMap->range_max(), 
		width, height, argv[1]);
    } else if ((c == 'p') && (strcmp(argv[1], "polygon") == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" polygon mode", (char *)NULL);
	    return TCL_ERROR;
	}
	graphics::RenderContext::PolygonMode mode;
	if (GetPolygonMode(interp, argv[2], &mode) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::renderContext->setPolygonMode(mode);
    } else if ((c == 'c') && (strcmp(argv[1], "cull") == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" cull mode", (char *)NULL);
	    return TCL_ERROR;
	}
	graphics::RenderContext::CullMode mode;
	if (GetCullMode(interp, argv[2], &mode) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::renderContext->setCullMode(mode);
    } else if ((c == 's') && (strcmp(argv[1], "shade") == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
		" shade model", (char *)NULL);
	    return TCL_ERROR;
	}
	graphics::RenderContext::ShadingModel model;
	if (GetShadingModel(interp, argv[2], &model) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::renderContext->setShadingModel(model);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be data, linecontour, legend, or transfunc", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int 
GridCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0], 
			 " option ?args?", (char *)NULL);
	return TCL_ERROR;
    }
    char c = argv[1][0];
    if ((c == 'v') && (strcmp(argv[1],"visible") == 0)) {
	int ivisible;

	if (Tcl_GetBoolean(interp, argv[2], &ivisible) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::grid->setVisible((bool)ivisible);
    } else if ((c == 'l') && (strcmp(argv[1],"linecount") == 0)) {
        int x, y, z;

        if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK) ||
            (Tcl_GetInt(interp, argv[3], &y) != TCL_OK) ||
            (Tcl_GetInt(interp, argv[4], &z) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (NanoVis::grid) {
	    NanoVis::grid->setGridLineCount(x, y, z);
	}
    } else if ((c == 'a') && (strcmp(argv[1],"axiscolor") == 0)) {
        float r, g, b, a;
        if ((GetFloat(interp, argv[2], &r) != TCL_OK) ||
            (GetFloat(interp, argv[3], &g) != TCL_OK) ||
            (GetFloat(interp, argv[4], &b) != TCL_OK)) {
	    return TCL_ERROR;
	}
	a = 1.0f;
	if ((argc == 6) && (GetFloat(interp, argv[5], &a) != TCL_OK)) {
	    return TCL_ERROR;
	}	    
	if (NanoVis::grid) {
	    NanoVis::grid->setAxisColor(r, g, b, a);
	}
    } else if ((c == 'l') && (strcmp(argv[1],"linecolor") == 0)) {
        float r, g, b, a;
        if ((GetFloat(interp, argv[2], &r) != TCL_OK) ||
            (GetFloat(interp, argv[3], &g) != TCL_OK) ||
	    (GetFloat(interp, argv[4], &b) != TCL_OK)) {
	    return TCL_ERROR;
	}
	a = 1.0f;
	if ((argc == 6) && (GetFloat(interp, argv[5], &a) != TCL_OK)) {
	    return TCL_ERROR;
	}	    
	if (NanoVis::grid) {
	    NanoVis::grid->setGridLineColor(r, g, b, a);
	}
    } else if ((c == 'm') && (strcmp(argv[1],"minmax") == 0)) {
	double x1, y1, z1, x2, y2, z2;
	if ((Tcl_GetDouble(interp, argv[2], &x1) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[3], &y1) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[4], &z1) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[5], &x2) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[6], &y2) != TCL_OK) ||
	    (Tcl_GetDouble(interp, argv[7], &z2) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (NanoVis::grid) {
	    NanoVis::grid->setMinMax(Vector3(x1, y1, z1), Vector3(x2, y2, z2));
	}
    } else if ((c == 'a') && (strcmp(argv[1],"axisname") == 0)) {
        int axisId;
	if (GetAxis(interp, argv[2], &axisId) != TCL_OK) {
	    return TCL_ERROR;
	}
        if (NanoVis::grid) {
	    NanoVis::grid->setAxisName(axisId, argv[3]);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
			 "\": should be data, outline, shading, or state", 
			 (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int 
AxisCmd(ClientData cdata, Tcl_Interp *interp, int argc, const char *argv[])
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }
    char c = argv[1][0];
    if ((c == 'v') && (strcmp(argv[1],"visible") == 0)) {
	int ivisible;

	if (Tcl_GetBoolean(interp, argv[2], &ivisible) != TCL_OK) {
	    return TCL_ERROR;
	}
	NanoVis::axis_on = (bool)ivisible;
    } else {
	Tcl_AppendResult(interp, "bad axis option \"", argv[1],
			 "\": should be visible", (char*)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetHeightMap
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetHeightMap(Tcl_Interp *interp, const char *string, HeightMap **hmPtrPtr)
{
    int mapIndex;
    if (Tcl_GetInt(interp, string, &mapIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((mapIndex < 0) || (mapIndex >= (int)NanoVis::heightMap.size()) || 
	(NanoVis::heightMap[mapIndex] == NULL)) {
	Tcl_AppendResult(interp, "invalid heightmap index \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    *hmPtrPtr = NanoVis::heightMap[mapIndex];
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumeIndex
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumeIndex(Tcl_Interp *interp, const char *string, unsigned int *indexPtr)
{
    int index;
    if (Tcl_GetInt(interp, string, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index < 0) {
	Tcl_AppendResult(interp, "can't have negative index \"", string,
			 "\"", (char*)NULL);
	return TCL_ERROR;
    }
    if (index >= (int)NanoVis::volume.size()) {
	Tcl_AppendResult(interp, "index \"", string,
			 "\" is out of range", (char*)NULL);
	return TCL_ERROR;
    }
    *indexPtr = (unsigned int)index;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolume
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolume(Tcl_Interp *interp, const char *string, Volume **volPtrPtr)
{
    unsigned int index;
    if (GetVolumeIndex(interp, string, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    Volume *vol;
    vol = NanoVis::volume[index];
    if (vol == NULL) {
	Tcl_AppendResult(interp, "no volume defined for index \"", string,
			 "\"", (char*)NULL);
	return TCL_ERROR;
    }
    *volPtrPtr = vol;
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumeIndices()
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumeIndices(Tcl_Interp *interp, int argc, const char *argv[],
    vector<unsigned int>* vectorPtr)
{
    if (argc == 0) {
        for (unsigned int n = 0; n < NanoVis::volume.size(); n++) {
            if (NanoVis::volume[n] != NULL) {
                vectorPtr->push_back(n);
            }
        }
    } else {
        for (int n = 0; n < argc; n++) {
	    unsigned int index;

	    if (GetVolumeIndex(interp, argv[n], &index) != TCL_OK) {
                return TCL_ERROR;
	    }
            if (NanoVis::volume[index] != NULL) {
                vectorPtr->push_back(index);
            }
        }
    }
    return TCL_OK;
}

static int
GetIndices(Tcl_Interp *interp, int argc, const char *argv[],
    vector<unsigned int>* vectorPtr)
{
    for (int n = 0; n < argc; n++) {
	int index;

        if (Tcl_GetInt(interp, argv[n], &index) != TCL_OK) {
            return TCL_ERROR;
        }
	if (index < 0) {
	    Tcl_AppendResult(interp, "can't have negative index \"", argv[n],
			     "\"", (char *)NULL);
	    return TCL_ERROR;
	}
        vectorPtr->push_back((unsigned int)index);
    }
    return TCL_OK;
}


/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetVolumes()
 *
 * Used internally to decode a series of volume index values and
 * store then in the specified vector.  If there are no volume index
 * arguments, this means "all volumes" to most commands, so all
 * active volume indices are stored in the vector.
 *
 * Updates pushes index values into the vector.  Returns TCL_OK or
 * TCL_ERROR to indicate an error.
 * ----------------------------------------------------------------------
 */
static int
GetVolumes(Tcl_Interp *interp, int argc, const char *argv[],
    vector<Volume *>* vectorPtr)
{
    if (argc == 0) {
        for (unsigned int n = 0; n < NanoVis::volume.size(); n++) {
            if (NanoVis::volume[n] != NULL) {
                vectorPtr->push_back(NanoVis::volume[n]);
            }
        }
    } else {
        for (int n = 0; n < argc; n++) {
	    Volume *volPtr;

	    if (GetVolume(interp, argv[n], &volPtr) != TCL_OK) {
                return TCL_ERROR;
	    }
            if (volPtr != NULL) {
                vectorPtr->push_back(volPtr);
            }
        }
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetAxis()
 *
 * Used internally to decode an axis value from a string ("x", "y",
 * or "z") to its index (0, 1, or 2).  Returns TCL_OK if successful,
 * along with a value in valPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetAxis(Tcl_Interp *interp, const char *string, int *valPtr)
{
    if (string[1] == '\0') {
	char c;

	c = tolower((unsigned char)string[0]);
	if (c == 'x') {
	    *valPtr = 0;
	    return TCL_OK;
	} else if (c == 'y') {
	    *valPtr = 1;
	    return TCL_OK;
	} else if (c == 'z') {
	    *valPtr = 2;
	    return TCL_OK;
	}
	/*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
	"\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

/*
 * ----------------------------------------------------------------------
 * FUNCTION: GetColor()
 *
 * Used internally to decode a color value from a string ("R G B")
 * as a list of three numbers 0-1.  Returns TCL_OK if successful,
 * along with RGB values in valPtr.  Otherwise, it returns TCL_ERROR
 * and an error message in the interpreter.
 * ----------------------------------------------------------------------
 */
static int
GetColor(Tcl_Interp *interp, const char *string, float *rgbPtr)
{
    int argc;
    const char **argv;

    if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (argc != 3) {
        Tcl_AppendResult(interp, "bad color \"", string,
            "\": should list of R G B values 0.0 - 1.0", (char*)NULL);
        return TCL_ERROR;
    }
    if ((GetFloat(interp, argv[0], rgbPtr + 0) != TCL_OK) ||
	(GetFloat(interp, argv[1], rgbPtr + 1) != TCL_OK) ||
	(GetFloat(interp, argv[2], rgbPtr + 2) != TCL_OK)) {
        Tcl_Free((char*)argv);
        return TCL_ERROR;
    }
    Tcl_Free((char*)argv);
    return TCL_OK;
}


static int 
PlaneNewCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	    const char *argv[])
{
    fprintf(stderr, "load plane for 2D visualization command\n");
    
    int index, w, h;
    
    if (argc != 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " plane_index w h \"", (char*)NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &w) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &h) != TCL_OK) {
	return TCL_ERROR;
    }
    
    //Now read w*h*4 bytes. The server expects the plane to be a stream of floats 
    char* tmp = new char[int(w*h*sizeof(float))];
    if (tmp == NULL) {
	Tcl_AppendResult(interp, "can't allocate stream data", (char *)NULL);
	return TCL_ERROR;
    }
    bzero(tmp, w*h*4);
    int status = read(0, tmp, w*h*sizeof(float));
    if (status <= 0) {
	exit(0);		// Bail out on read error?  Should log the
				// error and return a non-zero exit status.
    }
    plane[index] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, (float*)tmp);
    delete[] tmp;
    return TCL_OK;
}


static int 
PlaneLinkCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	     const char *argv[])
{
    fprintf(stderr, "link the plane to the 2D renderer command\n");
    
    int plane_index, tf_index;
    
    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " plane_index tf_index \"", (char*)NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[1], &plane_index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &tf_index) != TCL_OK) {
	return TCL_ERROR;
    }
    //plane_render->add_plane(plane[plane_index], tf[tf_index]);
    return TCL_OK;
}

//Enable a 2D plane for render
//The plane_index is the index mantained in the 2D plane renderer
static int 
PlaneEnableCmd(ClientData cdata, Tcl_Interp *interp, int argc, 
	       const char *argv[])
{
    fprintf(stderr,"enable a plane so the 2D renderer can render it command\n");
    
    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			 " plane_index mode \"", (char*)NULL);
	return TCL_ERROR;
    }
    int plane_index;
    if (Tcl_GetInt(interp, argv[1], &plane_index) != TCL_OK) {
	return TCL_ERROR;
    }
    int mode;
    if (Tcl_GetInt(interp, argv[2], &mode) != TCL_OK) {
	return TCL_ERROR;
    }
    if (mode == 0) {
	plane_index = -1;
    }
    plane_render->set_active_plane(plane_index);
    return TCL_OK;
}


void 
initTcl()
{
    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    Tcl_DStringInit(&cmdbuffer);

    // manipulate the viewpoint
    Tcl_CreateCommand(interp, "camera", CameraCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // turn on/off cut planes in x/y/z directions
    Tcl_CreateCommand(interp, "cutplane", CutplaneCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // request the legend for a plot (transfer function)
    Tcl_CreateCommand(interp, "legend", LegendCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // change the size of the screen (size of picture generated)
    Tcl_CreateCommand(interp, "screen", ScreenCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // manipulate transfer functions
    Tcl_CreateCommand(interp, "transfunc", TransfuncCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // set the "up" direction for volumes
    Tcl_CreateCommand(interp, "up", UpCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // manipulate volume data
    Tcl_CreateCommand(interp, "volume", VolumeCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "flow", FlowCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "axis", AxisCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "grid", GridCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "heightmap", HeightMapCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // get screenshot
    Tcl_CreateCommand(interp, "screenshot", ScreenShotCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "unirect2d", UniRect2dCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "axis", AxisCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "grid", GridCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "heightmap", HeightMapCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // get screenshot
    Tcl_CreateCommand(interp, "screenshot", ScreenShotCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "unirect2d", UniRect2dCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

#ifdef __TEST_CODE__
    Tcl_CreateCommand(interp, "test", TestCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);


//    Tcl_CreateCommand(interp, "flow", FlowCmd,
 //       (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
#endif

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        fprintf(stdin, "WARNING: bad default transfer function\n");
        fprintf(stdin, Tcl_GetStringResult(interp));
    }
}


void 
xinetd_listen()
{
    int flags = fcntl(0, F_GETFL, 0); 
    fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

    int status = TCL_OK;
    int npass = 0;

    //
    //  Read and execute as many commands as we can from stdin...
    //
    while (status == TCL_OK) {
        //
        //  Read the next command from the buffer.  First time through
        //  we block here and wait if necessary until a command comes in.
        //
        //  BE CAREFUL:  Read only one command, up to a newline.
        //  The "volume data follows" command needs to be able to read
        //  the data immediately following the command, and we shouldn't
        //  consume it here.
        //
        while (1) {
            char c = getchar();
            if (c <= 0) {
                if (npass == 0) {
                    exit(0);  // EOF -- we're done!
                } else {
                    break;
                }
            }
            Tcl_DStringAppend(&cmdbuffer, &c, 1);

            if (c=='\n' && Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer))) {
                break;
            }
        }

        // no command? then we're done for now
        if (Tcl_DStringLength(&cmdbuffer) == 0) {
            break;
        }

        // back to original flags during command evaluation...
        fcntl(0, F_SETFL, flags & ~O_NONBLOCK);
        status = Tcl_Eval(interp, Tcl_DStringValue(&cmdbuffer));
        Tcl_DStringSetLength(&cmdbuffer, 0);

        // non-blocking for next read -- we might not get anything
        fcntl(0, F_SETFL, flags | O_NONBLOCK);
        npass++;
    }
    fcntl(0, F_SETFL, flags);

    if (status != TCL_OK) {
        std::ostringstream errmsg;
        errmsg << "ERROR: " << Tcl_GetStringResult(interp) << std::endl;
        write(0, errmsg.str().c_str(), errmsg.str().size());
        return;
    }

    //
    // This is now in "FlowCmd()":
    //  Generate the latest frame and send it back to the client
    //
    /*
    if (NanoVis::licRenderer && NanoVis::licRenderer->isActivated())
    {
        NanoVis::licRenderer->convolve();
    }

    if (NanoVis::particleRenderer && NanoVis::particleRenderer->isActivated())
    {
        NanoVis::particleRenderer->advect();
    }
    */

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
    write(0, rle, rle_size);	//unsigned byte
#else
    NanoVis::bmp_write("nv>image -bytes");
#endif
}


/*
 * -----------------------------------------------------------------------
 *
 * GetDataStream -- 
 *
 *	Read the requested number of bytes from standard input into the given
 *	buffer.  The buffer is then decompressed and decoded.
 *
 * -----------------------------------------------------------------------
 */
static int
GetDataStream(Tcl_Interp *interp, Rappture::Buffer &buf, int nBytes)
{
    char buffer[8096];

    clearerr(stdin);
    while (nBytes > 0) {
	unsigned int chunk;
	int nRead;

	chunk = (sizeof(buffer) < (unsigned int) nBytes) ? 
	    sizeof(buffer) : nBytes;
	nRead = fread(buffer, sizeof(char), chunk, stdin);
	if (ferror(stdin)) {
            Tcl_AppendResult(interp, "while reading data stream: ",
		Tcl_PosixError(interp), (char*)NULL);
            return TCL_ERROR;
	}
	if (feof(stdin)) {
            Tcl_AppendResult(interp, "premature EOF while reading data stream",
		(char*)NULL);
            return TCL_ERROR;
	}
	buf.append(buffer, nRead);
	nBytes -= nRead;
    }
    {
	Rappture::Outcome err;

	err = Rappture::encoding::decode(buf, RPENC_Z|RPENC_B64|RPENC_HDR);
	if (err) {
	    printf("ERROR -- DECODING\n");
	    fflush(stdout);
	    Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 * -----------------------------------------------------------------------
 *
 * CreateHeightMap --
 *
 *	Creates a heightmap from the given the data. The format of the data
 *	should be as follows:
 *
 *		xMin, xMax, xNum, yMin, yMax, yNum, heights...
 *
 *	xNum and yNum must be integer values, all others are real numbers.
 *	The number of heights must be xNum * yNum;
 *
 * -----------------------------------------------------------------------
 */
static HeightMap *
CreateHeightMap(ClientData clientData, Tcl_Interp *interp, int argc, 
	const char *argv[])
{
    float xMin, yMin, xMax, yMax;
    int xNum, yNum;

    if (argc != 7) {
	Tcl_AppendResult(interp, 
	"wrong # of values: should be xMin yMin xMax yMax xNum yNum heights",
		(char *)NULL);
	return NULL;
    }
    if ((GetFloat(interp, argv[0], &xMin) != TCL_OK) ||
	(GetFloat(interp, argv[1], &yMin) != TCL_OK) || 
	(GetFloat(interp, argv[2], &xMax) != TCL_OK) ||
	(GetFloat(interp, argv[3], &yMax) != TCL_OK) ||
	(Tcl_GetInt(interp, argv[4], &xNum) != TCL_OK) ||
	(Tcl_GetInt(interp, argv[5], &yNum) != TCL_OK)) {
	return NULL;
    }
    int nValues;
    const char **elem;
    if (Tcl_SplitList(interp, argv[6], &nValues, &elem) != TCL_OK) {
	return NULL;
    }
    if ((xNum <= 0) || (yNum <= 0)) {
	Tcl_AppendResult(interp, "bad number of x or y values", (char *)NULL);
	goto error;
    }
    if (nValues != (xNum * yNum)) {
	Tcl_AppendResult(interp, "wrong # of heights", (char *)NULL);
	goto error;
    }

    float *heights;
    heights = new float[nValues];
    if (heights == NULL) {
	Tcl_AppendResult(interp, "can't allocate array of heights", 
		(char *)NULL);
	goto error;
    }

    int i;
    for (i = 0; i < nValues; i++) {
	if (GetFloat(interp, elem[i], heights + i) != TCL_OK) {
	    delete [] heights;
	    goto error;
	}
    }
    HeightMap* hMap;
    hMap = new HeightMap();
    hMap->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, heights);
    hMap->setColorMap(NanoVis::get_transfunc("default"));
    hMap->setVisible(true);
    hMap->setLineContourVisible(true);

    Tcl_Free((char *)elem);
    delete [] heights;
    return hMap;
 error:
    Tcl_Free((char *)elem);
    return NULL;
}

/*
 * This command should be Tcl procedure instead of a C command.  The reason
 * for this that 1) we are using a safe interpreter so we would need a master
 * interpreter to load the Tcl environment properly (including our "unirect2d"
 * procedure). And 2) the way nanovis is currently deployed, doesn't make it
 * easy to add new directories for procedures, since it's loaded into /tmp.
 *
 * Ideally, the "unirect2d" proc would do a rundimentary parsing of the data
 * to verify the structure and then pass it to the appropiate Tcl command
 * (heightmap, volume, etc). Our C command always creates a heightmap.  
 */
static int
UniRect2dCmd(ClientData, Tcl_Interp *interp, int argc, const char *argv[])
{    
    int xNum, yNum, zNum;
    float xMin, yMin, xMax, yMax;
    float *zValues;

    if ((argc & 0x01) == 0) {
	Tcl_AppendResult(interp, 
		"wrong number of arguments: should be key-value pairs", 
		(char *)NULL);
	return TCL_ERROR;
    }
    zValues = NULL;
    xNum = yNum = zNum = 0;
    xMin = yMin = xMax = yMax = 0.0f;
    int i;
    for (i = 1; i < argc; i += 2) {
	if (strcmp(argv[i], "xmin") == 0) {
	    if (GetFloat(interp, argv[i+1], &xMin) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "xmax") == 0) {
	    if (GetFloat(interp, argv[i+1], &xMax) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "xnum") == 0) {
	    if (Tcl_GetInt(interp, argv[i+1], &xNum) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (xNum <= 0) {
		Tcl_AppendResult(interp, "bad xnum value: must be > 0",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "ymin") == 0) {
	    if (GetFloat(interp, argv[i+1], &yMin) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "ymax") == 0) {
	    if (GetFloat(interp, argv[i+1], &yMax) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "ynum") == 0) {
	    if (Tcl_GetInt(interp, argv[i+1], &yNum) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (yNum <= 0) {
		Tcl_AppendResult(interp, "bad ynum value: must be > 0",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	} else if (strcmp(argv[i], "zvalues") == 0) {
	    const char **zlist;

	    if (Tcl_SplitList(interp, argv[i+1], &zNum, &zlist) != TCL_OK) {
		return TCL_ERROR;
	    }
	    int j;
	    zValues = new float[zNum];
	    for (j = 0; j < zNum; j++) {
		if (GetFloat(interp, zlist[j], zValues + j) != TCL_OK) {
		    Tcl_Free((char *)zlist);
		    return TCL_ERROR;
		}
	    }
	    Tcl_Free((char *)zlist);
	} else {
	    Tcl_AppendResult(interp, "unknown key \"", argv[i], 
		"\": should be xmin, xmax, xnum, ymin, ymax, ynum, or zvalues",
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (zValues == NULL) {
	Tcl_AppendResult(interp, "missing \"zvalues\" key", (char *)NULL);
	return TCL_ERROR;
    }
    if (zNum != (xNum * yNum)) {
	Tcl_AppendResult(interp, "wrong number of z values must be xnum*ynum",
		(char *)NULL);
	return TCL_ERROR;
    }
    HeightMap* hMap;
    hMap = new HeightMap();
    hMap->setHeight(xMin, yMin, xMax, yMax, xNum, yNum, zValues);
    hMap->setColorMap(NanoVis::get_transfunc("default"));
    hMap->setVisible(true);
    hMap->setLineContourVisible(true);
    NanoVis::heightMap.push_back(hMap);
    delete [] zValues;
    return TCL_OK;
}
