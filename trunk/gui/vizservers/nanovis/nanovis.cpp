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

#include "nanovis.h"
#include "RpFieldRect3D.h"
#include "RpFieldPrism3D.h"

ParticleSystem* psys;
float psys_x=0.4, psys_y=0, psys_z=0;

char* screen_buffer = new char[4*NPIX*NPIX+1];		//buffer to store data read from the screen
NVISid fbo, color_tex, pattern_tex, mag_tex, final_fbo, final_color_tex, final_depth_rb; //fbo related identifiers
NVISid vel_fbo, slice_vector_tex;	//for projecting 3d vector to 2d vector on a plane

bool advect=false;
float vert[NMESH*NMESH*3];		//particle positions in main memory
float slice_vector[NMESH*NMESH*4];	//per slice vectors in main memory

int n_volumes = 0;
Volume* volume[MAX_N_VOLUMES];		//point to volumes, currently handle up to 10 volumes
ColorMap* colormap[MAX_N_VOLUMES];	//transfer functions, currently handle up to 10 colormaps

PerfQuery* perf;			//perfromance counter

//Nvidia CG shaders and their parameters
CGcontext g_context;
CGprogram m_pos_fprog;
CGparameter m_vel_tex_param, m_pos_tex_param;
CGparameter m_pos_timestep_param, m_pos_spherePos_param;

CGprogram m_vel_fprog;
CGparameter m_vel_timestep_param, m_vel_damping_param, m_vel_gravity_param;
CGparameter m_vel_spherePos_param, m_vel_sphereVel_param, m_vel_sphereRadius_param;

CGprogram m_render_vel_fprog;
CGparameter m_vel_tex_param_render_vel, m_plane_normal_param_render_vel;

CGprogram m_passthru_fprog;
CGparameter m_passthru_scale_param, m_passthru_bias_param;

CGprogram m_copy_texcoord_fprog;

CGprogram m_one_volume_fprog;
CGparameter m_vol_one_volume_param;
CGparameter m_mvi_one_volume_param;
CGparameter m_render_param_one_volume_param;

CGprogram m_vert_std_vprog;
CGparameter m_mvp_vert_std_param;
CGparameter m_mvi_vert_std_param;


using namespace std;


// Tcl interpreter for incoming messages
static Tcl_Interp *interp;

static int CameraCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int ClearCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int SizeCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int OutlineCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int CutCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int HelloCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));
static int LoadCmd _ANSI_ARGS_((ClientData cdata, Tcl_Interp *interp, int argc, CONST84 char *argv[]));


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


