/*
 * ----------------------------------------------------------------------
 * Nanovis: Visualization of Nanoelectronics Data
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

#include <stdio.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/time.h>

#include "nanovis.h"
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
Camera* cam;

float color_table[256][4]; 	

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

ParticleSystem* psys;
float psys_x=0.4, psys_y=0, psys_z=0;

Lic* lic;

unsigned char* screen_buffer = new unsigned char[3*win_width*win_height+1];	//buffer to store data read from the screen
NVISid final_fbo, final_color_tex, final_depth_rb;      //frame buffer for final rendering
NVISid vel_fbo, slice_vector_tex;	//for projecting 3d vector to 2d vector on a plane

bool advect=false;
float vert[NMESH*NMESH*3];		//particle positions in main memory
float slice_vector[NMESH*NMESH*4];	//per slice vectors in main memory

int n_volumes = 0;
Volume* volume[MAX_N_VOLUMES];		//point to volumes, currently handle up to 10 volumes
TransferFunction* tf[MAX_N_VOLUMES];	//transfer functions, currently handle up to 10 colormaps

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

static void set_camera(float x_angle, float y_angle, float z_angle){
  live_rot_x = x_angle;
  live_rot_y = y_angle;
  live_rot_z = z_angle;
}


// Tcl interpreter for incoming messages
static Tcl_Interp *interp;

static int CameraCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ClearCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int SizeCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int OutlineCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CutCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int HelloCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int LoadCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int RefreshCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int MoveCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));

//Tcl callback functions
static int
CameraCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{

	fprintf(stderr, "camera cmd\n");
	double x, y, z;

	if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" x_angle y_angle z_angle\"", (char*)NULL);
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[1], &x) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[2], &y) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[3], &z) != TCL_OK) {
		return TCL_ERROR;
	}
	set_camera(x, y, z);
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


static int
MoveCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{

	fprintf(stderr, "move cmd\n");
	double x, y, z;

	if (argc != 4) {
		Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			" x_angle y_angle z_angle\"", (char*)NULL);
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[1], &x) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[2], &y) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetDouble(interp, argv[3], &z) != TCL_OK) {
		return TCL_ERROR;
	}
	set_object(x, y, z);
	return TCL_OK;
}


static int
HelloCmd(ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[])
{
	//send back 4 bytes as confirmation
	char ack[4]="ACK";
        fprintf(stderr, "wrote %d\n", write(fileno(stdout), ack, 4));
	fflush(stdout);
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


void load_volume(int index, int width, int height, int depth, int n_component, float* data);

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
	nx = pow(2.0, ceil(log10((double)nx)/log10(2.0)));  // must be an even power of 2
	ny = pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	nz = pow(2.0, ceil(log10((double)nz)/log10(2.0)));
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
void
load_volume_file(int index, char *fname) {
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

                std::ofstream ftmp("tmppts");
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

                if (system("voronoi -t < tmppts > tmpcells") == 0) {
                    int cx, cy, cz;
                    std::ifstream ftri("tmpcells");
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
                    std::cerr << "WARNING: triangularization failed" << std::endl;
                }
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
                    std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << npts << " points" << std::endl;
                    return;
                }
                else if (!isrect && (npts != nxy*nz)) {
                    std::cerr << "inconsistent data: expected " << nxy*nz << " points but found " << npts << " points" << std::endl;
                    return;
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
                std::cerr << "inconsistent data: expected " << nx*ny*nz << " points but found " << nread << " points" << std::endl;
                return;
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
	    nx = pow(2.0, ceil(log10((double)nx)/log10(2.0)));  // must be an even power of 2
	    ny = pow(2.0, ceil(log10((double)ny)/log10(2.0)));
	    nz = pow(2.0, ceil(log10((double)nz)/log10(2.0)));
#endif

            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }

            std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

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

                        // gradient in x-direction
                        double curval = (v < 0) ? 0.0 : v;
                        double oldval = ((ngen/4) % nx == 0) ? 0.0 : data[ngen-4];
                        oldval = (oldval < 0) ? 0.0 : oldval;
                        data[ngen+1] = (curval-oldval)/dmin;

                        // gradient in y-direction
                        oldval = (ngen-4*nx >= 0) ? data[ngen-4*nx] : 0.0;
                        oldval = (oldval < 0) ? 0.0 : oldval;
                        data[ngen+2] = (curval-oldval)/dmin;

                        // gradient in z-direction
                        oldval = (ngen-4*nx*ny >= 0) ? data[ngen-4*nx*ny] : 0.0;
                        oldval = (oldval < 0) ? 0.0 : oldval;
                        data[ngen+3] = (curval-oldval)/dmin;
			ngen += 4;
                    }
                }
            }

	    /*
	    //hack very first and last yz slice have 0 grad
	    for(int iz=0; iz<nz; iz++){
	      for(int iy=0; iy<ny; iy++){
                 int index1, index2;
		 int ix1 = 0;
		 int ix2 = 3;
		 index1 = ix1 + iy*nx + iz*nx*ny;
		 index2 = ix2 + iy*nx + iz*nx*ny;
		 data[4*index1 + 1] = data[4*index2 + 1];
		 data[4*index1 + 2] = data[4*index2 + 2];
		 data[4*index1 + 3] = data[4*index2 + 3];

		 ix1 = nx-1;
		 ix2 = nx-2;
		 index1 = ix1 + iy*nx + iz*nx*ny;
		 index2 = ix2 + iy*nx + iz*nx*ny;
		 data[4*index1 + 1] = data[4*index2 + 1];
		 data[4*index1 + 2] = data[4*index2 + 2];
		 data[4*index1 + 3] = data[4*index2 + 3];
	      }
	    }
	    */

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
                std::cerr << "inconsistent data: expected " << nxy*nz << " points but found " << nread << " points" << std::endl;
                return;
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
	    nx = pow(2.0, ceil(log10((double)nx)/log10(2.0)));  // must be an even power of 2
            ny = (int)ceil(dy/dmin);
	    ny = pow(2.0, ceil(log10((double)ny)/log10(2.0)));
            nz = (int)ceil(dz/dmin);
	    nz = pow(2.0, ceil(log10((double)nz)/log10(2.0)));
            float *data = new float[4*nx*ny*nz];

            double vmin = field.valueMin();
            double dv = field.valueMax() - field.valueMin();
            if (dv == 0.0) { dv = 1.0; }
            std::cout << "generating " << nx << "x" << ny << "x" << nz << " = " << nx*ny*nz << " points" << std::endl;

            // generate the uniformly sampled data that we need for a volume
            int ngen = 0;
            for (int iz=0; iz < nz; iz++) {
                double zval = z0 + iz*dmin;
                for (int iy=0; iy < ny; iy++) {
                    double yval = y0 + iy*dmin;
                    for (int ix=0; ix < nx; ix++) {
                        double xval = x0 + ix*dmin;
                        double v = field.value(xval,yval,zval);
                        data[ngen++] = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen++] = 0.0;
                        data[ngen++] = 0.0;
                        data[ngen++] = 0.0;
                    }
                }
            }
            load_volume(index, nx, ny, nz, 4, data);
            delete [] data;
        }
    } else {
        std::cerr << "WARNING: data not found in file " << fname << std::endl;
    }
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

  volume[index] = new Volume(0.f, 0.f, 0.f, width, height, depth, NVIS_FLOAT, NVIS_LINEAR_INTERP, n_component, data);
  assert(volume[index]!=0);
}


