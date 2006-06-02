/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
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

#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "nanovis.h"
#include "RpField1D.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

//transfer function headers
#include "transfer-function/TransferFunctionMain.h"
#include "transfer-function/ControlPoint.h"
#include "transfer-function/TransferFunctionGLUTWindow.h"
#include "transfer-function/ColorGradientGLUTWindow.h"
#include "transfer-function/ColorPaletteWindow.h"
#include "transfer-function/MainWindow.h"

//render server

VolumeRenderer* vol_render;
PlaneRenderer* plane_render;
Camera* cam;

//if true nanovis renders volumes in 3D, if not renders 2D plane
bool volume_mode = true; 

// color table for built-in transfer function editor
float color_table[256][4]; 	

// default transfer function
char *def_transfunc = "transfunc define default {\n\
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

#ifdef XINETD
FILE* xinetd_log;
#endif

FILE* event_log;
//log
void init_event_log();
void end_event_log();
double cur_time;	//in seconds
double get_time_interval();

int render_window; 		//the handle of the render window;
// forward declarations
void init_particles();
void get_slice_vectors();
Rappture::Outcome load_volume_file(int index, char *fname);
void load_volume(int index, int width, int height, int depth, int n_component, float* data);
TransferFunction* get_transfunc(char *name);

ParticleSystem* psys;
float psys_x=0.4, psys_y=0, psys_z=0;

Lic* lic;

//frame buffer for final rendering
unsigned char* screen_buffer = NULL;
NVISid final_fbo, final_color_tex, final_depth_rb;

bool advect=false;
float vert[NMESH*NMESH*3];		//particle positions in main memory
float slice_vector[NMESH*NMESH*4];	//per slice vectors in main memory

int n_volumes = 0;
// pointers to volumes, currently handle up to 10 volumes
Volume* volume[MAX_N_VOLUMES];
// maps transfunc name to TransferFunction object
Tcl_HashTable tftable;

// pointers to 2D planes, currently handle up 10
Texture2D* plane[10];

PerfQuery* perf;			//perfromance counter

//Nvidia CG shaders and their parameters
CGcontext g_context;

CGprogram m_passthru_fprog;
CGparameter m_passthru_scale_param, m_passthru_bias_param;

CGprogram m_copy_texcoord_fprog;

CGprogram m_one_volume_fprog;
CGparameter m_vol_one_volume_param;
CGparameter m_tf_one_volume_param;
CGparameter m_mvi_one_volume_param;
CGparameter m_mv_one_volume_param;
CGparameter m_render_param_one_volume_param;

CGprogram m_vert_std_vprog;
CGparameter m_mvp_vert_std_param;
CGparameter m_mvi_vert_std_param;

using namespace std;


// Tcl interpreter for incoming messages
static Tcl_Interp *interp;
static Tcl_DString cmdbuffer;

static int CameraCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CutplaneCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ClearCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ScreenCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int TransfuncCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int VolumeCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int VolumeLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int VolumeMoveCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int RefreshCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneNewCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int PlaneEnableCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

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

        int ivol;
        if (argc < 5) {
            for (ivol=0; ivol < n_volumes; ivol++) {
                if (state) {
                    volume[ivol]->enable_cutplane(axis);
                } else {
                    volume[ivol]->disable_cutplane(axis);
                }
            }
        } else {
            for (int n=4; n < argc; n++) {
                if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (ivol < 0 || ivol >= n_volumes) {
                    Tcl_AppendResult(interp, "bad volume index \"", argv[n],
                        "\": out of range", (char*)NULL);
                    return TCL_ERROR;
                }
                if (state) {
                    volume[ivol]->enable_cutplane(axis);
                } else {
                    volume[ivol]->disable_cutplane(axis);
                }
            }
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

        int ivol;
        if (argc < 5) {
            for (ivol=0; ivol < n_volumes; ivol++) {
                volume[ivol]->move_cutplane(axis, (float)relval);
            }
        } else {
            for (int n=4; n < argc; n++) {
                if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (ivol < 0 || ivol >= n_volumes) {
                    Tcl_AppendResult(interp, "bad volume index \"", argv[n],
                        "\": out of range", (char*)NULL);
                    return TCL_ERROR;
                }
                volume[ivol]->move_cutplane(axis, (float)relval);
            }
        }
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be position or state", (char*)NULL);
    return TCL_ERROR;
}

void resize_offscreen_buffer(int w, int h);

//change screen size
static int
ScreenCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{

	fprintf(stderr, "screen size cmd\n");
	double w, h;

	if (argc != 3) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" width height \"", (char*)NULL);
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[1], &w) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[2], &h) != TCL_OK) {
		return TCL_ERROR;
	}

	resize_offscreen_buffer((int)w, (int)h);
fprintf(stdin,"new screen size: %d %d\n",w,h);
	return TCL_OK;
}


static int
RefreshCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
  fprintf(stderr, "refresh cmd\n");
  return TCL_OK;
}

void set_object(float x, float y, float z){
  live_obj_x = x;
  live_obj_y = y;
  live_obj_z = z;
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

        if (Tcl_SplitList(interp, argv[3], &cmapc, &cmapv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (cmapc % 4 != 0) {
            Tcl_Free((char*)cmapv);
		    Tcl_AppendResult(interp, "bad colormap in transfunc: should be ",
                "{ v r g b ... }", (char*)NULL);
            return TCL_ERROR;
        }

        if (Tcl_SplitList(interp, argv[4], &wmapc, &wmapv) != TCL_OK) {
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
            if (GetAxis(interp, argv[3], &axis) != TCL_OK) {
                return TCL_ERROR;
            }

            int ivol;
            if (argc < 6) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    volume[ivol]->set_label(axis, argv[4]);
                }
            } else {
                for (int n=5; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    volume[ivol]->set_label(axis, argv[4]);
                }
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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    if (state) {
                        volume[ivol]->enable_data();
                    } else {
                        volume[ivol]->disable_data();
                    }
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    if (state) {
                        volume[ivol]->enable_data();
                    } else {
                        volume[ivol]->disable_data();
                    }
                }
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
            vol_render->add_volume(volume[n], get_transfunc("default"));

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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    if (state) {
                        volume[ivol]->enable_outline();
                    } else {
                        volume[ivol]->disable_outline();
                    }
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    if (ivol < 0 || ivol >= n_volumes) {
                        Tcl_AppendResult(interp, "bad volume index \"", argv[n],
                            "\": out of range", (char*)NULL);
                        return TCL_ERROR;
                    }
                    if (state) {
                        volume[ivol]->enable_outline();
                    } else {
                        volume[ivol]->disable_outline();
                    }
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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    volume[ivol]->set_outline_color(rgb);
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    if (ivol < 0 || ivol >= n_volumes) {
                        Tcl_AppendResult(interp, "bad volume index \"", argv[n],
                            "\": out of range", (char*)NULL);
                        return TCL_ERROR;
                    }
                    volume[ivol]->set_outline_color(rgb);
                }
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

            TransferFunction *tf = get_transfunc(argv[3]);
            if (tf == NULL) {
                Tcl_AppendResult(interp, "transfer function \"", argv[3],
                    "\" is not defined", (char*)NULL);
                return TCL_ERROR;
            }

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    vol_render->shade_volume(volume[ivol], tf);
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    vol_render->shade_volume(volume[ivol], tf);
                }
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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    volume[ivol]->set_diffuse((float)dval);
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    volume[ivol]->set_diffuse((float)dval);
                }
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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    volume[ivol]->set_opacity_scale((float)dval);
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    volume[ivol]->set_opacity_scale((float)dval);
                }
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

            int ivol;
            if (argc < 5) {
                for (ivol=0; ivol < n_volumes; ivol++) {
                    volume[ivol]->set_specular((float)dval);
                }
            } else {
                for (int n=4; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    volume[ivol]->set_specular((float)dval);
                }
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

        int ivol;
        if (argc < 4) {
            for (ivol=0; ivol < n_volumes; ivol++) {
                if (state) {
                    volume[ivol]->enable();
                } else {
                    volume[ivol]->disable();
                }
            }
        } else {
            for (int n=3; n < argc; n++) {
                    if (Tcl_GetInt(interp, argv[n], &ivol) != TCL_OK) {
                            return TCL_ERROR;
                    }
                if (state) {
                    volume[ivol]->enable();
                } else {
                    volume[ivol]->disable();
                }
            }
        }
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", argv[1],
        "\": should be data, outline, shading, or state", (char*)NULL);
    return TCL_ERROR;
}