//load a vector field from file
void init_vector_field(){

  FILE* fp = fopen("./data/tornado_64x64x64.raw", "rb");
  assert(fp!=0);

  float* data = new float[64*64*64*3];
  for(int j=0; j<64*64*64*3; j++){
       fread(data + j, 4, 1, fp);
       //fprintf(stderr, "%f ", data[3*j+i]);
  }
  fclose(fp);

  //normalize data
  float _max = FLT_MIN;
  float _min = FLT_MAX;

  for(int j=0; j<64*64*64; j++){
    float mag = sqrt(data[j*3]*data[j*3] + data[j*3+1]*data[j*3+1] + data[j*3+2]*data[j*3+2]);
    if(mag > _max)
      _max = mag;
    if(mag < _min) 
      _min = mag;
  }

  fprintf(stderr, "max=%f, min=%f\n", _max, _min);

  float scale = _max - _min;
  
  for(int j=0; j<64*64*64*3; j++){
    data[j]=data[j]/scale;

#ifndef NV40
    //cut negative values
    if(data[j]<0)
      data[j]=0;
#endif
  }

  /*
  for(int j=0; j<64*64*64; j++)
    fprintf(stderr, "[%f, %f, %f]\t", data[j*3], data[j*3+1], data[j*3+2]);
  */

  //load the vector field as a volume
  load_volume(0, 64, 64, 64, 3, data);
  n_volumes++;
  delete[] data;

  fprintf(stderr, "init_vector_field\n");
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
            while (!fin.eof() && nread < npts) {
                if (!(fin >> dval).fail()) {
                    field.define(nread++, dval);
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
                        data[ngen++] = (isnan(v)) ? -1.0 : (v - vmin)/dv;
                        data[ngen++] = 0.0;
                        data[ngen++] = 0.0;
                        data[ngen++] = 0.0;
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
void load_colormap(int index, int size, float* data){

  if(colormap[index]!=0){
    delete colormap[index];
    colormap[index]=0;
  }

  colormap[index] = new ColorMap(size, data);
}


//initialize frame buffer objects for offscreen rendering
void init_fbo(){

  //initialize fbo for projecting a 3d vector to 2d plane
  glGenFramebuffersEXT(1, &vel_fbo);
  glGenTextures(1, &slice_vector_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);

  //initialize texture storing per slice vectors
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, NMESH, NMESH, 0, GL_RGBA, GL_FLOAT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, slice_vector_tex, 0);

  //initialize final fbo for final display
  glGenFramebuffersEXT(1, &final_fbo);
  glGenTextures(1, &final_color_tex);
  glGenRenderbuffersEXT(1, &final_depth_rb);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, final_fbo);

  //initialize final color texture
  glBindTexture(GL_TEXTURE_2D, final_color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
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

  //initialize fbo for lic
  glGenFramebuffersEXT(1, &fbo);
  glGenTextures(1, &color_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

  //initialize color texture for lic
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, win_width, win_height, 0,
               GL_RGB, GL_INT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, color_tex, 0);

  // Check framebuffer completeness at the end of initialization.
  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);
}


void makeMagnitudes(){
  GLubyte mag[NMESH][NMESH][4];

  //read vector filed
  for(int i=0; i<NMESH; i++){
    for(int j=0; j<NMESH; j++){

      float x=DM*i;
      float y=DM*j;

      float magnitude = sqrt(x*x+y*y)/1.414;
      
      //from green to red
      GLubyte r = floor(magnitude*255);
      GLubyte g = 0;
      GLubyte b = 255 - r;
      GLubyte a = 122;

      mag[i][j][0] = r;
      mag[i][j][1] = g;
      mag[i][j][2] = b;
      mag[i][j][3] = a;
    }
  }

  glGenTextures(1, &mag_tex);
  glBindTexture(GL_TEXTURE_2D, mag_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NMESH, NMESH, 0, GL_RGBA, GL_UNSIGNED_BYTE, mag);
}


void init_cg(){
    cgSetErrorCallback(cgErrorCallback);
    g_context = cgCreateContext();

    //load programs that can be used in all render modes
    
    //standard vertex program
    m_vert_std_vprog = loadProgram(g_context, CG_PROFILE_VP30, CG_SOURCE, "./shaders/vertex_std.cg");
    m_mvp_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewProjMatrix");
    m_mvi_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewInv");

    m_pos_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/update_pos.cg");
    m_pos_timestep_param  = cgGetNamedParameter(m_pos_fprog, "timestep");
    m_vel_tex_param = cgGetNamedParameter(m_pos_fprog, "vel_tex");
    m_pos_tex_param = cgGetNamedParameter(m_pos_fprog, "pos_tex");
    cgGLSetTextureParameter(m_vel_tex_param, volume[0]->id);

    m_render_vel_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/render_vel.cg");
    m_vel_tex_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, "vel_tex");
    m_plane_normal_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, "plane_normal");
    cgGLSetTextureParameter(m_vel_tex_param_render_vel, volume[0]->id);

    /*
    m_vel_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/update_vel.cg");
    m_vel_timestep_param  = cgGetNamedParameter(m_vel_fprog, "timestep");
    m_vel_damping_param   = cgGetNamedParameter(m_vel_fprog, "damping");
    m_vel_gravity_param   = cgGetNamedParameter(m_vel_fprog, "gravity");
    m_vel_spherePos_param = cgGetNamedParameter(m_vel_fprog, "spherePos");
    m_vel_sphereVel_param = cgGetNamedParameter(m_vel_fprog, "sphereVel");
    */

    m_passthru_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/passthru.cg");
    m_passthru_scale_param = cgGetNamedParameter(m_passthru_fprog, "scale");
    m_passthru_bias_param  = cgGetNamedParameter(m_passthru_fprog, "bias");

    m_copy_texcoord_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/copy_texcoord.cg");

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
      //render one volume
      m_one_volume_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/one_volume.cg");
      m_vol_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "volume");
      cgGLSetTextureParameter(m_vol_one_volume_param, volume[0]->id);
      m_mvi_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "modelViewInv");
      m_render_param_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "renderParameters");
      break;

   case 1:
      //render two volumes
      
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
      bool particle = rand() % 256 > 100; 
      if(particle)
      {
        data[4*index] = psys_x;
	data[4*index+1]= i/float(psys->psys_width);
	data[4*index+2]= j/float(psys->psys_height);
	data[4*index+3]= 30;	
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

  fprintf(stderr, "init particles\n");
}



