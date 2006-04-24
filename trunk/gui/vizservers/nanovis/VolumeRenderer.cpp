/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.cpp : VolumeRenderer class for volume visualization
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

#include "VolumeRenderer.h"


VolumeRenderer::VolumeRenderer(){}

VolumeRenderer::VolumeRenderer(Camera* _cam, Volume* _vol, TransferFunction* _tf,
				CGcontext _context):
  cam(_cam),
  n_volumes(0),
  g_context(_context)
{
  //init the vectors for volumes and transfer functions
  volume.clear();
  tf.clear(); 
  slice.clear();
  add_volume(_vol, _tf, 256);

  //initialize the volume shaders
  //
  //standard vertex program
  m_vert_std_vprog = loadProgram(g_context, CG_PROFILE_VP30, CG_SOURCE, "./shaders/vertex_std.cg");
  m_mvp_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewProjMatrix");
  m_mvi_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewInv");

  //volume rendering shader
  m_one_volume_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/one_volume.cg");
  m_vol_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "volume");
  cgGLSetTextureParameter(m_vol_one_volume_param, volume[0]->id);
  m_tf_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "tf");
  cgGLSetTextureParameter(m_tf_one_volume_param, tf[0]->id);
  m_mvi_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "modelViewInv");
  m_mv_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "modelView");
  m_render_param_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "renderParameters");

  live_diffuse = 1.;
  live_specular = 3.;
}

VolumeRenderer::~VolumeRenderer(){}

void VolumeRenderer::add_volume(Volume* _vol, TransferFunction* _tf, int _slice){
  volume.push_back(_vol);
  tf.push_back(_tf);
  slice.push_back(_slice);
  render_bit.push_back(true);

  n_volumes++;
}

void VolumeRenderer::render(int volume_index){
  int n_slices = slice[volume_index];

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

  glScalef(volume[volume_index]->aspect_ratio_width, 
	  volume[volume_index]->aspect_ratio_height, 
	  volume[volume_index]->aspect_ratio_depth);

  glEnable(GL_DEPTH_TEST);

  //draw volume bounding box
  draw_bounding_box(x0+shift_4d.x, y0+shift_4d.y, z0+shift_4d.z, x0+shift_4d.x+1, y0+shift_4d.y+1, z0+shift_4d.z+1, 0.8, 0.1, 0.1, 1.5);

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
    glScalef(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);
    
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
    
    activate_one_volume_shader(volume_index, n_actual_slices);
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

void VolumeRenderer::draw_bounding_box(float x0, float y0, float z0,
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



void VolumeRenderer::activate_one_volume_shader(int volume_index, int n_actual_slices){

  cgGLSetStateMatrixParameter(m_mvp_vert_std_param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
  cgGLSetStateMatrixParameter(m_mvi_vert_std_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLBindProgram(m_vert_std_vprog);
  cgGLEnableProfile(CG_PROFILE_VP30);

  cgGLSetStateMatrixParameter(m_mvi_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLSetStateMatrixParameter(m_mv_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
  cgGLSetTextureParameter(m_vol_one_volume_param, volume[volume_index]->id);
  cgGLEnableTextureParameter(m_vol_one_volume_param);
  cgGLEnableTextureParameter(m_tf_one_volume_param);

  cgGLSetParameter4f(m_render_param_one_volume_param, n_actual_slices, 0., live_diffuse, live_specular);
  cgGLBindProgram(m_one_volume_fprog);
  cgGLEnableProfile(CG_PROFILE_FP30);
}


void VolumeRenderer::deactivate_one_volume_shader(){
  cgGLDisableProfile(CG_PROFILE_VP30);
  cgGLDisableProfile(CG_PROFILE_FP30);

  cgGLDisableTextureParameter(m_vol_one_volume_param);
  cgGLDisableTextureParameter(m_tf_one_volume_param);
}

void VolumeRenderer::get_near_far_z(Mat4x4 mv, double &zNear, double &zFar)
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

void VolumeRenderer::set_specular(float val){ live_specular = val; }
void VolumeRenderer::set_diffuse(float val){ live_diffuse = val; }


