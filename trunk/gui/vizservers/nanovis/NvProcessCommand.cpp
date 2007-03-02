#include "NvProcessCommand.h"
#include "Nv.h"
#include "nanovis.h"
//transfer function headers
#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"
#include "ZincBlendeVolume.h"
#include "NvLoadFile.h"
#include "NvVolQDVolume.h"
#include "NvColorTableRenderer.h"
#include "NvParticleRenderer.h"
#include "NvEventLog.h"

#include <fstream>
#include <iostream>
#include <sstream>

// R2 headers
#include <R2/R2FilePath.h>
#include <R2/R2Fonts.h>

#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

extern int n_volumes;
// pointers to volumes, currently handle up to 10 volumes
extern vector<Volume*> volume;

// maps transfunc name to TransferFunction object
extern Tcl_HashTable tftable;

// pointers to 2D planes, currently handle up 10
extern Texture2D* plane[10];

// value indicates "up" axis:  x=1, y=2, z=3, -x=-1, -y=-2, -z=-3
extern int updir;

extern VolumeRenderer* g_vol_render;
extern NvColorTableRenderer* g_color_table_renderer;
extern PlaneRenderer* plane_render;
extern Camera* cam;
extern char *def_transfunc;
extern float color_table[256][4]; 	
extern unsigned char* screen_buffer;
extern float live_rot_x;     //object rotation angles
extern float live_rot_y;
extern float live_rot_z;

extern float live_obj_x;    //object translation location from the origin
extern float live_obj_y;
extern float live_obj_z;

extern int win_width;           //size of the render window
extern int win_height;          //size of the render window


// Tcl interpreter for incoming messages
Tcl_Interp *interp;
Tcl_DString cmdbuffer;

extern void update_tf_texture();
extern void get_slice_vectors();
extern Rappture::Outcome load_volume_file(int index, char *fname);
extern void load_volume(int index, int width, int height, int depth, int n_component, float* data, double vmin, double vmax);
extern TransferFunction* get_transfunc(char *name);
extern void resize_offscreen_buffer(int w, int h);
extern void offscreen_buffer_capture();
extern void bmp_header_add_int(unsigned char* header, int& pos, int data);
extern void bmp_write(const char* cmd);
extern void bmp_write_to_file();
extern void display();
extern void display_offscreen_buffer();
extern void read_screen();


static int ScreenShotCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CameraCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CutplaneCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int LegendCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ScreenCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int TransfuncCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int UpCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int VolumeCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int FlowCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

static int PlaneNewCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneEnableCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

static int GetVolumeIndices _ANSI_ARGS_((Tcl_Interp *interp, int argc, CONST84 char *argv[], std::vector<int>* vectorPtr));
static int GetAxis _ANSI_ARGS_((Tcl_Interp *interp, char *str, int *valPtr));
static int GetColor _ANSI_ARGS_((Tcl_Interp *interp, char *str, float *rgbPtr));

void NvInitTclCommand()
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

    // get screenshot
    Tcl_CreateCommand(interp, "screenshot", ScreenShotCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "flow", FlowCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        fprintf(stdin, "WARNING: bad default transfer function\n");
        fprintf(stdin, Tcl_GetStringResult(interp));
    }
}

/*
 * ----------------------------------------------------------------------
 * CLIENT COMMAND:
 *   camera aim <x0> <y0> <z0>
 *   camera angle <xAngle> <yAngle> <zAngle>
 *   camera zoom <factor>
 *
 * Clients send these commands to manipulate the camera.  The "angle"
 * operation controls the angle of the camera around the focal point.
 * The "zoom" operation sets the zoom factor, moving the camera in
 * and out.
 * ----------------------------------------------------------------------
 */