static int
VolumeLinkCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
  fprintf(stderr, "link volume command\n");

  double volume_index, tf_index;

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
        " volume_index tf_index \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[1], &volume_index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &tf_index) != TCL_OK) {
	return TCL_ERROR;
  }

  //vol_render->add_volume(volume[(int)volume_index], tf[(int)tf_index]);

  return TCL_OK;
}


static int
VolumeResizeCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
  fprintf(stderr, "resize drawing size of the volume command\n");

  double volume_index, size; 

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" volume_index size \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[1], &volume_index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &size) != TCL_OK) {
	return TCL_ERROR;
  }

  volume[(int)volume_index]->set_size((float) size);
  return TCL_OK;
}

static int
VolumeMoveCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{

	fprintf(stderr, "move volume cmd\n");
	double index, x, y, z;

	if (argc != 5) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" index x_coord y_coord z_coord\"", (char*)NULL);
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[1], &index) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[2], &x) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[3], &y) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[4], &z) != TCL_OK) {
		return TCL_ERROR;
	}
	
	//set_object(x, y, z);
        volume[(int)index]->move(Vector3(x, y, z));
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
    if (Tcl_SplitList(interp, str, &rgbc, &rgbv) != TCL_OK) {
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

/*
 * ----------------------------------------------------------------------
 * USAGE: debug("string", ...)
 *
 * Use this anywhere within the package to send a debug message
 * back to the client.  The string can have % fields as used by
 * the printf() package.  Any remaining arguments are treated as
 * field substitutions on that.
 * ----------------------------------------------------------------------
 */
void
debug(char *str)
{
    write(0, str, strlen(str));
}

void
debug(char *str, double v1)
{
    char buffer[512];
    sprintf(buffer, str, v1);
    write(0, buffer, strlen(buffer));
}

void
debug(char *str, double v1, double v2)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2);
    write(0, buffer, strlen(buffer));
}

void
debug(char *str, double v1, double v2, double v3)
{
    char buffer[512];
    sprintf(buffer, str, v1, v2, v3);
    write(0, buffer, strlen(buffer));
}


static int 
PlaneNewCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])){
  fprintf(stderr, "load plane for 2D visualization command\n");

  double index, w, h;

  if (argc != 4) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" plane_index w h \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[1], &index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &w) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[3], &h) != TCL_OK) {
	return TCL_ERROR;
  }

  //Now read w*h*4 bytes. The server expects the plane to be a stream of floats 
  char* tmp = new char[int(w*h*sizeof(float))];
  bzero(tmp, w*h*4);
  int status = read(0, tmp, w*h*sizeof(float));
  if (status <= 0){
    exit(0);
  }
 
  plane[(int)index] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, (float*)tmp);
  
  delete[] tmp;
  return TCL_OK;
}


static 
int PlaneLinkCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])){
  fprintf(stderr, "link the plane to the 2D renderer command\n");

  double plane_index, tf_index;

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" plane_index tf_index \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[1], &plane_index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &tf_index) != TCL_OK) {
	return TCL_ERROR;
  }

  //plane_render->add_plane(plane[(int)plane_index], tf[(int)tf_index]);

  return TCL_OK;
}


//Enable a 2D plane for render
//The plane_index is the index mantained in the 2D plane renderer
static 
int PlaneEnableCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])){
  fprintf(stderr, "enable a plane so the 2D renderer can render it command\n");

  double plane_index, mode;

  if (argc != 3) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" plane_index mode \"", (char*)NULL);
    return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[1], &plane_index) != TCL_OK) {
	return TCL_ERROR;
  }
  if (Tcl_GetDouble(interp, argv[2], &mode) != TCL_OK) {
	return TCL_ERROR;
  }

  if(mode==0)
    plane_render->set_active_plane(-1);
  else
    plane_render->set_active_plane(plane_index);

  return TCL_OK;
}


//report errors related to CG shaders
void cgErrorCallback(void)
{
    CGerror lastError = cgGetError();
    if(lastError) {
        const char *listing = cgGetLastListing(g_context);
        printf("\n---------------------------------------------------\n");
        printf("%s\n\n", cgGetErrorString(lastError));
        printf("%s\n", listing);
        printf("-----------------------------------------------------\n");
        printf("Cg error, exiting...\n");
        cgDestroyContext(g_context);
        exit(-1);
    }
}


void init_glew(){
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		//glew init failed, exit.
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		getchar();
		assert(false);
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
}



/* Load a 3D vector field from a dx-format file
 */
void
load_vector_file(int index, char *fname) {
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;
    std::ifstream fin(fname);

    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
            }
            else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            }
            else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            }
            else if (sscanf(start, "object %d class array type %s shape 3 rank 1 items %d data follows", &dummy, type, &npts) == 3) {
                if (npts != nx*ny*nz) {
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                break;
            }
        }
    } while (!fin.eof());

    // read data points
    if (!fin.eof()) {
        Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
        Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
        Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
        Rappture::FieldRect3D xfield(xgrid, ygrid, zgrid);
        Rappture::FieldRect3D yfield(xgrid, ygrid, zgrid);
        Rappture::FieldRect3D zfield(xgrid, ygrid, zgrid);

        double vx, vy, vz;
        int nread = 0;
        for (int ix=0; ix < nx; ix++) {
            for (int iy=0; iy < ny; iy++) {
                for (int iz=0; iz < nz; iz++) {
                    if (fin.eof() || nread > npts) {
                        break;
                    }
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
                        int nindex = iz*nx*ny + iy*nx + ix;
                        xfield.define(nindex, vx);
                        yfield.define(nindex, vy);
                        zfield.define(nindex, vz);
                        nread++;
                    }
                }
            }
        }

        // make sure that we read all of the expected points
        if (nread != nx*ny*nz) {
            std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << nread << " points" << std::endl;
            return;
        }

        // figure out a good mesh spacing
        int nsample = 30;
        dx = xfield.rangeMax(Rappture::xaxis) - xfield.rangeMin(Rappture::xaxis);
        dy = xfield.rangeMax(Rappture::yaxis) - xfield.rangeMin(Rappture::yaxis);
        dz = xfield.rangeMax(Rappture::zaxis) - xfield.rangeMin(Rappture::zaxis);
        double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

        nx = (int)ceil(dx/dmin);
        ny = (int)ceil(dy/dmin);
        nz = (int)ceil(dz/dmin);