void makePatterns();
void get_slice_vectors();


/*----------------------------------------------------*/
void initGL(void) 
{ 
   system_info();
   init_glew();

   glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); 
   glTranslatef(-1.0, -1.0, 0.0); 
   glScalef(2.0, 2.0, 1.0);

   glGenTextures(1, &pattern_tex);
   glBindTexture(GL_TEXTURE_2D, pattern_tex);
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_WRAP_S, GL_REPEAT); 
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_WRAP_T, GL_REPEAT); 
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, 
                   GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   makePatterns();
   makeMagnitudes();

   glEnable(GL_TEXTURE_2D);
   glShadeModel(GL_FLAT);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glClear(GL_COLOR_BUFFER_BIT);

   //init volume and colormap arrays
   for(int i=0; i<MAX_N_VOLUMES; i++){
     volume[i] = 0;
     colormap[i] = 0;
   }

   //check if performance query is supported
   if(check_query_support()){
     //create queries to count number of rendered pixels
     perf = new PerfQuery(); 
   }

   init_vector_field();	//3d vector field
   //load_volume_file(0, "./data/A-apbs-2-out-potential-PE0.dx");
   //load_volume_file(0, "./data/nw-AB-Vg=0.000-Vd=1.000-potential.dx");
   //load_volume_file(0, "./data/test2.dx");

   init_fbo();	//frame buffer objects
   init_cg();	//init cg shaders

   //load the default one volume shader
   switch_shader(cur_shader);

   //psys = new ParticleSystem(NMESH, NMESH, g_context, volume[0]->id);
   //init_particles();	//fill initial particles

   //get_slice_vectors();
}


void initTcl(){
  interp = Tcl_CreateInterp();
  Tcl_MakeSafe(interp);
}