//load a colormap 1D texture
void load_transfer_function(int index, int size, float* data){

  if(tf[index]!=0){
    delete tf[index];
    tf[index]=0;
  }

  tf[index] = new TransferFunction(size, data);
}


//load value from the gui, only one transfer function
extern void update_tf_texture(){
  glutSetWindow(render_window);

  //fprintf(stderr, "tf update\n");
  if(tf[0]==0) return;

  float data[256*4];
  for(int i=0; i<256; i++){
    data[4*i+0] = color_table[i][0];
    data[4*i+1] = color_table[i][1];
    data[4*i+2] = color_table[i][2];
    data[4*i+3] = color_table[i][3];
    //fprintf(stderr, "(%f,%f,%f,%f) ", data[4*i+0], data[4*i+1], data[4*i+2], data[4*i+3]);
  }

  tf[0]->update(data);

#ifdef EVENTLOG
  float param[3] = {0,0,0};
  Event* tmp = new Event(EVENT_ROTATE, param, get_time_interval());
  tmp->write(event_log);
  delete tmp;
#endif
}



//initialize frame buffer objects for offscreen rendering
void init_fbo(){

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


void init_tf(){
  float data[256*4];
  memset(data, 0, 4*256*sizeof(float));

  tf[0] = new TransferFunction(256, data);
}


//init line integral convolution
void init_lic(){
  lic = new Lic(NMESH, win_width, win_height, 0.3, g_context, volume[0]->id, 
		  volume[0]->aspect_ratio_width,
		  volume[0]->aspect_ratio_height,
		  volume[0]->aspect_ratio_depth);
}

/*----------------------------------------------------*/
void initGL(void) 
{ 
   system_info();
   init_glew();

   //create the camera with default setting
   cam = new Camera(win_width, win_height, 
		   live_obj_x, live_obj_y, live_obj_z,
		   0., 0., 100.,
		   live_rot_x, live_rot_y, live_rot_z);

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

   //init volume and colormap arrays
   for(int i=0; i<MAX_N_VOLUMES; i++){
     volume[i] = 0;
     tf[i] = 0;
   }

   //check if performance query is supported
   if(check_query_support()){
     //create queries to count number of rendered pixels
     perf = new PerfQuery(); 
   }

   //load_volume_file(0, "./data/A-apbs-2-out-potential-PE0.dx");
   //load_volume_file(0, "./data/nw-AB-Vg=0.000-Vd=1.000-potential.dx");
   //load_volume_file(0, "./data/test2.dx");

   load_vector_file(0, "./data/J-wire-vec.dx");
   load_volume_file(1, "./data/mu-wire-3d.dx");

   init_tf();   //initialize transfer function
   init_fbo();	//frame buffer objects
   init_cg();	//init cg shaders
   init_lic();  //init line integral convolution

   //create volume renderer
   vol_render = new VolumeRenderer(cam, volume[1], tf[0], g_context);
   
   psys = new ParticleSystem(NMESH, NMESH, g_context, volume[0]->id,
		   1./volume[0]->aspect_ratio_width,
		   1./volume[0]->aspect_ratio_height,
		   1./volume[0]->aspect_ratio_depth);
   
   //psys = new ParticleSystem(NMESH, NMESH, g_context, volume[0]->id, 0.25, 1., 1.);

   init_particles();	//fill initial particles
}



void initTcl(){
  interp = Tcl_CreateInterp();
  Tcl_MakeSafe(interp);

  Tcl_CreateCommand(interp, "camera", CameraCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //Tcl_CreateCommand(interp, "size", SizeCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //Tcl_CreateCommand(interp, "clear", ClearCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //Tcl_CreateCommand(interp, "cut", CutCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //Tcl_CreateCommand(interp, "outline", OutlineCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(interp, "hello", HelloCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(interp, "move", MoveCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateCommand(interp, "refresh", RefreshCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  //Tcl_CreateCommand(interp, "load", LoadCmd, (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
}


void read_screen(){
  //glBindTexture(GL_TEXTURE_2D, 0);
  //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);
  glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, screen_buffer);
  assert(glGetError()==0);
  
  for(int i=0; i<win_width*win_height; i++){
    if(screen_buffer[3*i]!=0 || screen_buffer[3*i+1]!=0 || screen_buffer[3*i+2]!=0)
      fprintf(stderr, "(%d %d %d) ", screen_buffer[3*i], screen_buffer[3*i+1],  screen_buffer[3*i+2]);
  }
  
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


void xinetd_listen(){
    //command:
    // 0. load data
    // 1. flip on/off screen
    // 2. rotation
    // 3. zoom
    // 4. more slices
    // 5. less sleces

    std::string data;
    char tmp[256];
    bzero(tmp, 256);
    int status = read(0, tmp, 256);
    if (status <= 0) {
      exit(0);
    }
    fprintf(stderr, "read %d\n", status);
    data = tmp;

    if(Tcl_Eval(interp, (char*)data.c_str()) != TCL_OK) {
      //error to log file...
      fprintf(stderr, "Tcl error: parser\n");
      return;
    }

    display();

#if DO_RLE
    do_rle();
    int sizes[2] = {  offsets_size*sizeof(offsets[0]), rle_size }; 
    fprintf(stderr, "Writing %d,%d\n", sizes[0], sizes[1]); fflush(stderr);
    write(0, &sizes, sizeof(sizes));
    write(0, offsets, offsets_size*sizeof(offsets[0]));
    write(0, rle, rle_size);	//unsigned byte
#else
    write(0, screen_buffer, win_width * win_height * 3);
#endif

    cerr << "server: serve() image sent" << endl;
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


void display_final_fbo(){
   glEnable(GL_TEXTURE_2D);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glBindTexture(GL_TEXTURE_2D, final_color_tex);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(-1, 1, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   //glColor3f(1.,1.,1.);		//MUST HAVE THIS LINE!!!
   glBegin(GL_QUADS);
   glTexCoord2f(0, 0); glVertex2f(-1, -1);
   glTexCoord2f(1, 0); glVertex2f(1, -1);
   glTexCoord2f(1, 1); glVertex2f(1, 1);
   glTexCoord2f(0, 1); glVertex2f(-1, 1);
   glEnd();
}

void final_fbo_capture(){
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
	glScalef(0.0005, 0.0005, 0.0005);
	glutStrokeCharacter(GLUT_STROKE_ROMAN, 'x');
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0.4, 0.);
	glScalef(0.0005, 0.0005, 0.0005);
	glutStrokeCharacter(GLUT_STROKE_ROMAN, 'y');
	glPopMatrix();	

	glPushMatrix();
	glTranslatef(0., 0., 0.4);
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

   lic->convolve(); //flow line integral convolution

   psys->advect(); //advect particles

   final_fbo_capture();
   //display_texture(slice_vector_tex, NMESH, NMESH);

#if 1
   //start final rendering
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);

   //camera setting activated
   cam->activate();

   //now render things in the scene
   //
   draw_3d_axis();
   
   lic->render(); 	//display the line integral convolution result
   
   //soft_display_verts();
   perf->enable();
     psys->render();
   perf->disable();
   //fprintf(stderr, "particle pixels: %d\n", perf->get_pixel_count());
   perf->reset();


   perf->enable();
     //render volume :0 
     //volume[0]->location =Vector3(0.,0.,0.);
     //render_volume(0, 256);

     //render volume :1
     volume[1]->location =Vector3(0., 0., 0.);
     //render_volume(1, 256);
     vol_render->render(0);

     //fprintf(stderr, "%lf\n", get_time_interval());
   perf->disable();
   //fprintf(stderr, "volume pixels: %d\n", perf->get_pixel_count());
 
#ifdef XINETD
   float cost  = perf->get_pixel_count();
   write(3, &cost, sizeof(cost));
#endif
   perf->reset();

#endif

   display_final_fbo(); //display the final rendering on screen

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
	case 'o':
		live_specular+=1;
		fprintf(stderr, "specular: %f\n", live_specular);
		vol_render->set_specular(live_specular);
		break;
	case 'p':
		live_specular-=1;
		fprintf(stderr, "specular: %f\n", live_specular);
		vol_render->set_specular(live_specular);
		break;
	case '[':
		live_diffuse+=0.5;
		vol_render->set_diffuse(live_diffuse);
		break;
	case ']':
		live_diffuse-=0.5;
		vol_render->set_diffuse(live_diffuse);
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

   MainTransferFunctionWindow * tf_window;
   tf_window = new MainTransferFunctionWindow();
   tf_window->mainInit();
   
   glutInitWindowSize(NPIX, NPIX);
   glutInitWindowPosition(10, 10);
   render_window = glutCreateWindow(argv[0]);

   glutDisplayFunc(display);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);

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