#ifndef NV40
        // must be an even power of 2 for older cards
	    nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
	    ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	    nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

        float *data = new float[3*nx*ny*nz];

        std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

        // generate the uniformly sampled data that we need for a volume
        double vmin = 0.0;
        double vmax = 0.0;
        int ngen = 0;
        for (int iz=0; iz < nz; iz++) {
            double zval = z0 + iz*dmin;
            for (int iy=0; iy < ny; iy++) {
                double yval = y0 + iy*dmin;
                for (int ix=0; ix < nx; ix++) {
                    double xval = x0 + ix*dmin;

                    vx = xfield.value(xval,yval,zval);
                    vy = yfield.value(xval,yval,zval);
                    vz = zfield.value(xval,yval,zval);
		    //vx = 1;
		    //vy = 1;
		    vz = 0;
                    double vm = sqrt(vx*vx + vy*vy + vz*vz);

                    if (vm < vmin) { vmin = vm; }
                    if (vm > vmax) { vmax = vm; }

                    data[ngen++] = vx;
                    data[ngen++] = vy;
                    data[ngen++] = vz;
                }
            }
        }

        ngen = 0;
        for (ngen=0; ngen < npts; ngen++) {
            data[ngen] = (data[ngen]/(2.0*vmax) + 0.5);
        }

        load_volume(index, nx, ny, nz, 3, data);
        delete [] data;
    } else {
        std::cerr << "WARNING: data not found in file " << fname << std::endl;
    }
}


/* Load a 3D volume from a dx-format file
 */
Rappture::Outcome
load_volume_file(int index, char *fname) {
    Rappture::Outcome result;

    Rappture::MeshTri2D xymesh;
    int dummy, nx, ny, nz, nxy, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char line[128], type[128], *start;
    std::ifstream fin(fname);

    int isrect = 1;

    do {
        fin.getline(line,sizeof(line)-1);
        for (start=&line[0]; *start == ' ' || *start == '\t'; start++)
            ;  // skip leading blanks

        if (*start != '#') {  // skip comment lines
            if (sscanf(start, "object %d class gridpositions counts %d %d %d", &dummy, &nx, &ny, &nz) == 4) {
                // found grid size
                isrect = 1;
            }
            else if (sscanf(start, "object %d class array type float rank 1 shape 3 items %d data follows", &dummy, &nxy) == 2) {
                isrect = 0;
                double xx, yy, zz;
                for (int i=0; i < nxy; i++) {
                    fin.getline(line,sizeof(line)-1);
                    if (sscanf(line, "%lg %lg %lg", &xx, &yy, &zz) == 3) {
                        xymesh.addNode( Rappture::Node2D(xx,yy) );
                    }
                }

                char fpts[128];
                sprintf(fpts, "/tmp/tmppts%d", getpid());
                char fcells[128];
                sprintf(fcells, "/tmp/tmpcells%d", getpid());

                std::ofstream ftmp(fpts);
                // save corners of bounding box first, to work around meshing
                // problems in voronoi utility
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMin(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMax(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                ftmp << xymesh.rangeMin(Rappture::xaxis) << " "
                     << xymesh.rangeMax(Rappture::yaxis) << std::endl;
                for (int i=0; i < nxy; i++) {
                    ftmp << xymesh.atNode(i).x() << " " << xymesh.atNode(i).y() << std::endl;
                }
                ftmp.close();

                char cmdstr[512];
                sprintf(cmdstr, "voronoi -t < %s > %s", fpts, fcells);
                if (system(cmdstr) == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri(fcells);
                    while (!ftri.eof()) {
                        ftri.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%d %d %d", &cx, &cy, &cz) == 3) {
                            if (cx >= 4 && cy >= 4 && cz >= 4) {
                                // skip first 4 boundary points
                                xymesh.addCell(cx-4, cy-4, cz-4);
                            }
                        }
                    }
                    ftri.close();
                } else {
                    return result.error("triangularization failed");
                }

                sprintf(cmdstr, "rm -f %s %s", fpts, fcells);
                system(cmdstr);
            }
            else if (sscanf(start, "object %d class regulararray count %d", &dummy, &nz) == 2) {
                // found z-grid
            }
            else if (sscanf(start, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
                // found origin
            }
            else if (sscanf(start, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
                // found one of the delta lines
                if (ddx != 0.0) { dx = ddx; }
                else if (ddy != 0.0) { dy = ddy; }
                else if (ddz != 0.0) { dz = ddz; }
            }
            else if (sscanf(start, "object %d class array type %s rank 0 items %d data follows", &dummy, type, &npts) == 3) {
                if (isrect && (npts != nx*ny*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, npts);
                    return result.error(mesg);
                }
                else if (!isrect && (npts != nxy*nz)) {
                    char mesg[256];
                    sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, npts);
                    return result.error(mesg);
                }
                break;
            }
        }
    } while (!fin.eof());

    // read data points
    if (!fin.eof()) {
        if (isrect) {
            Rappture::Mesh1D xgrid(x0, x0+nx*dx, nx);
            Rappture::Mesh1D ygrid(y0, y0+ny*dy, ny);
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldRect3D field(xgrid, ygrid, zgrid);

            double dval;
            int nread = 0;
            for (int ix=0; ix < nx; ix++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int iz=0; iz < nz; iz++) {
                        if (fin.eof() || nread > npts) {
                            break;
                        }
                        fin.getline(line,sizeof(line)-1);
                        if (sscanf(line, "%lg", &dval) == 1) {
                            int nindex = iz*nx*ny + iy*nx + ix;
                            field.define(nindex, dval);
                            nread++;
                        }
                    }
                }
            }

            // make sure that we read all of the expected points
            if (nread != nx*ny*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nx*ny*nz, nread);
                result.error(mesg);
                return result;
            }

            // figure out a good mesh spacing
            int nsample = 30;
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);

#ifndef NV40
            // must be an even power of 2 for older cards
	        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
	        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;
                        ngen += 4;
                    }
                }
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-1];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+1];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

            load_volume(index, nx, ny, nz, 4, data);
            delete [] data;

        } else {
            Rappture::Mesh1D zgrid(z0, z0+nz*dz, nz);
            Rappture::FieldPrism3D field(xymesh, zgrid);

            double dval;
            int nread = 0;
            while (!fin.eof() && nread < npts) {
                if (!(fin >> dval).fail()) {
                    field.define(nread++, dval);
                }
            }

            // make sure that we read all of the expected points
            if (nread != nxy*nz) {
                char mesg[256];
                sprintf(mesg,"inconsistent data: expected %d points but found %d points", nxy*nz, nread);
                return result.error(mesg);
            }

            // figure out a good mesh spacing
            int nsample = 30;
            x0 = field.rangeMin(Rappture::xaxis);
            dx = field.rangeMax(Rappture::xaxis) - field.rangeMin(Rappture::xaxis);
            y0 = field.rangeMin(Rappture::yaxis);
            dy = field.rangeMax(Rappture::yaxis) - field.rangeMin(Rappture::yaxis);
            z0 = field.rangeMin(Rappture::zaxis);
            dz = field.rangeMax(Rappture::zaxis) - field.rangeMin(Rappture::zaxis);
            double dmin = pow((dx*dy*dz)/(nsample*nsample*nsample), 0.333);

            nx = (int)ceil(dx/dmin);
            ny = (int)ceil(dy/dmin);
            nz = (int)ceil(dz/dmin);
#ifndef NV40
            // must be an even power of 2 for older cards
	        nx = (int)pow(2.0, ceil(log10((double)nx)/log10(2.0)));
	        ny = (int)pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	        nz = (int)pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);

                        // scale all values [0-1], -1 => out of bounds
                        v = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen] = v;

                        ngen += 4;
                    }
                }
            }

            // Compute the gradient of this data.  BE CAREFUL: center
            // calculation on each node to avoid skew in either direction.
            ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                for (int iy=0; iy < ny; iy++) {
                    for (int ix=0; ix < nx; ix++) {
                        // gradient in x-direction
                        double valm1 = (ix == 0) ? 0.0 : data[ngen-1];
                        double valp1 = (ix == nx-1) ? 0.0 : data[ngen+1];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+1] = 0.0;
                        } else {
                            data[ngen+1] = valp1-valm1; // assume dx=1
                        }

                        // gradient in y-direction
                        valm1 = (iy == 0) ? 0.0 : data[ngen-4*nx];
                        valp1 = (iy == ny-1) ? 0.0 : data[ngen+4*nx];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+2] = 0.0;
                        } else {
                            data[ngen+2] = valp1-valm1; // assume dy=1
                        }

                        // gradient in z-direction
                        valm1 = (iz == 0) ? 0.0 : data[ngen-4*nx*ny];
                        valp1 = (iz == nz-1) ? 0.0 : data[ngen+4*nx*ny];
                        if (valm1 < 0 || valp1 < 0) {
                            data[ngen+3] = 0.0;
                        } else {
                            data[ngen+3] = valp1-valm1; // assume dz=1
                        }

                        ngen += 4;
                    }
                }
            }

            load_volume(index, nx, ny, nz, 4, data);
            delete [] data;
        }
    } else {
        char mesg[256];
        sprintf(mesg,"data not found in file %s", fname);
        return result.error(mesg);
    }

    //
    // Center this new volume on the origin.
    //
    float dx0 = -0.5;
    float dy0 = -0.5*dy/dx;
    float dz0 = -0.5*dz/dx;
    volume[index]->move(Vector3(dx0, dy0, dz0));

    return result;
}