/*----------------------------------------------------*/
void makePatterns(void) 
{ 
   int lut[256];
   int phase[NPN][NPN];
   GLubyte pat[NPN][NPN][4];
   int i, j, k, t;
    
   for (i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
   for (i = 0; i < NPN; i++)
   for (j = 0; j < NPN; j++) phase[i][j] = rand() % 256; 

   for (k = 0; k < Npat; k++) {
      t = k*256/Npat;
      for (i = 0; i < NPN; i++) 
      for (j = 0; j < NPN; j++) {
          pat[i][j][0] =
          pat[i][j][1] =
          pat[i][j][2] = lut[(t + phase[i][j]) % 255];
          pat[i][j][3] = alpha;
      }

      glNewList(k + 1, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, pattern_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, GL_RGBA, GL_UNSIGNED_BYTE, pat);
      glEndList();
   }
}


/*----------------------------------------------------*/
void getDP(float x, float y, float *px, float *py) 
{
   float dx, dy, vx, vy, r;

   
   /*
   dx = x - 0.5;         
   dy = y - 0.5; 
   r  = dx*dx + dy*dy; 
   if (r < 0.0001) r = 0.0001;
   vx = sa*dx/r + 0.02;  
   vy = sa*dy/r;
   r  = vx*vx + vy*vy;
   if (r > dmax*dmax) { 
      r  = sqrt(r); 
      vx *= dmax/r; 
      vy *= dmax/r; 
   }
   *px = x + vx;         
   *py = y + vy;
   */
   
   int xi = x*NMESH;
   int yi = y*NMESH;

   vx = slice_vector[4*(xi+yi*NMESH)];
   vy = slice_vector[4*(xi+yi*NMESH)+1];
   r  = vx*vx + vy*vy;
   if (r > dmax*dmax) { 
      r  = sqrt(r); 
      vx *= dmax/r; 
      vy *= dmax/r; 
   }
   *px = x + vx;         
   *py = y + vy;

}


void read_screen(){
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  glReadPixels(0, 0, win_width, win_height, GL_RGBA, /*GL_COLOR_ATTACHMENT0_EXT*/ GL_UNSIGNED_BYTE, screen_buffer);
  assert(glGetError()==0);
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
    fprintf(stderr, "read %d\n", read(fileno(stdin), tmp, 256));
    data = tmp;

    if(Tcl_Eval(interp, (char*)data.c_str()) != TCL_OK) {
      //error to log file...
      fprintf(stderr, "Tcl error: parser\n");
      return;
    }

    //read the image
    read_screen(); 
    writen(fileno(stdout), screen_buffer, 4*win_width*win_height);	//unsigned byte

    cerr << "server: serve() done" << endl;
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
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000;

  nanosleep(&ts, 0);
  //xinetd_listen();

  glutPostRedisplay();
}


void fbo_capture(){
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
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

   glColor3f(1.,1.,1.);		//MUST HAVE THIS LINE!!!
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


//line integral convolution
void lic(){
   int   i, j; 
   float x1, x2, y, px, py;

   glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); 
   glTranslatef(-1.0, -1.0, 0.0); 
   glScalef(2.0, 2.0, 1.0);

   //sa = 0.010*cos(iframe*2.0*M_PI/200.0);
   glBindTexture(GL_TEXTURE_2D, pattern_tex);
   glEnable(GL_TEXTURE_2D);
   sa = 0.01;
   for (i = 0; i < NMESH-1; i++) {
      x1 = DM*i; x2 = x1 + DM;
      glBegin(GL_QUAD_STRIP);
      for (j = 0; j < NMESH-1; j++) {
          y = DM*j;
          glTexCoord2f(x1, y); 
          getDP(x1, y, &px, &py);
          glVertex2f(px, py);

          glTexCoord2f(x2, y); 
          getDP(x2, y, &px, &py); 
          glVertex2f(px, py);
      }
      glEnd();
   }
   iframe = iframe + 1;

   glEnable(GL_BLEND); 
   glCallList(iframe % Npat + 1);
   glBegin(GL_QUAD_STRIP);
      glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
      glTexCoord2f(0.0,  tmax); glVertex2f(0.0, 1.0);
      glTexCoord2f(tmax, 0.0);  glVertex2f(1.0, 0.0);
      glTexCoord2f(tmax, tmax); glVertex2f(1.0, 1.0);
   glEnd();

   //inject dye
   glDisable(GL_TEXTURE_2D);
   glColor4f(1.,0.8,0.,1.);
   glBegin(GL_QUADS);
     glVertex2d(0.6, 0.6);
     glVertex2d(0.6, 0.62);
     glVertex2d(0.62, 0.62);
     glVertex2d(0.62, 0.6);
   glEnd();

   glDisable(GL_BLEND);
   glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, NPIX, NPIX, 0);
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


//get lic per slice vectors
void get_slice_vectors(){

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);
   glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);

   glClear(GL_COLOR_BUFFER_BIT);

   glViewport(0, 0, NMESH, NMESH);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, NMESH, 0, NMESH);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   cgGLBindProgram(m_render_vel_fprog);
   cgGLEnableTextureParameter(m_vel_tex_param_render_vel);
   cgGLSetParameter4f(m_plane_normal_param_render_vel, 1., 1., 0., 0);

   cgGLEnableProfile(CG_PROFILE_FP30);
   glBegin(GL_QUADS);
    glTexCoord3f(0., 0., slice_z); glVertex2f(0.,         0.);
    glTexCoord3f(1., 0., slice_z); glVertex2f((float)NMESH,     0.);
    glTexCoord3f(1., 1., slice_z); glVertex2f((float)NMESH, (float)NMESH);
    glTexCoord3f(0., 1., slice_z); glVertex2f((float)0.,    (float)NMESH);

    /*
    glTexCoord3f(0., 0., 0.5); glVertex2f(0.,         0.);
    glTexCoord3f(1., 0., 0.5); glVertex2f((float)NMESH,     0.);
    glTexCoord3f(1., 1., 0.5); glVertex2f((float)NMESH, (float)NMESH);
    glTexCoord3f(0., 1., 0.5); glVertex2f((float)0.,    (float)NMESH);
    */

   glEnd();
   cgGLDisableProfile(CG_PROFILE_FP30);
   
   cgGLDisableTextureParameter(m_vel_tex_param_render_vel);

   //read the vectors
   glReadPixels(0, 0, NMESH, NMESH, GL_RGBA, GL_FLOAT, slice_vector);
   /*
   for(int i=0; i<NMESH*NMESH; i++){
     fprintf(stderr, "%f,%f,%f,%f", slice_vector[4*i], slice_vector[4*i+1], slice_vector[4*i+2], slice_vector[4*i+3]);
   }
   */
   assert(glGetError()==0);
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

