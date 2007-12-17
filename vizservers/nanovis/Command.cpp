#include "Command.h"


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
#include "Camera.h"

// FOR testing new functions
//#define  _LOCAL_ZINC_TEST_
//#include "Test.h"

// EXTERN DECLARATIONS
// in Nv.cpp
extern VolumeRenderer* g_vol_render;
extern PointSetRenderer* g_pointset_renderer;
extern NvColorTableRenderer* g_color_table_renderer;
extern Grid* g_grid;

// in nanovis.cpp
extern int n_volumes;
extern vector<Volume*> volume;
extern vector<HeightMap*> g_heightMap;
extern vector<PointSet*> g_pointSet;

extern float live_rot_x;		//object rotation angles
extern float live_rot_y;
extern float live_rot_z;
extern float live_obj_x;	//object translation location from the origin
extern float live_obj_y;
extern float live_obj_z;
extern int updir;
extern Camera* cam;

extern char *def_transfunc;
extern Tcl_HashTable tftable;
extern float live_diffuse;
extern float live_specular;

extern bool axis_on;

extern int win_width;			//size of the render window
extern int win_height;			//size of the render window
extern PlaneRenderer* plane_render;
extern Texture2D* plane[10];


extern Rappture::Outcome load_volume_stream(int index, std::iostream& fin);
extern Rappture::Outcome load_volume_stream2(int index, std::iostream& fin);
extern void load_volume(int index, int width, int height, int depth, int n_component, float* data, double vmin, double vmax, 
                double nzero_min);
extern TransferFunction* get_transfunc(char *name);
extern void resize_offscreen_buffer(int w, int h);
extern void offscreen_buffer_capture();
extern void bmp_header_add_int(unsigned char* header, int& pos, int data);
extern void bmp_write(const char* cmd);
extern void bmp_write_to_file();
extern void display();
extern void display_offscreen_buffer();
extern void read_screen();
extern int renderLegend(int ivol, int width, int height, const char* volArg);

// Tcl interpreter for incoming messages
Tcl_Interp *interp;
Tcl_DString cmdbuffer;

static int ScreenShotCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CameraCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CutplaneCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int LegendCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ScreenCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int TransfuncCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int UpCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int VolumeCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

static int PlaneNewCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneEnableCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

static int GridCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int AxisCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

static int GetVolumeIndices _ANSI_ARGS_((Tcl_Interp *interp, int argc, CONST84 char *argv[], vector<int>* vectorPtr));
static int GetIndices(Tcl_Interp *interp, int argc, CONST84 char *argv[], vector<int>* vectorPtr);
static int GetAxis _ANSI_ARGS_((Tcl_Interp *interp, char *str, int *valPtr));
static int GetColor _ANSI_ARGS_((Tcl_Interp *interp, char *str, float *rgbPtr));


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
static int CameraCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
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
ScreenShotCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
    int old_win_width = win_width;
    int old_win_height = win_height;