/* Load a 3D volume
 * index: the index into the volume array, which stores pointers to 3D volume instances
 * data: pointer to an array of floats.  
 * n_component: the number of scalars for each space point. 
 * 		All component scalars for a point are placed consequtively in data array 
 * width, height and depth: number of points in each dimension
 */
void load_volume(int index, int width, int height, int depth, int n_component, float* data){
  if(volume[index]!=0){
    delete volume[index];
    volume[index]=0;
  }

  volume[index] = new Volume(0.f, 0.f, 0.f, width, height, depth, 1.,  n_component, data);
  assert(volume[index]!=0);

  if (n_volumes <= index) {
    n_volumes = index+1;
  }
}


// get a colormap 1D texture by name
TransferFunction*
get_transfunc(char *name) {
    Tcl_HashEntry *entryPtr;
    entryPtr = Tcl_FindHashEntry(&tftable, name);
    if (entryPtr) {
        return (TransferFunction*)Tcl_GetHashValue(entryPtr);
    }
    return NULL;
}


//Update the transfer function using local GUI in the non-server mode
extern void update_tf_texture(){
  glutSetWindow(render_window);

  //fprintf(stderr, "tf update\n");
  TransferFunction *tf = get_transfunc("default");
  if (tf == NULL) return;

  float data[256*4];
  for(int i=0; i<256; i++){
    data[4*i+0] = color_table[i][0];
    data[4*i+1] = color_table[i][1];
    data[4*i+2] = color_table[i][2];
    data[4*i+3] = color_table[i][3];
    //fprintf(stderr, "(%f,%f,%f,%f) ", data[4*i+0], data[4*i+1], data[4*i+2], data[4*i+3]);
  }

  tf->update(data);

#ifdef EVENTLOG
  float param[3] = {0,0,0};
  Event* tmp = new Event(EVENT_ROTATE, param, get_time_interval());
  tmp->write(event_log);
  delete tmp;
#endif
}


//initialize frame buffer objects for offscreen rendering
void init_offscreen_buffer(){

  //initialize final fbo for final display
  glGenFramebuffersEXT(1, &final_fbo);
  glGenTextures(1, &final_color_tex);
  glGenRenderbuffersEXT(1, &final_depth_rb);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);

  //initialize final color texture
  glBindTexture(GL_TEXTURE_2D, final_color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef NV40
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
#else
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
#endif
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, final_color_tex, 0);

	
  // initialize final depth renderbuffer
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                           GL_DEPTH_COMPONENT24, win_width, win_height);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                               GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, final_depth_rb);

  // Check framebuffer completeness at the end of initialization.
  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);
}


//resize the offscreen buffer 
void resize_offscreen_buffer(int w, int h){
  win_width = w;
  win_height = h;

  //fprintf(stderr, "screen_buffer size: %d\n", sizeof(screen_buffer));

  if (screen_buffer) {
      delete[] screen_buffer;
      screen_buffer = NULL;
  }
  screen_buffer = new unsigned char[4*win_width*win_height];
  assert(screen_buffer != NULL);

  //delete the current render buffer resources
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
  glDeleteTextures(1, &final_color_tex);
  glDeleteFramebuffersEXT(1, &final_fbo);

  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
  glDeleteRenderbuffersEXT(1, &final_depth_rb);
fprintf(stdin,"  after glDeleteRenderbuffers\n");

  //change the camera setting
  cam->set_screen_size(win_width, win_height);
  plane_render->set_screen_size(win_width, win_height);

  //Reinitialize final fbo for final display
  glGenFramebuffersEXT(1, &final_fbo);
  glGenTextures(1, &final_color_tex);
  glGenRenderbuffersEXT(1, &final_depth_rb);
fprintf(stdin,"  after glGenRenderbuffers\n");

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);

  //initialize final color texture
  glBindTexture(GL_TEXTURE_2D, final_color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef NV40
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
#else
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
#endif
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, final_color_tex, 0);
fprintf(stdin,"  after glFramebufferTexture2DEXT\n");
	
  // initialize final depth renderbuffer
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, final_depth_rb);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                           GL_DEPTH_COMPONENT24, win_width, win_height);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                               GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, final_depth_rb);
fprintf(stdin,"  after glFramebufferRenderEXT\n");

  // Check framebuffer completeness at the end of initialization.
  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);