void get_near_far_z(Mat4x4 mv, double &zNear, double &zFar)
{

  double x0 = 0;
  double y0 = 0;
  double z0 = 0;
  double x1 = 1;
  double y1 = 1;
  double z1 = 1;

  double zMin, zMax;
  zMin =  10000;
  zMax = -10000;

  double vertex[8][4];

  vertex[0][0]=x0; vertex[0][1]=y0; vertex[0][2]=z0; vertex[0][3]=1.0;
  vertex[1][0]=x1; vertex[1][1]=y0; vertex[1][2]=z0; vertex[1][3]=1.0;
  vertex[2][0]=x0; vertex[2][1]=y1; vertex[2][2]=z0; vertex[2][3]=1.0;
  vertex[3][0]=x0; vertex[3][1]=y0; vertex[3][2]=z1; vertex[3][3]=1.0;
  vertex[4][0]=x1; vertex[4][1]=y1; vertex[4][2]=z0; vertex[4][3]=1.0;
  vertex[5][0]=x1; vertex[5][1]=y0; vertex[5][2]=z1; vertex[5][3]=1.0;
  vertex[6][0]=x0; vertex[6][1]=y1; vertex[6][2]=z1; vertex[6][3]=1.0;
  vertex[7][0]=x1; vertex[7][1]=y1; vertex[7][2]=z1; vertex[7][3]=1.0;

  for(int i=0;i<8;i++)
  {
    Vector4 tmp = mv.transform(Vector4(vertex[i][0], vertex[i][1], vertex[i][2], vertex[i][3]));
    tmp.perspective_devide();
    vertex[i][2] = tmp.z;
    if (vertex[i][2]<zMin) zMin = vertex[i][2];
    if (vertex[i][2]>zMax) zMax = vertex[i][2];
  }

  zNear = zMax;
  zFar = zMin;
}