#ifdef XINETD
    resize_offscreen_buffer(1024, 1024);
    cam->set_screen_size(30, 90, 1024 - 60, 1024 - 120);
    offscreen_buffer_capture();  //enable offscreen render
    display();

    // INSOO
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

    bmp_write("nv>screenshot -bytes");
    
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

        vector<int> ivol;
        if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        vector<int>::iterator iter = ivol.begin();
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

        vector<int> ivol;
        if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        vector<int>::iterator iter = ivol.begin();
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

    renderLegend(ivol, width, height, argv[1]);

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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-5, argv+5, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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
            printf("Data Loading\n");
            //fflush(stdout);
            //return TCL_OK;

            int nbytes;
            if (Tcl_GetInt(interp, argv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }

            Rappture::Outcome err;
            Rappture::Buffer buf;

            // DEBUG
            int totalsize = nbytes;
            char buffer[8096];
            while (nbytes > 0) 
            {
                int chunk = (sizeof(buffer) < nbytes) ? sizeof(buffer) : nbytes;
                int status = fread(buffer, 1, chunk, stdin);
                //printf("Begin Reading [%d Read : %d Left]\n", status, nbytes - status);
                fflush(stdout);
                if (status > 0) {
                    buf.append(buffer,status);
                    nbytes -= status;
                } else {
                    printf("data unpacking failed\n");
                    Tcl_AppendResult(interp, "data unpacking failed: unexpected EOF",
                        (char*)NULL);
                    return TCL_ERROR;
                }
            }

            err = Rappture::encoding::decode(buf,RPENC_Z|RPENC_B64|RPENC_HDR);
            if (err) {
                printf("ERROR -- DECODING\n");
                fflush(stdout);
                Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
                return TCL_ERROR;
            }

            int n = n_volumes;
            char header[6];
            memcpy(header, buf.bytes(), sizeof(char) * 5);
            header[5] = '\0';

#ifdef _LOCAL_ZINC_TEST_
            //FILE* fp = fopen("/home/nanohub/vrinside/nv/data/HOON/QDWL_100_100_50_strain_8000i.nd_zatom_12_1", "rb");
            FILE* fp = fopen("/home/nanohub/vrinside/nv/data/HOON/GaAs_AlGaAs_2QD_B4.nd_zc_1_wf", "rb");
            unsigned char* b = (unsigned char*) malloc(buf.size());
            if (fp == 0)
            {
                printf("cannot open the file\n");
                fflush(stdout);
                return TCL_ERROR;
            }
            fread(b, buf.size(), 1, fp);
            fclose(fp);
#endif

            
            printf("Checking header[%s]\n", header);
            fflush(stdout);
            if (!strcmp(header, "<HDR>"))
            {
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
#endif

                printf("finish loading\n");
                fflush(stdout);
                if (vol)
                {
                    while (n_volumes <= n) 
                    {
                        volume.push_back((Volume*) NULL);
                        n_volumes++;
                    }

                    if (volume[n] != NULL) 
                    {
                        delete volume[n];
                        volume[n] = NULL;
                    }

                    float dx0 = -0.5;
                    float dy0 = -0.5*vol->height/vol->width;
                    float dz0 = -0.5*vol->depth/vol->width;
                    vol->move(Vector3(dx0, dy0, dz0));

                    volume[n] = vol;
                }
            }
#ifdef __TEST_CODE__
            else if (!strcmp(header, "<FET>"))
            {
                printf("FET loading...\n");
                fflush(stdout);
                std::stringstream fdata;
                fdata.write(buf.bytes(),buf.size());
                err = load_volume_stream3(n, fdata);

                if (err) {
                    Tcl_AppendResult(interp, err.remark().c_str(), (char*)NULL);
                    return TCL_ERROR;
                }
            }
#endif
            else
            {
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
            if (volume[n])
            {
                volume[n]->set_n_slice(256-n);
                volume[n]->disable_cutplane(0);
                volume[n]->disable_cutplane(1);
                volume[n]->disable_cutplane(2);

                g_vol_render->add_volume(volume[n], get_transfunc("default"));
            }

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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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
        else if (c == 'v' && strcmp(argv[2],"visible") == 0) {
            if (argv[3] == "false")
            {
                for (int i = 0; i < n_volumes; ++i)
                {
                    if (volume[i]) volume[i]->disable_outline();
                }
            }
            else if (argv[3] == "true")
            {
                for (int i = 0; i < n_volumes; ++i)
                {
                    if (volume[i]) volume[i]->enable_outline();
                }
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

            vector<int> ivol;
            if (GetVolumeIndices(interp, argc-4, argv+4, &ivol) != TCL_OK) {
                return TCL_ERROR;
            }

            vector<int>::iterator iter = ivol.begin();
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

        vector<int> ivol;
        if (GetVolumeIndices(interp, argc-3, argv+3, &ivol) != TCL_OK) {
            return TCL_ERROR;
        }

        vector<int>::iterator iter = ivol.begin();
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
    else if (c == 't' && strcmp(argv[1],"test2") == 0) {
        volume[1]->disable_data();
        volume[1]->disable();
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be data, outline, shading, or state", (char*)NULL);
    return TCL_ERROR;
}


int HeightMapCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
{
    if (argc < 2) 
    {
        {   
        srand( (unsigned)time( NULL ) );
        int size = 20 * 20;
        float sigma = 5.0f;
        float mean = exp(0.0f) / (sigma * sqrt(2.0f));
        float* data = (float*) malloc(sizeof(float) * size);

        float x;
        for (int i = 0; i < size; ++i)
        {
            x = - 10 + i%20;
            data[i] = exp(- (x * x)/(2 * sigma * sigma)) / (sigma * sqrt(2.0f)) / mean;
        }

        HeightMap* heightMap = new HeightMap();
        heightMap->setHeight(0, 0, 1, 1, 20, 20, data);
        heightMap->setColorMap(get_transfunc("default"));
        heightMap->setVisible(true);
        heightMap->setLineContourVisible(true);
        g_heightMap.push_back(heightMap);
        }

        return TCL_OK;
    }

    char c = *argv[1];
    if (c == 'd' && strcmp(argv[1],"data") == 0) 
    {
        //bytes
        vector<int> indices;
        if (strcmp(argv[2],"visible") == 0) 
        {
            bool visible = !strcmp(argv[3], "true");
            
            if (GetIndices(interp, argc-4, argv+4, &indices) != TCL_OK) 
            {
               return TCL_ERROR;
            }

            for (int i = 0; i < indices.size(); ++i)
            {
                if ((indices[i] < g_heightMap.size()) && (g_heightMap[indices[i]] != NULL))
                {
                    g_heightMap[indices[i]]->setVisible(visible);
                }
            }
            return TCL_OK;
        }
        else if (c == 'f' && strcmp(argv[2],"follows") == 0) {
            int nbytes;
            if (Tcl_GetInt(interp, argv[3], &nbytes) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    else if (c == 'l' && (strcmp(argv[1], "linecontour") == 0))
    {
        //bytes
        vector<int> indices;
        if (strcmp(argv[2],"visible") == 0) 
        {
            
            bool visible = !(strcmp("true", argv[3]));
            printf("heightmap linecontour visible %s\n", (visible)?"true":"false");
            if (GetIndices(interp, argc-4, argv+4, &indices) != TCL_OK) 
            {
                return TCL_ERROR;
            }

            for (int i = 0; i < indices.size(); ++i)
            {
                printf("heightmap index %d\n");
                if ((indices[i] < g_heightMap.size()) && (g_heightMap[indices[i]] != NULL))
                {
                    printf("heightmap index %d visible applied\n");
                    g_heightMap[indices[i]]->setLineContourVisible(visible);
                }
            }
            return TCL_OK;
        }
        else if (strcmp(argv[2],"color") == 0) 
        {
            double r, g, b;
            if ((Tcl_GetDouble(interp, argv[3], &r) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[4], &g) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[5], &b) == TCL_OK)) {
                r = r / 255.0;
                g = g / 255.0;
                b = b / 255.0;
            }
            else 
            {
                return TCL_ERROR;
            }

            vector<int> indices;
            if (GetIndices(interp, argc-6, argv+6, &indices) != TCL_OK) 
            {
                return TCL_ERROR;
            }
            for (int i = 0; i < indices.size(); ++i)
            {
                if ((indices[i] < g_heightMap.size()) && (g_heightMap[indices[i]] != NULL))
                {
                    g_heightMap[indices[i]]->setLineContourColor(r, g, b);
                }
            }

            return TCL_OK;
        }
    }
    else if (c == 't' && (strcmp(argv[1], "transfunc") == 0))
    {
        TransferFunction *tf = get_transfunc((char*)argv[2]);
        if (tf == NULL) {
            Tcl_AppendResult(interp, "transfer function \"", argv[3],
                "\" is not defined", (char*)NULL);
            return TCL_ERROR;
        }

        vector<int> indices;
        if (GetVolumeIndices(interp, argc - 3, argv + 3, &indices) != TCL_OK) 
        {
            for (int i = 0; i < indices.size(); ++i)
            {
                if ((indices[i] < g_heightMap.size()) && (g_heightMap[indices[i]] != NULL))
                {
                    g_heightMap[indices[i]]->setColorMap(tf);
                }
            }
        }
        return TCL_OK;
    }
    
    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be data, outline, shading, or state", (char*)NULL);
    return TCL_ERROR;
}

int GridCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
{
    char c = *argv[1];
    if (c == 'v' && strcmp(argv[1],"visible") == 0) 
    {
        if (strcmp(argv[2],"true") == 0) 
        {
            g_grid->setVisible(true);
            return TCL_OK;
        }
        else if (strcmp(argv[2],"false") == 0) 
        {
            g_grid->setVisible(false);
            return TCL_OK;
        }
    }
    else if (c == 'l' && strcmp(argv[1],"linecount") == 0) 
    {
        int x, y, z;

        if ((Tcl_GetInt(interp, argv[2], &x) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[3], &y) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[4], &z) == TCL_OK)) {

            if (g_grid) g_grid->setGridLineCount(x, y, z);

            return TCL_OK;
        }
    }
    else if (c == 'a' && strcmp(argv[1],"axiscolor") == 0) 
    {
        int r, g, b;
        if ((Tcl_GetInt(interp, argv[2], &r) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[3], &g) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[4], &b) == TCL_OK)) {

            if (g_grid) g_grid->setAxisColor(r / 255.0f, g / 255.0f, b / 255.0f);
            return TCL_OK;
        }
    }
    else if (c == 'l' && strcmp(argv[1],"linecolor") == 0) 
    {
        int r, g, b;
        if ((Tcl_GetInt(interp, argv[2], &r) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[3], &g) == TCL_OK) &&
            (Tcl_GetInt(interp, argv[4], &b) == TCL_OK)) {

            if (g_grid) g_grid->setGridLineColor(r / 255.0f, g / 255.0f, b / 255.0f);
            return TCL_OK;
        }
    }
    else if (c == 'm')
    {
        if (strcmp(argv[1],"minmax") == 0) 
        {
            double x1, y1, z1, x2, y2, z2;
            if ((Tcl_GetDouble(interp, argv[2], &x1) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[3], &y1) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[4], &z1) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[5], &x2) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[6], &y2) == TCL_OK) &&
                (Tcl_GetDouble(interp, argv[7], &z2) == TCL_OK)) {

                if (g_grid) g_grid->setMinMax(Vector3(x1, y1, z1), Vector3(x2, y2, z2));

                return TCL_OK;
            }
        }
    }
    else if (c == 'a' && strcmp(argv[1],"axisname") == 0) 
    {
        int axisID = 0;
        if (!strcmp(argv[2], "x")) axisID = 0;
        if (!strcmp(argv[2], "y")) axisID = 1;
        if (!strcmp(argv[2], "z")) axisID = 2;
        
        if (g_grid) g_grid->setAxisName(axisID, argv[3]);
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be data, outline, shading, or state", (char*)NULL);
    return TCL_ERROR;
}


int AxisCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]))
{
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
            " option arg arg...\"", (char*)NULL);
        return TCL_ERROR;
    }

    char c = *argv[1];
    if (c == 'v' && strcmp(argv[1],"visible") == 0) 
    {
        if (strcmp(argv[2],"true") == 0) 
        {
            axis_on = true;
        }
        else if (strcmp(argv[2],"false") == 0) 
        {
            axis_on = false;
        }
            
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
    vector<int>* vectorPtr)
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


static int
GetIndices(Tcl_Interp *interp, int argc, CONST84 char *argv[],
    vector<int>* vectorPtr)
{
    int ivol;
    for (int n=0; n < argc; n++) 
    {
        if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
            return TCL_ERROR;
        }
        vectorPtr->push_back(ivol);
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


void initTcl()
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

    Tcl_CreateCommand(interp, "axis", AxisCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "grid", GridCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    Tcl_CreateCommand(interp, "heightmap", HeightMapCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // get screenshot
    Tcl_CreateCommand(interp, "screenshot", ScreenShotCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

#ifdef __TEST_CODE__
    Tcl_CreateCommand(interp, "test", TestCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
#endif

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        fprintf(stdin, "WARNING: bad default transfer function\n");
        fprintf(stdin, Tcl_GetStringResult(interp));
    }
}

void xinetd_listen()
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
    //  Generate the latest frame and send it back to the client
    //
    // INSOO
    offscreen_buffer_capture();  //enable offscreen render

    display();

    // INSOO
#ifdef XINETD
   read_screen();
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 
#else
   display_offscreen_buffer(); //display the final rendering on screen
   read_screen();
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
    bmp_write("nv>image -bytes");
#endif
}