fprintf(stdin,"  after assert\n");
}


void init_cg(){
    cgSetErrorCallback(cgErrorCallback);
    g_context = cgCreateContext();

#ifdef NEW_CG
    m_posvel_fprog = loadProgram(g_context, CG_PROFILE_FP40, CG_SOURCE, "./shaders/update_pos_vel.cg");
    m_posvel_timestep_param  = cgGetNamedParameter(m_posvel_fprog, "timestep");
    m_posvel_damping_param   = cgGetNamedParameter(m_posvel_fprog, "damping");
    m_posvel_gravity_param   = cgGetNamedParameter(m_posvel_fprog, "gravity");
    m_posvel_spherePos_param = cgGetNamedParameter(m_posvel_fprog, "spherePos");
    m_posvel_sphereVel_param = cgGetNamedParameter(m_posvel_fprog, "sphereVel");
#endif
}


//switch shader to change render mode
void switch_shader(int choice){

  switch (choice){
    case 0:
      break;

   case 1:
      break;
      
   default:
      break;
  }
}

void init_particles(){
  //random placement on a slice
  float* data = new float[psys->psys_width * psys->psys_height * 4];
  bzero(data, sizeof(float)*4* psys->psys_width * psys->psys_height);

  for (int i=0; i<psys->psys_width; i++){ 
    for (int j=0; j<psys->psys_height; j++){ 
      int index = i + psys->psys_height*j;
      bool particle = rand() % 256 > 150; 
      //particle = true;
      if(particle) /*&& i/float(psys->psys_width)>0.3 && i/float(psys->psys_width)<0.7 
		      && j/float(psys->psys_height)>0.1 && j/float(psys->psys_height)<0.4)*/
      {
	//assign any location (x,y,z) in range [0,1]
        data[4*index] = lic_slice_x;
	data[4*index+1]= j/float(psys->psys_height);
	data[4*index+2]= i/float(psys->psys_width);
	data[4*index+3]= 30; //shorter life span, quicker iterations	
      }
      else
      {
        data[4*index] = 0;
	data[4*index+1]= 0;
	data[4*index+2]= 0;
	data[4*index+3]= 0;	
      }
    }
   }

  psys->initialize((Particle*)data);
  delete[] data;
}


//init line integral convolution
void init_lic(){
  lic = new Lic(NMESH, win_width, win_height, 0.3, g_context, volume[1]->id, 
		  volume[1]->aspect_ratio_width,
		  volume[1]->aspect_ratio_height,
		  volume[1]->aspect_ratio_depth);
}

//init the particle system using vector field volume->[1]
void init_particle_system(){
   psys = new ParticleSystem(NMESH, NMESH, g_context, volume[1]->id,
		   1./volume[1]->aspect_ratio_width,
		   1./volume[1]->aspect_ratio_height,
		   1./volume[1]->aspect_ratio_depth);

   init_particles();	//fill initial particles
}


void make_test_2D_data(){

  int w = 300;
  int h = 200;
  float* data = new float[w*h];

  //procedurally make a gradient plane
  for(int j=0; j<h; j++){
    for(int i=0; i<w; i++){
      data[w*j+i] = float(i)/float(w);
    }
  }

  plane[0] = new Texture2D(w, h, GL_FLOAT, GL_LINEAR, 1, data);

  delete[] data;
}


/*----------------------------------------------------*/
void initGL(void) 
{ 
   system_info();
   init_glew();

   //buffer to store data read from the screen
   if (screen_buffer) {
       delete[] screen_buffer;
       screen_buffer = NULL;
   }
   screen_buffer = new unsigned char[4*win_width*win_height];
   assert(screen_buffer != NULL);

   //create the camera with default setting
   cam = new Camera(win_width, win_height, 
		   live_obj_x, live_obj_y, live_obj_z,
		   0., 0., 100.,
		   (int)live_rot_x, (int)live_rot_y, (int)live_rot_z);

   glEnable(GL_TEXTURE_2D);
   glShadeModel(GL_FLAT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glClearColor(0,0,0,1);
   glClear(GL_COLOR_BUFFER_BIT);

   //initialize lighting
   GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
   GLfloat mat_shininess[] = {30.0};
   GLfloat white_light[] = {1.0, 1.0, 1.0, 1.0};
   GLfloat green_light[] = {0.1, 0.5, 0.1, 1.0};

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
   glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);	
   glLightfv(GL_LIGHT1, GL_DIFFUSE, green_light);
   glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);	

   //init volume array
   for(int i=0; i<MAX_N_VOLUMES; i++){
     volume[i] = 0;
   }

   // init table of transfer functions
   Tcl_InitHashTable(&tftable, TCL_STRING_KEYS);

   //check if performance query is supported
   if(check_query_support()){
     //create queries to count number of rendered pixels
     perf = new PerfQuery(); 
   }

   //load_volume_file(0, "./data/A-apbs-2-out-potential-PE0.dx");
   //load_volume_file(0, "./data/nw-AB-Vg=0.000-Vd=1.000-potential.dx");
   //load_volume_file(0, "./data/test2.dx");
   //load_volume_file(0, "./data/mu-wire-3d.dx"); //I added this line to debug: Wei
   //load_volume_file(0, "./data/input_nd_dx_4"); //take a VERY long time?
   //load_vector_file(1, "./data/J-wire-vec.dx");
   //load_volume_file(1, "./data/mu-wire-3d.dx");  //I added this line to debug: Wei
   //load_volume_file(3, "./data/mu-wire-3d.dx");
   //load_volume_file(4, "./data/mu-wire-3d.dx");

   init_offscreen_buffer();    //frame buffer object for offscreen rendering
   init_cg();	//create cg shader context 

   //create volume renderer and add volumes to it
   vol_render = new VolumeRenderer(g_context);

   
   /*
   //I added this to debug : Wei
   float tmp_data[4*124];
   memset(tmp_data, 0, 4*4*124);
   TransferFunction* tmp_tf = new TransferFunction(124, tmp_data);
   vol_render->add_volume(volume[0], tmp_tf);
   volume[0]->get_cutplane(0)->enabled = false;
   volume[0]->get_cutplane(1)->enabled = false;
   volume[0]->get_cutplane(2)->enabled = false;

   //volume[1]->move(Vector3(0.5, 0.6, 0.7));
   //vol_render->add_volume(volume[1], tmp_tf);
   */


   //create an 2D plane renderer
   plane_render = new PlaneRenderer(g_context, win_width, win_height);
   make_test_2D_data();

   plane_render->add_plane(plane[0], get_transfunc("default"));

   assert(glGetError()==0);

   //init_particle_system();
   //init_lic(); 
}