static int
CameraCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
	if (argc < 2) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" option arg arg...\"", (char*)NULL);
		return TCL_ERROR;
    }

    char c = *argv[1];
	if (c == 'a' && strcmp(argv[1],"angle") == 0) {
        if (argc != 5) {
		    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			    " angle xangle yangle zangle\"", (char*)NULL);
		    return TCL_ERROR;
        }

        double xangle, yangle, zangle;
	    if (Tcl_GetDouble(interp, argv[2], &xangle) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    if (Tcl_GetDouble(interp, argv[3], &yangle) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    if (Tcl_GetDouble(interp, argv[4], &zangle) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    cam->rotate(xangle, yangle, zangle);

	    return TCL_OK;
	}
	else if (c == 'a' && strcmp(argv[1],"aim") == 0) {
        if (argc != 5) {
		    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			    " aim x y z\"", (char*)NULL);
		    return TCL_ERROR;
        }

        double x0, y0, z0;
	    if (Tcl_GetDouble(interp, argv[2], &x0) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    if (Tcl_GetDouble(interp, argv[3], &y0) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    if (Tcl_GetDouble(interp, argv[4], &z0) != TCL_OK) {
		    return TCL_ERROR;
	    }
	    cam->aim(x0, y0, z0);

	    return TCL_OK;
	}
	else if (c == 'z' && strcmp(argv[1],"zoom") == 0) {
        if (argc != 3) {
		    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			    " zoom factor\"", (char*)NULL);
		    return TCL_ERROR;
        }

        double zoom;
	    if (Tcl_GetDouble(interp, argv[2], &zoom) != TCL_OK) {
		    return TCL_ERROR;
	    }

        live_obj_z = -2.5/zoom;
		cam->move(live_obj_x, live_obj_y, live_obj_z);

	    return TCL_OK;
    }

	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be aim, angle, or zoom", (char*)NULL);
	return TCL_ERROR;
}

static int
FlowCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    if (g_particleRenderer) 
    {
        g_particleRenderer->reset();
    }
	return TCL_OK;
}

static int
ScreenShotCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    int old_win_width = win_width;
    int old_win_height = win_height;
#ifdef XINETD
    resize_offscreen_buffer(1024, 1024);
    cam->set_screen_size(30, 90, 1024 - 60, 1024 - 120);
    offscreen_buffer_capture();  //enable offscreen render
    display();

    // TBD
    Volume* vol = volume[0];
    TransferFunction* tf = g_vol_render->get_volume_shading(vol);
    if (tf)
    {
        float data[512];
        for (int i=0; i < 256; i++) {
            data[i] = data[i+256] = (float)(i/255.0);
        }
        Texture2D* plane = new Texture2D(256, 2, GL_FLOAT, GL_LINEAR, 1, data);
        g_color_table_renderer->render(1024, 1024, plane, tf, vol->range_min(), vol->range_max());
        delete plane;
    }

    read_screen();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    // INSOO
    // TBD
    bmp_write_to_file();
    //bmp_write("nv>screenshot -bytes");
    resize_offscreen_buffer(old_win_width, old_win_height); 
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
 * operations are applied to the volumes represented by one or more
 * <volume> indices.  If no volumes are specified, then all volumes
 * are updated.
 * ----------------------------------------------------------------------
 */