void activate_one_volume_shader(int n_actual_slices){

  cgGLSetStateMatrixParameter(m_mvp_vert_std_param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
  cgGLSetStateMatrixParameter(m_mvi_vert_std_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLBindProgram(m_vert_std_vprog);
  cgGLEnableProfile(CG_PROFILE_VP30);

  cgGLSetStateMatrixParameter(m_mvi_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLEnableTextureParameter(m_vol_one_volume_param);
  cgGLSetParameter4f(m_render_param_one_volume_param, n_actual_slices, 0., 0., 0.);
  cgGLBindProgram(m_one_volume_fprog);
  cgGLEnableProfile(CG_PROFILE_FP30);
}


void deactivate_one_volume_shader(){
  cgGLDisableProfile(CG_PROFILE_VP30);
  cgGLDisableProfile(CG_PROFILE_FP30);

  cgGLDisableTextureParameter(m_vel_tex_param);
  cgGLDisableTextureParameter(m_pos_tex_param);
}


//render volumes
void render_volume(int volume_index, int n_slices){

  //volume start location
  Vector4 shift_4d(volume[volume_index]->location.x, volume[volume_index]->location.y, volume[volume_index]->location.z, 0);

  double x0 = 0;
  double y0 = 0;
  double z0 = 0;

  Mat4x4 model_view;
  Mat4x4 model_view_inverse;

  double zNear, zFar;

  //initialize volume plane with world coordinates
  Plane volume_planes[6];

  volume_planes[0].set_coeffs(1, 0, 0, -x0);
  volume_planes[1].set_coeffs(-1, 0, 0, x0+1);
  volume_planes[2].set_coeffs(0, 1, 0, -y0);
  volume_planes[3].set_coeffs(0, -1, 0, y0+1);
  volume_planes[4].set_coeffs(0, 0, 1, -z0);
  volume_planes[5].set_coeffs(0, 0, -1, z0+1);

  glPushMatrix();

  glScaled(volume[volume_index]->aspect_ratio_width, 
	  volume[volume_index]->aspect_ratio_height, 
	  volume[volume_index]->aspect_ratio_depth);

  glEnable(GL_DEPTH_TEST);

  //draw volume bounding box
  draw_bounding_box(x0+shift_4d.x, y0+shift_4d.y, z0+shift_4d.z, x0+shift_4d.x+1, y0+shift_4d.y+1, z0+shift_4d.z+1, 1, 1, 0, 2.0);

  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);

  model_view = Mat4x4(mv);
  model_view_inverse = model_view.inverse();

  glPopMatrix();

  //transform volume_planes to eye coordinates.
  for(int i=0; i<6; i++)
    volume_planes[i].transform(model_view);

  get_near_far_z(mv, zNear, zFar);
  //fprintf(stderr, "zNear:%f, zFar:%f\n", zNear, zFar);
  //fflush(stderr);

  //compute actual rendering slices
  float z_step = fabs(zNear-zFar)/n_slices;		
  int n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
  //fprintf(stderr, "slices: %d\n", n_actual_slices);
  //fflush(stderr);

  static ConvexPolygon staticPoly;	
  float slice_z;

  Vector4 vert1 = (Vector4(-10, -10, -0.5, 1));
  Vector4 vert2 = (Vector4(-10, +10, -0.5, 1));
  Vector4 vert3 = (Vector4(+10, +10, -0.5, 1));
  Vector4 vert4 = (Vector4(+10, -10, -0.5, 1));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  
  for (int i=0; i<n_actual_slices; i++){
    slice_z = zFar + i * z_step;	//back to front
	
    ConvexPolygon *poly;
    poly = &staticPoly;
    poly->vertices.clear();

    //Setting Z-coordinate 
    vert1.z = slice_z;
    vert2.z = slice_z;
    vert3.z = slice_z;
    vert4.z = slice_z;
		
    poly->append_vertex(vert1);
    poly->append_vertex(vert2);
    poly->append_vertex(vert3);
    poly->append_vertex(vert4);
	
    for(int k=0; k<6; k++){
      poly->clip(volume_planes[k]);
    }

    poly->copy_vertices_to_texcoords();
    //move the volume to the proper location

    poly->transform(model_view_inverse);
    poly->translate(shift_4d);
    poly->transform(model_view);

    glPushMatrix();
    glScaled(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);
    
    //glPushMatrix();
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glScaled(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);

    /*
    //draw slice lines only
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0);
    glColor3f(1,1,1);
    glBegin(GL_LINE_LOOP);
      poly->Emit(false);
    glEnd();
    */
    
    activate_one_volume_shader(n_actual_slices);
    glPopMatrix();

    glBegin(GL_POLYGON);
      poly->Emit(true); 
    glEnd();

    deactivate_one_volume_shader();
		
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  //glPopMatrix();
}

#if 0
void render_volume(int volume_index, int n_slices){ 

	double x0 = 0.;
	double y0 = 0.;
	double z0 = 0.;

	Mat4x4 model_view;
	Mat4x4 model_view_inverse;
	
	double zNear, zFar;

	//initialize volume plane with world coordinates
  	Plane volume_planes[6];
	
	volume_planes[0].set_coeffs(1, 0, 0, x0);
	volume_planes[1].set_coeffs(-1, 0, 0, x0+1);
	volume_planes[2].set_coeffs(0, 1, 0, y0);
	volume_planes[3].set_coeffs(0, -1, 0, y0+1);
	volume_planes[4].set_coeffs(0, 0, 1, z0);
	volume_planes[5].set_coeffs(0, 0, -1, z0+1);
	
	
	glPushMatrix();
	
	//glScaled(volume[0]->aspect_ratio_width, volume[0]->aspect_ratio_height, volume[0]->aspect_ratio_depth);
	glScaled(1,1,1);
	
	glEnable(GL_DEPTH_TEST);

	//draw volume bounding box
	//draw_bounding_box(Vector3(x0, y0, z0), Vector3(x1, y1, z1), Vector3(1,1,0), 2.0);
	draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1, 
			  1,1,1, 2.0);

	GLfloat mv[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);

	model_view = Mat4x4(mv);
	model_view_inverse = model_view.inverse();

	glPopMatrix();

	//transform volume_planes to eye coordinates.
	for(int i=0; i<6; i++)
		volume_planes[i].transform(model_view);
	
	get_near_far_z(mv, zNear, zFar);
	printf("zNear:%f, zFar:%f\n", zNear, zFar);

	//compute actual rendering slices
	float z_step = fabs(zNear-zFar)/n_slices;		
	int n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
	printf("slice:%d\n", n_actual_slices);

	static ConvexPolygon staticPoly;	
	float slice_z;

	Vector4 vert1 = (Vector4(-10, -10, -0.5, 1));
	Vector4 vert2 = (Vector4(-10, +10, -0.5, 1));
	Vector4 vert3 = (Vector4(+10, +10, -0.5, 1));
	Vector4 vert4 = (Vector4(+10, -10, -0.5, 1));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	for (int i=0; i<n_actual_slices; i++){
		slice_z = zFar + i * z_step;	//back to front
		
		ConvexPolygon *poly;
    		poly = &staticPoly;
		poly->vertices.clear();

		//Setting Z-coordinate 
		vert1.z = slice_z;
		vert2.z = slice_z;
		vert3.z = slice_z;
		vert4.z = slice_z;
		
		poly->append_vertex(vert1);
		poly->append_vertex(vert2);
		poly->append_vertex(vert3);
		poly->append_vertex(vert4);
	
		for(int k=0; k<6; k++){
			poly->clip(volume_planes[k]);
		}

		poly->copy_vertices_to_texcoords();
		//poly->transform(model_view_inverse);
		//poly->translate(shift_4d);
		//poly->transform(model_view);

		glPushMatrix();
		glScaled(volume[0]->aspect_ratio_width, volume[0]->aspect_ratio_height, volume[0]->aspect_ratio_depth);

		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_3D);
		glDisable(GL_TEXTURE_2D);
			glColor4f(1.,1.,1.,1.);
			glLineWidth(1.0);
			glBegin(GL_LINE_LOOP);
				poly->Emit(false);
			glEnd();
		glPopMatrix();


		glPopMatrix();

	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	//glPopMatrix();
}
#endif


/*----------------------------------------------------*/
void display()
{

   assert(glGetError()==0);

   //enable fbo
   fbo_capture();

   //convolve
   //lic();

   /*
   //blend magnitude texture
   glBindTexture(GL_TEXTURE_2D, mag_tex);
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBegin(GL_QUADS);
      glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
      glTexCoord2f(0.0,  1.0); glVertex2f(0.0, 1.);
      glTexCoord2f(1.0, 1.0);  glVertex2f(1., 1.);
      glTexCoord2f(1.0, 0.0); glVertex2f(1., 0.0);
   glEnd();
   */
   
   //advect particles
   //psys->advect();

   final_fbo_capture();
   //display_texture(slice_vector_tex, NMESH, NMESH);

#if 1
   //start final rendering
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glBindTexture(GL_TEXTURE_2D, color_tex);

   glViewport(0, 0, NPIX, NPIX);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(60, (GLdouble)1, 0.1, 50.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   //glTranslatef(live_obj_x, live_obj_y, live_obj_z);
   glTranslatef(0, 0, live_obj_z);

   glRotated(live_rot_x, 1., 0., 0.);
   glRotated(live_rot_y, 0., 1., 0.);
   glRotated(live_rot_z, 0., 0., 1.);

   /*
   //draw line integral convolution quad
   glBegin(GL_QUADS);
   glTexCoord2f(0, 0); glVertex3f(0, 0, slice_z);
   glTexCoord2f(1, 0); glVertex3f(1, 0, slice_z);
   glTexCoord2f(1, 1); glVertex3f(1, 1, slice_z);
   glTexCoord2f(0, 1); glVertex3f(0, 1, slice_z);
   glEnd();
   */

   //soft_display_verts();
   //psys->display_vertices();

   //render multiple volumes
   //volume[0]->location =Vector3(0.,0.,0.);
   //render_volume(0, 256);

   //render another but shifted using the same texture
   volume[0]->location =Vector3(-0.5,-0.5,-0.5);

   perf->enable();
   render_volume(0, 256);
   perf->disable();
   fprintf(stderr, "pixels: %d\n", perf->get_pixel_count());
   perf->reset();

   glDisable(GL_DEPTH_TEST);
#endif

   display_final_fbo();

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
}


void update_trans(int delta_x, int delta_y, int delta_z){
        live_obj_x += delta_x*0.03;
        live_obj_y += delta_y*0.03;
        live_obj_z += delta_z*0.03;
}


void keyboard(unsigned char key, int x, int y){
   
   switch (key){
	case 'q':
		exit(0);
		break;
	case '+':
		slice_z+=0.1;
		get_slice_vectors();
		break;
	case '-':
		slice_z-=0.1;
		get_slice_vectors();
		break;
	case '1':
		advect = true;
		break;
	case '2':
		psys_x+=0.1;
		break;
	case '3':
		psys_x-=0.1;
		break;
	default:
		break;
    }	
    
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
    //if(abs(delta_x)<5 && abs(delta_y)<5)
      //return;

    if(left_down){
      left_last_x = x;
      left_last_y = y;

      update_rot(delta_y, delta_x);
    }
    else if (right_down){
      //fprintf(stderr, "right mouse motion (%d,%d)\n", x, y);

      right_last_x = x;
      right_last_y = y;

      update_trans(0, 0, delta_x);
    }

    glutPostRedisplay();
}



/*----------------------------------------------------*/
int main(int argc, char** argv) 
{ 
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); 
   glutInitWindowSize(NPIX, NPIX);
   glutCreateWindow(argv[0]);
   glutDisplayFunc(display);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutKeyboardFunc(keyboard);
   glutIdleFunc(idle);
   initGL();
   initTcl();

   //FILE* log_file = fopen("log.txt", "w");
   //close(2);
   //dup2(fileno(log_file), 2);

   glutMainLoop();
   return 0;
}