void initTcl(){
    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);

    Tcl_DStringInit(&cmdbuffer);

    // manipulate the viewpoint
    Tcl_CreateCommand(interp, "camera", CameraCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // turn on/off cut planes in x/y/z directions
    Tcl_CreateCommand(interp, "cutplane", CutplaneCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // change the size of the screen (size of picture generated)
    Tcl_CreateCommand(interp, "screen", ScreenCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // manipulate transfer functions
    Tcl_CreateCommand(interp, "transfunc", TransfuncCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // manipulate volume data
    Tcl_CreateCommand(interp, "volume", VolumeCmd,
        (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    // create a default transfer function
    if (Tcl_Eval(interp, def_transfunc) != TCL_OK) {
        fprintf(stdin, "WARNING: bad default transfer function\n");
        fprintf(stdin, Tcl_GetStringResult(interp));
    }


  //link an EXISTING volume and transfer function to the volume renderer. 
  //Only after being added, can a volume be rendered. This command does not create a volume nor a transfer function
  Tcl_CreateCommand(interp, "volume_link", VolumeLinkCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //move an volume
  Tcl_CreateCommand(interp, "volume_move", VolumeMoveCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //resize volume 
  Tcl_CreateCommand(interp, "volume_resize", VolumeResizeCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);

  //refresh the screen (render again)
  Tcl_CreateCommand(interp, "refresh", RefreshCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
}


void read_screen(){
  //glBindTexture(GL_TEXTURE_2D, 0);
  //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
 
  glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);
  assert(glGetError()==0);
}

void display();


char rle[512*512*3];
int rle_size;

short offsets[512*512*3];
int offsets_size;

void do_rle(){
  int len = win_width*win_height*3;
  rle_size = 0;
  offsets_size = 0;

  int i=0;
  while(i<len){
    if (screen_buffer[i] == 0) {
      int pos = i+1;
      while ( (pos<len) && (screen_buffer[pos] == 0)) {
        pos++;
      }
      offsets[offsets_size++] = -(pos - i);
      i = pos;
    }

    else {
      int pos;
      for (pos = i; (pos<len) && (screen_buffer[pos] != 0); pos++) {
        rle[rle_size++] = screen_buffer[pos];
      }
      offsets[offsets_size++] = (pos - i);
      i = pos;
    }

  }
}

// used internally to build up the BMP file header
// writes an integer value into the header data structure at pos
void
bmp_header_add_int(unsigned char* header, int& pos, int data)
{
    header[pos++] = data & 0xff;
    header[pos++] = (data >> 8) & 0xff;
    header[pos++] = (data >> 16) & 0xff;
    header[pos++] = (data >> 24) & 0xff;
}


void xinetd_listen(){

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
    display();

#if DO_RLE
    do_rle();
    int sizes[2] = {  offsets_size*sizeof(offsets[0]), rle_size }; 
    fprintf(stderr, "Writing %d,%d\n", sizes[0], sizes[1]); fflush(stderr);
    write(0, &sizes, sizeof(sizes));
    write(0, offsets, offsets_size*sizeof(offsets[0]));
    write(0, rle, rle_size);	//unsigned byte
#else
    unsigned char header[54];
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    int fsize = win_width*win_height*3 + sizeof(header);
    bmp_header_add_int(header, pos, fsize);

    // reserved value (must be 0)
    bmp_header_add_int(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmp_header_add_int(header, pos, sizeof(header));

    // size of the BITMAPINFOHEADER
    bmp_header_add_int(header, pos, 40);

    // width of the image in pixels
    bmp_header_add_int(header, pos, win_width);

    // height of the image in pixels
    bmp_header_add_int(header, pos, win_height);

    // 1 plane + 24 bits/pixel << 16
    bmp_header_add_int(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmp_header_add_int(header, pos, 0);
    bmp_header_add_int(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    for (int i=0; i < 3*win_width*win_height; i += 3) {
        unsigned char tmp = screen_buffer[i+2];
        screen_buffer[i+2] = screen_buffer[i];
        screen_buffer[i] = tmp;
    }

    std::ostringstream result;
    result << "nv>image -bytes " << fsize << "\n";
    write(0, result.str().c_str(), result.str().size());

    write(0, header, sizeof(header));
    write(0, screen_buffer, 3*win_width*win_height);
#endif
}


/*
//draw vectors 
void draw_arrows(){
  glColor4f(0.,0.,1.,1.);
  for(int i=0; i<NMESH; i++){
    for(int j=0; j<NMESH; j++){
      Vector2 v = grid.get(i, j);

      int x1 = i*DM;
      int y1 = j*DM;

      int x2 = x1 + v.x;
      int y2 = y1 + v.y;

      glBegin(GL_LINES);
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
      glEnd();
    }
  }
}
*/


/*----------------------------------------------------*/
void idle(){
  glutSetWindow(render_window);

  
  /*
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 300000000;
  nanosleep(&ts, 0);
  */
  

#ifdef XINETD
  xinetd_listen();
#else
  glutPostRedisplay();
#endif
}


void display_offscreen_buffer(){
   glEnable(GL_TEXTURE_2D);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glBindTexture(GL_TEXTURE_2D, final_color_tex);

   glViewport(0, 0, win_width, win_height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, win_width, 0, win_height);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(1.,1.,1.);		//MUST HAVE THIS LINE!!!
   glBegin(GL_QUADS);
   glTexCoord2f(0, 0); glVertex2f(0, 0);
   glTexCoord2f(1, 0); glVertex2f(win_width, 0);
   glTexCoord2f(1, 1); glVertex2f(win_width, win_height);
   glTexCoord2f(0, 1); glVertex2f(0, win_height);
   glEnd();
}

void offscreen_buffer_capture(){
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
}

void draw_bounding_box(float x0, float y0, float z0,
		float x1, float y1, float z1,
		float r, float g, float b, float line_width)
{
	glDisable(GL_TEXTURE_2D);

	glColor4d(r, g, b, 1.0);
	glLineWidth(line_width);
	
	glBegin(GL_LINE_LOOP);

		glVertex3d(x0, y0, z0);
		glVertex3d(x1, y0, z0);
		glVertex3d(x1, y1, z0);
		glVertex3d(x0, y1, z0);
		
	glEnd();

	glBegin(GL_LINE_LOOP);

		glVertex3d(x0, y0, z1);
		glVertex3d(x1, y0, z1);
		glVertex3d(x1, y1, z1);
		glVertex3d(x0, y1, z1);
		
	glEnd();


	glBegin(GL_LINE_LOOP);

		glVertex3d(x0, y0, z0);
		glVertex3d(x0, y0, z1);
		glVertex3d(x0, y1, z1);
		glVertex3d(x0, y1, z0);
		
	glEnd();

	glBegin(GL_LINE_LOOP);

		glVertex3d(x1, y0, z0);
		glVertex3d(x1, y0, z1);
		glVertex3d(x1, y1, z1);
		glVertex3d(x1, y1, z0);
		
	glEnd();

	glEnable(GL_TEXTURE_2D);
}



int particle_distance_sort(const void* a, const void* b){
  if((*((Particle*)a)).aux > (*((Particle*)b)).aux)
    return -1;
  else
    return 1;
}


void soft_read_verts(){
  glReadPixels(0, 0, psys->psys_width, psys->psys_height, GL_RGB, GL_FLOAT, vert);
  //fprintf(stderr, "soft_read_vert");

  //cpu sort the distance  
  Particle* p = (Particle*) malloc(sizeof(Particle)* psys->psys_width * psys->psys_height);
  for(int i=0; i<psys->psys_width * psys->psys_height; i++){
    float x = vert[3*i];
    float y = vert[3*i+1];
    float z = vert[3*i+2];

    float dis = (x-live_obj_x)*(x-live_obj_x) + (y-live_obj_y)*(y-live_obj_y) + (z-live_obj_z)*(z-live_obj_z); 
    p[i].x = x;
    p[i].y = y;
    p[i].z = z;
    p[i].aux = dis;
  }

  qsort(p, psys->psys_width * psys->psys_height, sizeof(Particle), particle_distance_sort);

  for(int i=0; i<psys->psys_width * psys->psys_height; i++){
    vert[3*i] = p[i].x;
    vert[3*i+1] = p[i].y;
    vert[3*i+2] = p[i].z;
  }

  free(p);
}


//display the content of a texture as a screen aligned quad
void display_texture(NVISid tex, int width, int height){
   glPushMatrix();

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_RECTANGLE_NV, tex);

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, width, 0, height);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   cgGLBindProgram(m_passthru_fprog);
   cgGLEnableProfile(CG_PROFILE_FP30);

   cgGLSetParameter4f(m_passthru_scale_param, 1.0, 1.0, 1.0, 1.0);
   cgGLSetParameter4f(m_passthru_bias_param, 0.0, 0.0, 0.0, 0.0);
   
   draw_quad(width, height, width, height);
   cgGLDisableProfile(CG_PROFILE_FP30);

   glPopMatrix();

   assert(glGetError()==0);
}


//draw vertices in the main memory
void soft_display_verts(){
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glPointSize(0.5);
  glColor4f(0,0.8,0.8,0.6);
  glBegin(GL_POINTS);
  for(int i=0; i<psys->psys_width * psys->psys_height; i++){
    glVertex3f(vert[3*i], vert[3*i+1], vert[3*i+2]);
  }
  glEnd();
  //fprintf(stderr, "soft_display_vert");
}


#if 0

//oddeven sort on GPU
void sortstep()
{
    // perform one step of the current sorting algorithm

	/*
    // swap buffers
    int sourceBuffer = targetBuffer;
    targetBuffer = (targetBuffer+1)%2;   
    int pstage = (1<<stage);
    int ppass  = (1<<pass);
    int activeBitonicShader = 0;

#ifdef _WIN32
    buffer->BindBuffer(wglTargets[sourceBuffer]);
#else
    buffer->BindBuffer(glTargets[sourceBuffer]);
#endif
    if (buffer->IsDoubleBuffered()) glDrawBuffer(glTargets[targetBuffer]);
    */

    checkGLError("after db");

    int pstage = (1<<stage);
    int ppass  = (1<<pass);
    //int activeBitonicShader = 0;

    // switch on correct sorting shader
    oddevenMergeSort.bind();
    glUniform3fARB(oddevenMergeSort.getUniformLocation("Param1"), float(pstage+pstage), 
		   float(ppass%pstage), float((pstage+pstage)-(ppass%pstage)-1));
    glUniform3fARB(oddevenMergeSort.getUniformLocation("Param2"), float(psys_width), float(psys_height), float(ppass));
    glUniform1iARB(oddevenMergeSort.getUniformLocation("Data"), 0);
    staticdebugmsg("sort","stage "<<pstage<<" pass "<<ppass);

    // This clear is not necessary for sort to function. But if we are in interactive mode 
    // unused parts of the texture that are visible will look bad.
    //if (!perfTest) glClear(GL_COLOR_BUFFER_BIT);

    //buffer->Bind();
    //buffer->EnableTextureTarget();

    // initiate the sorting step on the GPU
    // a full-screen quad
    glBegin(GL_QUADS);
    /*
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f); glVertex2f(-1.0f,-1.0f);	
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f); glVertex2f(1.0f,-1.0f);
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f); glVertex2f(1.0f,1.0f);	
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f); glVertex2f(-1.0f,1.0f);	
    */
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,0.0f,0.0f,1.0f); glVertex2f(0.,0.);	
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),0.0f,1.0f,1.0f); glVertex2f(float(psys_width), 0.);
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,float(psys_width),float(psys_height),1.0f,0.0f); glVertex2f(float(psys_width), float(psys_height));	
    glMultiTexCoord4fARB(GL_TEXTURE0_ARB,0.0f,float(psys_height),0.0f,0.0f); glVertex2f(0., float(psys_height));	
    glEnd();


    // switch off sorting shader
    oddevenMergeSort.release();

    //buffer->DisableTextureTarget();

    assert(glGetError()==0);
}

#endif


void draw_3d_axis(){
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);

 	//draw axes
	GLUquadric *obj;
	obj = gluNewQuadric();
	
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	int segments = 50;

	glColor3f(0.8, 0.8, 0.8);
	glPushMatrix();
	glTranslatef(0.4, 0., 0.);
	glRotatef(90, 1, 0, 0);
	glRotatef(180, 0, 1, 0);
	glScalef(0.0005, 0.0005, 0.0005);
	glutStrokeCharacter(GLUT_STROKE_ROMAN, 'x');
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0.4, 0.);
	glRotatef(90, 1, 0, 0);
	glRotatef(180, 0, 1, 0);
	glScalef(0.0005, 0.0005, 0.0005);
	glutStrokeCharacter(GLUT_STROKE_ROMAN, 'y');
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0., 0.4);
	glRotatef(90, 1, 0, 0);
	glRotatef(180, 0, 1, 0);
	glScalef(0.0005, 0.0005, 0.0005);
	glutStrokeCharacter(GLUT_STROKE_ROMAN, 'z');
	glPopMatrix();	

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glColor3f(0.2, 0.2, 0.8);
	glPushMatrix();
	glutSolidSphere(0.02, segments, segments );
	glPopMatrix();

	glPushMatrix();
	glRotatef(-90, 1, 0, 0);	
	gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0.3, 0.);
	glRotatef(-90, 1, 0, 0);	
	gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
	glPopMatrix();	

	glPushMatrix();
	glRotatef(90, 0, 1, 0);
	gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0.3, 0., 0.);
	glRotatef(90, 0, 1, 0);	
	gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
	glPopMatrix();	

	glPushMatrix();
	gluCylinder(obj, 0.01, 0.01, 0.3, segments, segments);
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0., 0.3);
	gluCylinder(obj, 0.02, 0.0, 0.06, segments, segments);
	glPopMatrix();	

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	gluDeleteQuadric(obj);

  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
}