static int
CutplaneCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option ?arg arg...?\"", (char*)NULL);
        return TCL_ERROR;
    }

    char c = *argv[1];
    if (c == 's' && strcmp(argv[1],"state") == 0) {
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

        std::vector<int> ivol;
        if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        std::vector<int>::iterator iter = ivol.begin();
        while (iter != ivol.end()) {
            if (state) {
                volume[*iter]->enable_cutplane(axis);
            } else {
                volume[*iter]->disable_cutplane(axis);
            }
            ++iter;
        }
        return TCL_OK;
    }
    else if (c == 'p' && strcmp(argv[1],"position") == 0) {
        if (argc < 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " position relval axis ?volume ...? \"", (char*)NULL);
            return TCL_ERROR;
        }

        double relval;
        if (Tcl_GetDouble(interp, argv[2], &relval) != TCL_OK) {
            return TCL_ERROR;
        }
        // keep this just inside the volume so it doesn't disappear
        if (relval < 0.01) { relval = 0.01; }
        if (relval > 0.99) { relval = 0.99; }

        int axis;
        if (GetAxis(interp, (char*) argv[3], &axis) != TCL_OK) {
            return TCL_ERROR;
        }

        std::vector<int> ivol;
        if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        std::vector<int>::iterator iter = ivol.begin();
        while (iter != ivol.end()) {
            volume[*iter]->move_cutplane(axis, (float)relval);
            ++iter;
        }
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be position or state", (char*)NULL);
    return TCL_ERROR;
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
LegendCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    if (argc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " transfunc width height\"", (char*)NULL);
        return TCL_ERROR;
    }

    TransferFunction *tf = NULL;
    int ivol;
    if (Tcl_GetInt(interp, argv[1], &ivol) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ivol < n_volumes) {
        tf = g_vol_render->get_volume_shading(volume[ivol]);
    }
    if (tf == NULL) {
        Tcl_AppendResult(interp, "transfer function not defined for volume ", argv[1], (char*)NULL);
        return TCL_ERROR;
    }

    int old_width = win_width;
    int old_height = win_height;

    int width, height;
    if (Tcl_GetInt(interp, argv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }

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
    result << "nv>legend " << argv[1];
    result << " " << volume[ivol]->range_min();
    result << " " << volume[ivol]->range_max();
    bmp_write(result.str().c_str());
    write(0, "\n", 1);

    plane_render->remove_plane(index);
    resize_offscreen_buffer(old_width, old_height);

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
ScreenCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
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
    resize_offscreen_buffer(w, h);

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
TransfuncCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
	if (argc < 2) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" option arg arg...\"", (char*)NULL);
		return TCL_ERROR;
    }

    char c = *argv[1];
	if (c == 'd' && strcmp(argv[1],"define") == 0) {
        if (argc != 5) {
		    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			    argv[1], " define name colormap alphamap\"", (char*)NULL);
            return TCL_ERROR;
        }

        // decode the data and store in a series of fields
        Rappture::Field1D rFunc, gFunc, bFunc, wFunc;
        int cmapc, wmapc, i, j;
        char **cmapv, **wmapv;

        if (Tcl_SplitList(interp, argv[3], &cmapc, (const char***)&cmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (cmapc % 4 != 0) {
            Tcl_Free((char*)cmapv);
		    Tcl_AppendResult(interp, "bad colormap in transfunc: should be ",
                "{ v r g b ... }", (char*)NULL);
            return TCL_ERROR;
        }

        if (Tcl_SplitList(interp, argv[4], &wmapc, (const char***)&wmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (wmapc % 2 != 0) {
            Tcl_Free((char*)cmapv);
            Tcl_Free((char*)wmapv);
		    Tcl_AppendResult(interp, "bad alphamap in transfunc: should be ",
                "{ v w ... }", (char*)NULL);
            return TCL_ERROR;
        }

        for (i=0; i < cmapc; i += 4) {
            double vals[4];
            for (j=0; j < 4; j++) {
                if (Tcl_GetDouble(interp, cmapv[i+j], &vals[j]) != TCL_OK) {
                    Tcl_Free((char*)cmapv);
                    Tcl_Free((char*)wmapv);
                    return TCL_ERROR;
                }
                if (vals[j] < 0 || vals[j] > 1) {
                    Tcl_Free((char*)cmapv);
                    Tcl_Free((char*)wmapv);
		            Tcl_AppendResult(interp, "bad value \"", cmapv[i+j],
                        "\": should be in the range 0-1", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            rFunc.define(vals[0], vals[1]);
            gFunc.define(vals[0], vals[2]);
            bFunc.define(vals[0], vals[3]);
        }

        for (i=0; i < wmapc; i += 2) {
            double vals[2];
            for (j=0; j < 2; j++) {
                if (Tcl_GetDouble(interp, wmapv[i+j], &vals[j]) != TCL_OK) {
                    Tcl_Free((char*)cmapv);
                    Tcl_Free((char*)wmapv);
                    return TCL_ERROR;
                }
                if (vals[j] < 0 || vals[j] > 1) {
                    Tcl_Free((char*)cmapv);
                    Tcl_Free((char*)wmapv);
		            Tcl_AppendResult(interp, "bad value \"", wmapv[i+j],
                        "\": should be in the range 0-1", (char*)NULL);
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

        //INSOO
        FILE* f = fopen("/tmp/aa.cpp", "wt");
        fprintf(f, "float tfdata[] = {\n");
        fprintf(f, "    ");
        int num = nslots * 4;
        for (i=0; i < num; i++) {
            if (((i + 1) % 4) == 0)
            {
                fprintf(f, "\n    ");
            }
            fprintf(f, "%f, ", data[i]);
        }
        fclose(f);

        // find or create this transfer function
        int newEntry;
        Tcl_HashEntry *entryPtr;
        TransferFunction *tf;

        entryPtr = Tcl_CreateHashEntry(&tftable, argv[2], &newEntry);
        if (newEntry) {
            tf = new TransferFunction(nslots, data);
            Tcl_SetHashValue(entryPtr, (ClientData)tf);
        } else {
            tf = (TransferFunction*)Tcl_GetHashValue(entryPtr);
            tf->update(data);
        }

        return TCL_OK;
    }


    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be define", (char*)NULL);
    return TCL_ERROR;
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
UpCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
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

    updir = (axis+1)*sign;

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
void NvLoadVolumeBinFile2(int index, char* filename);
static int
VolumeCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }

    char c = *argv[1];
    if (c == 'a' && strcmp(argv[1],"axis") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                argv[1], " option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = *argv[2];
        if (c == 'l' && strcmp(argv[2],"label") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " label x|y|z string ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            int axis;
            if (GetAxis(interp, (char*)argv[3], &axis) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-5, argv+5, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                volume[*iter]->set_label(axis, (char*)argv[4]);
                ++iter;
            }
            return TCL_OK;
        }
        Tcl_AppendResult(interp, "bad option \"", argv[2],
            "\": should be label", (char*)NULL);
        return TCL_ERROR;
    }
    else if (c == 'd' && strcmp(argv[1],"data") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                argv[1], " option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = *argv[2];
        if (c == 's' && strcmp(argv[2],"state") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " state on|off ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            int state;
            if (Tcl_GetBoolean(interp, argv[3], &state) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                if (state) {
                    volume[*iter]->enable_data();
                } else {
                    volume[*iter]->disable_data();
                }
                ++iter;
            }
            return TCL_OK;
        }
        else if (c == 'f' && strcmp(argv[2],"follows") == 0) {
            int nbytes;
            if (Tcl_GetInt(interp, argv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }

            char fname[64];
            sprintf(fname,"/tmp/nv%d.dat",getpid());
            std::ofstream dfile(fname);

            char buffer[8096];
            while (nbytes > 0) {
                int chunk = (sizeof(buffer) < nbytes) ? sizeof(buffer) : nbytes;
                int status = fread(buffer, 1, chunk, stdin);
                if (status > 0) {
                    dfile.write(buffer,status);
                    nbytes -= status;
                } else {
                    Tcl_AppendResult(interp, "data unpacking failed in file ",
                        fname, (char*)NULL);
                    return TCL_ERROR;
                }
            }
            dfile.close();

            char cmdstr[512];
            sprintf(cmdstr, "mimedecode %s | gunzip -c > /tmp/nv%d.dx", fname, getpid());
            if (system(cmdstr) != 0) {
                Tcl_AppendResult(interp, "data unpacking failed in file ",
                    fname, (char*)NULL);
                return TCL_ERROR;
            }

            sprintf(fname,"/tmp/nv%d.dx",getpid());

            int n = n_volumes;
            Rappture::Outcome err = load_volume_file(n, fname);

            sprintf(cmdstr, "rm -f /tmp/nv%d.dat /tmp/nv%d.dx", getpid(), getpid());
            system(cmdstr);

            if (err) {
                Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
                return TCL_ERROR;
            }

            //
            // BE CAREFUL:  Set the number of slices to something
            //   slightly different for each volume.  If we have
            //   identical volumes at exactly the same position
            //   with exactly the same number of slices, the second
            //   volume will overwrite the first, so the first won't
            //   appear at all.
            //
            volume[n]->set_n_slice(256-n);
            volume[n]->disable_cutplane(0);
            volume[n]->disable_cutplane(1);
            volume[n]->disable_cutplane(2);
            g_vol_render->add_volume(volume[n], get_transfunc("default"));

            return TCL_OK;
        }
        Tcl_AppendResult(interp, "bad option \"", argv[2],
            "\": should be follows or state", (char*)NULL);
        return TCL_ERROR;
    }
    else if (c == 'o' && strcmp(argv[1],"outline") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                argv[1], " option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = *argv[2];
        if (c == 's' && strcmp(argv[2],"state") == 0) {
            if (argc < 3) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " state on|off ?volume ...? \"", (char*)NULL);
                return TCL_ERROR;
            }

            int state;
            if (Tcl_GetBoolean(interp, argv[3], &state) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                if (state) {
                    volume[*iter]->enable_outline();
                } else {
                    volume[*iter]->disable_outline();
                }
                ++iter;
            }
            return TCL_OK;
        }
        else if (c == 'c' && strcmp(argv[2],"color") == 0) {
            if (argc < 3) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " color {R G B} ?volume ...? \"", (char*)NULL);
                return TCL_ERROR;
            }

            float rgb[3];
            if (GetColor(interp, (char*) argv[3], rgb) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                volume[*iter]->set_outline_color(rgb);
                ++iter;
            }
            return TCL_OK;
        }

        Tcl_AppendResult(interp, "bad option \"", argv[2],
            "\": should be color or state", (char*)NULL);
        return TCL_ERROR;
    }
    else if (c == 's' && strcmp(argv[1],"shading") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                argv[1], " option ?arg arg...?\"", (char*)NULL);
            return TCL_ERROR;
        }
        c = *argv[2];
        if (c == 't' && strcmp(argv[2],"transfunc") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " transfunc name ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            TransferFunction *tf = get_transfunc((char*)argv[3]);
            if (tf == NULL) {
                Tcl_AppendResult(interp, "transfer function \"", argv[3],
                    "\" is not defined", (char*)NULL);
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                g_vol_render->shade_volume(volume[*iter], tf);
                ++iter;
            }
            return TCL_OK;
        }
        else if (c == 'd' && strcmp(argv[2],"diffuse") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " diffuse value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            double dval;
            if (Tcl_GetDouble(interp, argv[3], &dval) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                volume[*iter]->set_diffuse((float)dval);
                ++iter;
            }
            return TCL_OK;
        }
        else if (c == 'o' && strcmp(argv[2],"opacity") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " opacity value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            double dval;
            if (Tcl_GetDouble(interp, argv[3], &dval) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                volume[*iter]->set_opacity_scale((float)dval);
                ++iter;
            }
            return TCL_OK;
        }
        else if (c == 's' && strcmp(argv[2],"specular") == 0) {
            if (argc < 4) {
                Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    argv[1], " specular value ?volume ...?\"", (char*)NULL);
                return TCL_ERROR;
            }

            double dval;
            if (Tcl_GetDouble(interp, argv[3], &dval) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            std::vector<int>::iterator iter = ivol.begin();
            while (iter != ivol.end()) {
                volume[*iter]->set_specular((float)dval);
                ++iter;
            }
            return TCL_OK;
        }
        Tcl_AppendResult(interp, "bad option \"", argv[2],
            "\": should be diffuse, opacity, specular, or transfunc", (char*)NULL);
        return TCL_ERROR;
    }
    else if (c == 's' && strcmp(argv[1],"state") == 0) {
        if (argc < 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                argv[1], " on|off ?volume...?\"", (char*)NULL);
            return TCL_ERROR;
        }

        int state;
        if (Tcl_GetBoolean(interp, argv[2], &state) != TCL_OK) {
            return TCL_ERROR;
        }

        std::vector<int> ivol;
        if (GetVolumeIndices(interp, argc-3, argv+3, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        std::vector<int>::iterator iter = ivol.begin();
        while (iter != ivol.end()) {
            if (state) {
                volume[*iter]->enable();
            } else {
                volume[*iter]->disable();
            }
            ++iter;
        }
        return TCL_OK;
    }
    else if (c == 'v' && strcmp(argv[1],"volqd") == 0) {
        int n = n_volumes;

        NvLoadVolumeBinFile(n, "/home/nanohub/vrinside/grid_1_0");

        volume[n]->set_n_slice(256-n);
        volume[n]->disable_cutplane(0);
        volume[n]->disable_cutplane(1);
        volume[n]->disable_cutplane(2);
        g_vol_render->add_volume(volume[n], get_transfunc("default"));

        return TCL_OK;
    }
    else if (c == 'z' && strcmp(argv[1],"zinc") == 0) {
        int n = n_volumes;

        NvLoadVolumeBinFile2(n, "/home/nanohub/vrinside/grid_1_0");

        volume[n]->set_n_slice(256-n);
        volume[n]->disable_cutplane(0);
        volume[n]->disable_cutplane(1);
        volume[n]->disable_cutplane(2);
        g_vol_render->add_volume(volume[n], get_transfunc("default"));

        return TCL_OK;
    }
    else if (c == 't' && strcmp(argv[1],"test2") == 0) {
        volume[1]->disable_data();
        volume[1]->disable();
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be data, outline, shading, or state", (char*)NULL);
    return TCL_ERROR;
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
GetVolumeIndices(Tcl_Interp *interp, int argc, CONST84 char *argv[],
    std::vector<int>* vectorPtr)
{
    if (argc == 0) {
        for (int n=0; n < volume.size(); n++) {
            if (volume[n] != NULL) {
                vectorPtr->push_back(n);
            }
        }
    } else {
        int ivol;
        for (int n=0; n < argc; n++) {
            if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                return TCL_ERROR;
            }
            if (ivol < 0 || ivol >= volume.size()) {
                Tcl_AppendResult(interp, "bad volume index \"", argv[n],
                    "\"", (char*)NULL);
                return TCL_ERROR;
            }
            if (volume[ivol] != NULL) {
                vectorPtr->push_back(ivol);
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
GetAxis(Tcl_Interp *interp, char *str, int *valPtr)
{
    if (strcmp(str,"x") == 0) {
        *valPtr = 0;
        return TCL_OK;
    }
    else if (strcmp(str,"y") == 0) {
        *valPtr = 1;
        return TCL_OK;
    }
    else if (strcmp(str,"z") == 0) {
        *valPtr = 2;
        return TCL_OK;
    }
    Tcl_AppendResult(interp, "bad axis \"", str,
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
GetColor(Tcl_Interp *interp, char *str, float *rgbPtr)
{
    int rgbc;
    char **rgbv;
    if (Tcl_SplitList(interp, str, &rgbc, (const char***)&rgbv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (rgbc != 3) {
        Tcl_AppendResult(interp, "bad color \"", str,
            "\": should be {R G B} as double values 0-1", (char*)NULL);
        return TCL_ERROR;
    }

    double rval, gval, bval;
    if (Tcl_GetDouble(interp, rgbv[0], &rval) != TCL_OK) {
        Tcl_Free((char*)rgbv);
        return TCL_ERROR;
    }
    if (Tcl_GetDouble(interp, rgbv[1], &gval) != TCL_OK) {
        Tcl_Free((char*)rgbv);
        return TCL_ERROR;
    }
    if (Tcl_GetDouble(interp, rgbv[2], &bval) != TCL_OK) {
        Tcl_Free((char*)rgbv);
        return TCL_ERROR;
    }
    Tcl_Free((char*)rgbv);

    rgbPtr[0] = (float)rval;
    rgbPtr[1] = (float)gval;
    rgbPtr[2] = (float)bval;

    return TCL_OK;
}


static int 
PlaneNewCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
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
  bzero(tmp, w*h*4);
  int status = read(0, tmp, w*h*sizeof(float));
  if (status <= 0){
    exit(0);
  }
 
  plane[index] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, (float*)tmp);
  
  delete[] tmp;
  return TCL_OK;
}

static 
int PlaneLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
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
static 
int PlaneEnableCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
{
  fprintf(stderr, "enable a plane so the 2D renderer can render it command\n");

  int plane_index, mode;

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" plane_index mode \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[1], &plane_index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetInt(interp, argv[2], &mode) != TCL_OK) {
	return TCL_ERROR;
  }

  if(mode==0)
    plane_render->set_active_plane(-1);
  else
    plane_render->set_active_plane(plane_index);

  return TCL_OK;
}