void draw_axis(){

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);

  //red x
  glColor3f(1,0,0);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(1.5,0,0);
  glEnd();

  //blue y
  glColor3f(0,0,1);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,1.5,0);
  glEnd();

  //green z
  glColor3f(0,1,0);
  glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,0,1.5);
  glEnd();

  glEnable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
}



/*----------------------------------------------------*/
void display()
{

   assert(glGetError()==0);

   //lic->convolve(); //flow line integral convolution
   //psys->advect(); //advect particles

   offscreen_buffer_capture();  //enable offscreen render
   //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); //enable onscreen render

   //start final rendering
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

   if(volume_mode){ //3D rendering mode
     glEnable(GL_TEXTURE_2D);
     glEnable(GL_DEPTH_TEST);

     //camera setting activated
     cam->activate();

     //now render things in the scene
     //
     draw_3d_axis();
   
     //lic->render(); 	//display the line integral convolution result
     //soft_display_verts();
     //perf->enable();
     //  psys->render();
     //perf->disable();
     //fprintf(stderr, "particle pixels: %d\n", perf->get_pixel_count());
     //perf->reset();
    
     perf->enable();
       vol_render->render_all();
       //fprintf(stderr, "%lf\n", get_time_interval());
     perf->disable();
   }
   else{ //2D rendering mode

     perf->enable();
       plane_render->render();
     perf->disable();
   }


#ifdef XINETD
   float cost  = perf->get_pixel_count();
   write(3, &cost, sizeof(cost));
#endif
   perf->reset();

   display_offscreen_buffer(); //display the final rendering on screen

#ifdef XINETD
   read_screen();
#else
   //read_screen();
#endif   

   glutSwapBuffers(); 
}


void mouse(int button, int state, int x, int y){
  if(button==GLUT_LEFT_BUTTON){

    if(state==GLUT_DOWN){
      left_last_x = x;
      left_last_y = y;
      left_down = true;
      right_down = false;
    }
    else{
      left_down = false;
      right_down = false;
    }
  }
  else{
    //fprintf(stderr, "right mouse\n");

    if(state==GLUT_DOWN){
      //fprintf(stderr, "right mouse down\n");
      right_last_x = x;
      right_last_y = y;
      left_down = false;
      right_down = true;
    }
    else{
      //fprintf(stderr, "right mouse up\n");
      left_down = false;
      right_down = false;
    }
  }
}


void update_rot(int delta_x, int delta_y){
	live_rot_x += delta_x;
	live_rot_y += delta_y;

	if(live_rot_x > 360.0)
		live_rot_x -= 360.0;	
	else if(live_rot_x < -360.0)
		live_rot_x += 360.0;

	if(live_rot_y > 360.0)
		live_rot_y -= 360.0;	
	else if(live_rot_y < -360.0)
		live_rot_y += 360.0;

	cam->rotate(live_rot_x, live_rot_y, live_rot_z);
}


void update_trans(int delta_x, int delta_y, int delta_z){
        live_obj_x += delta_x*0.03;
        live_obj_y += delta_y*0.03;
        live_obj_z += delta_z*0.03;
}

void end_service();

void keyboard(unsigned char key, int x, int y){
  
   bool log = false;

   switch (key){
	case 'q':
#ifdef XINETD
                end_service();
#endif
		exit(0);
		break;
	case '+':
		lic_slice_z+=0.05;
		lic->set_offset(lic_slice_z);
		break;
	case '-':
		lic_slice_z-=0.05;
		lic->set_offset(lic_slice_z);
		break;
	case ',':
		lic_slice_x+=0.05;
		init_particles();
		break;
	case '.':
		lic_slice_x-=0.05;
		init_particles();
		break;
	case '1':
		advect = true;
		break;
	case '2':
		psys_x+=0.05;
		break;
	case '3':
		psys_x-=0.05;
		break;
	case 'w': //zoom out
		live_obj_z-=0.05;
		log = true;
		cam->move(live_obj_x, live_obj_y, live_obj_z);
		break;
	case 's': //zoom in
		live_obj_z+=0.05;
		log = true;
		cam->move(live_obj_x, live_obj_y, live_obj_z);
		break;
	case 'a': //left
		live_obj_x-=0.05;
		log = true;
		cam->move(live_obj_x, live_obj_y, live_obj_z);
		break;
	case 'd': //right
		live_obj_x+=0.05;
		log = true;
		cam->move(live_obj_x, live_obj_y, live_obj_z);
		break;
	case 'i':
		init_particles();
		break;
	case 'v':
		vol_render->switch_volume_mode();
		break;
	case 'b':
		vol_render->switch_slice_mode();
		break;
	case 'n':
		resize_offscreen_buffer(win_width*2, win_height*2);
		break;
	case 'm':
		resize_offscreen_buffer(win_width/2, win_height/2);
		break;

	default:
		break;
    }	

#ifdef EVENTLOG
   if(log){
     float param[3] = {live_obj_x, live_obj_y, live_obj_z};
     Event* tmp = new Event(EVENT_MOVE, param, get_time_interval());
     tmp->write(event_log);
     delete tmp;
   }
#endif
}

void motion(int x, int y){

    int old_x, old_y;	

    if(left_down){
      old_x = left_last_x;
      old_y = left_last_y;   
    }
    else if(right_down){
      old_x = right_last_x;
      old_y = right_last_y;   
    }

    int delta_x = x - old_x;
    int delta_y = y - old_y;

    //more coarse event handling
    //if(abs(delta_x)<10 && abs(delta_y)<10)
      //return;

    if(left_down){
      left_last_x = x;
      left_last_y = y;

      update_rot(-delta_y, -delta_x);
    }
    else if (right_down){
      //fprintf(stderr, "right mouse motion (%d,%d)\n", x, y);

      right_last_x = x;
      right_last_y = y;

      update_trans(0, 0, delta_x);
    }

#ifdef EVENTLOG
    float param[3] = {live_rot_x, live_rot_y, live_rot_z};
    Event* tmp = new Event(EVENT_ROTATE, param, get_time_interval());
    tmp->write(event_log);
    delete tmp;
#endif

    glutPostRedisplay();
}

#ifdef XINETD
void init_service(){
  //open log and map stderr to log file
  xinetd_log = fopen("log.txt", "w");
  close(2);
  dup2(fileno(xinetd_log), 2);
  dup2(2,1);

  //flush junk 
  fflush(stdout);
  fflush(stderr);
}

void end_service(){
  //close log file
  fclose(xinetd_log);
}
#endif

void init_event_log(){
  event_log = fopen("event.txt", "w");
  assert(event_log!=0);

  struct timeval time;
  gettimeofday(&time, NULL);
  cur_time = time.tv_sec*1000. + time.tv_usec/1000.;
}

void end_event_log(){
  fclose(event_log);
}

double get_time_interval(){
  struct timeval time;
  gettimeofday(&time, NULL);
  double new_time = time.tv_sec*1000. + time.tv_usec/1000.;

  double interval = new_time - cur_time;
  cur_time = new_time;
  return interval;
}


/*----------------------------------------------------*/
int main(int argc, char** argv) 
{ 
#ifdef XINETD
   init_service();
#endif

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); 
   
   glutInitWindowSize(win_width, win_height);
   glutInitWindowPosition(10, 10);
   render_window = glutCreateWindow(argv[0]);
   glutDisplayFunc(display);

#ifndef XINETD
   //MainTransferFunctionWindow * tf_window;
   //tf_window = new MainTransferFunctionWindow();
   //tf_window->mainInit();

   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutKeyboardFunc(keyboard);
#endif
   glutIdleFunc(idle);
   glutReshapeFunc(resize_offscreen_buffer);

   initGL();
   initTcl();

#ifdef EVENTLOG
   init_event_log();
#endif
   //event loop
   glutMainLoop();
#ifdef EVENTLOG
   end_event_log();
#endif

#ifdef XINETD
   end_service();
#endif

   return 0;
}
