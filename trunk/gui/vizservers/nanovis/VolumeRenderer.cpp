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


VolumeRenderer::VolumeRenderer(CGcontext _context):
  n_volumes(0),
  g_context(_context),
  slice_mode(false),
  volume_mode(true)
{
  volume.clear();
  tf.clear(); 

  init_shaders();
}


VolumeRenderer::~VolumeRenderer(){}

//initialize the volume shaders
void VolumeRenderer::init_shaders(){
  
  //standard vertex program
  m_vert_std_vprog = loadProgram(g_context, CG_PROFILE_VP30, CG_SOURCE, "./shaders/vertex_std.cg");
  m_mvp_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewProjMatrix");
  m_mvi_vert_std_param = cgGetNamedParameter(m_vert_std_vprog, "modelViewInv");

  //volume rendering shader
  m_one_volume_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/one_volume.cg");
  m_vol_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "volume");
  //cgGLSetTextureParameter(m_vol_one_volume_param, _vol->id);
  m_tf_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "tf");
  //cgGLSetTextureParameter(m_tf_one_volume_param, _tf->id);
  m_mvi_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "modelViewInv");
  m_mv_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "modelView");
  m_render_param_one_volume_param = cgGetNamedParameter(m_one_volume_fprog, "renderParameters");
}


int VolumeRenderer::add_volume(Volume* _vol, TransferFunction* _tf){

  int ret = n_volumes;

  volume.push_back(_vol);
  tf.push_back(_tf);

  n_volumes++;

  return ret;
}

typedef struct SortElement{
  float z;
  int volume_id;
  int slice_id;
  SortElement(float _z, int _v, int _s):
	  z(_z), volume_id(_v), slice_id(_s){}
};

int slice_sort(const void* a, const void* b){
  if((*((SortElement*)a)).z > (*((SortElement*)b)).z)
      return 1;
   else
      return -1;
}


void VolumeRenderer::render_all(){
  int total_enabled_volumes = 0;
  int total_rendered_slices = 0;

  //compute total enabled volumes
  for(int i=0; i<n_volumes; i++){
    if(volume[i]->is_enabled())
      total_enabled_volumes ++;
  }
  //fprintf(stderr, "total volumes rendered: %d\n", total_enabled_volumes);

  ConvexPolygon*** polys = new ConvexPolygon**[total_enabled_volumes];	//two dimension pointer array
  									//storing the slices
  int* actual_slices = new int[total_enabled_volumes]; //number of actual slices for each volume

  int cur_vol = -1;
  for(int i=0; i<n_volumes; i++){
    if(!volume[i]->is_enabled())
      continue; //skip this volume

    cur_vol++;

    int volume_index = i;
    int n_slices = volume[volume_index]->get_n_slice();

    //volume start location
    Vector3* location = volume[volume_index]->get_location();
    Vector4 shift_4d(location->x, location->y, location->z, 0);

    double x0 = 0;
    double y0 = 0;
    double z0 = 0;

    Mat4x4 model_view_no_trans, model_view_trans;
    Mat4x4 model_view_no_trans_inverse, model_view_trans_inverse;

    double zNear, zFar;

    //initialize volume plane with world coordinates
    Plane volume_planes[6];
    volume_planes[0].set_coeffs(1, 0, 0, -x0);
    volume_planes[1].set_coeffs(-1, 0, 0, x0+1);
    volume_planes[2].set_coeffs(0, 1, 0, -y0);
    volume_planes[3].set_coeffs(0, -1, 0, y0+1);
    volume_planes[4].set_coeffs(0, 0, 1, -z0);
    volume_planes[5].set_coeffs(0, 0, -1, z0+1);
  
    //get modelview matrix with no translation
    glPushMatrix();
    glScalef(volume[volume_index]->aspect_ratio_width, 
	  volume[volume_index]->aspect_ratio_height, 
	  volume[volume_index]->aspect_ratio_depth);

    glEnable(GL_DEPTH_TEST);

    GLfloat mv_no_trans[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv_no_trans);

    model_view_no_trans = Mat4x4(mv_no_trans);
    model_view_no_trans_inverse = model_view_no_trans.inverse();

    glPopMatrix();

    //get modelview matrix with translation
    glPushMatrix();
    glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
    glScalef(volume[volume_index]->aspect_ratio_width, 
	  volume[volume_index]->aspect_ratio_height, 
	  volume[volume_index]->aspect_ratio_depth);
    GLfloat mv_trans[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv_trans);

    model_view_trans = Mat4x4(mv_trans);
    model_view_trans_inverse = model_view_trans.inverse();

    //draw volume bounding box with translation (the correct location in space)
    draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1, 0.8, 0.1, 0.1, 1.5);
    glPopMatrix();

    //transform volume_planes to eye coordinates.
    for(int i=0; i<6; i++)
      volume_planes[i].transform(model_view_no_trans);
    get_near_far_z(mv_no_trans, zNear, zFar);

    //compute actual rendering slices
    float z_step = fabs(zNear-zFar)/n_slices;		
    int n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
    actual_slices[cur_vol] = n_actual_slices;
    polys[cur_vol] = new ConvexPolygon*[n_actual_slices];

    float slice_z;

    Vector4 vert1 = (Vector4(-10, -10, -0.5, 1));
    Vector4 vert2 = (Vector4(-10, +10, -0.5, 1));
    Vector4 vert3 = (Vector4(+10, +10, -0.5, 1));
    Vector4 vert4 = (Vector4(+10, -10, -0.5, 1));

    
    //Render cutplanes first with depth test enabled.
    //They will mark the image with their depth values. Then we render other volume slices.
    //These volume slices will be occluded correctly by the cutplanes and vice versa.

    ConvexPolygon static_poly;
    for(int i=0; i<volume[volume_index]->get_cutplane_count(); i++){
      if(!volume[volume_index]->cutplane_is_enabled(i))
        continue;

      float offset = volume[volume_index]->get_cutplane(i)->offset;
      int axis = volume[volume_index]->get_cutplane(i)->orient;

      if(axis==1){
        vert1 = Vector4(-10, -10, offset, 1);
        vert2 = Vector4(-10, +10, offset, 1);
        vert3 = Vector4(+10, +10, offset, 1);
        vert4 = Vector4(+10, -10, offset, 1);
	//continue;
      }
      else if(axis==2){
        vert1 = Vector4(offset, -10, -10, 1);
        vert2 = Vector4(offset, +10, -10, 1);
        vert3 = Vector4(offset, +10, +10, 1);
        vert4 = Vector4(offset, -10, +10, 1);
	//continue;
      }
      else if(axis==3){
        vert1 = Vector4(-10, offset, -10, 1);
        vert2 = Vector4(+10, offset, -10, 1);
        vert3 = Vector4(+10, offset, +10, 1);
        vert4 = Vector4(-10, offset, +10, 1);
	//continue;
      }

      vert1 = model_view_no_trans.transform(vert1);
      vert2 = model_view_no_trans.transform(vert2);
      vert3 = model_view_no_trans.transform(vert3);
      vert4 = model_view_no_trans.transform(vert4);

      ConvexPolygon* p = &static_poly;
      p->vertices.clear();

      p->append_vertex(vert1);
      p->append_vertex(vert2);
      p->append_vertex(vert3);
      p->append_vertex(vert4);

      for(int k=0; k<6; k++){
        p->clip(volume_planes[k], true);
      }

      p->transform(model_view_no_trans_inverse);
      p->transform(model_view_trans);

      glPushMatrix();
      glScalef(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);

      activate_volume_shader(volume_index, true);
      glPopMatrix();

      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      glBegin(GL_POLYGON);
        p->Emit(true); 
      glEnd();
      glDisable(GL_DEPTH_TEST);

      deactivate_volume_shader();
    } //done cutplanes

   
    //Now do volume rendering

    vert1 = (Vector4(-10, -10, -0.5, 1));
    vert2 = (Vector4(-10, +10, -0.5, 1));
    vert3 = (Vector4(+10, +10, -0.5, 1));
    vert4 = (Vector4(+10, -10, -0.5, 1));

    int counter = 0;
    //transform slices and store them
    for (int i=0; i<n_actual_slices; i++){
      slice_z = zFar + i * z_step;	//back to front

      ConvexPolygon *poly = new ConvexPolygon();
      polys[cur_vol][counter] = poly;
      counter++;

      poly->vertices.clear();
      poly->set_id(volume_index);

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
        poly->clip(volume_planes[k], true);
      }

      poly->transform(model_view_no_trans_inverse);
      poly->transform(model_view_trans);

      if(poly->vertices.size()>=3)
	total_rendered_slices++; 
    }
  } //iterate all volumes
  //fprintf(stderr, "total slices: %d\n", total_rendered_slices); 

  //We sort all the polygons according to their eye-space depth, from farthest to the closest.
  //This step is critical for correct blending

  SortElement* slices = (SortElement*) malloc(sizeof(SortElement)*total_rendered_slices);

  int counter = 0;
  for(int i=0; i<total_enabled_volumes; i++){
    for(int j=0; j<actual_slices[i]; j++){
      if(polys[i][j]->vertices.size() >= 3){
        slices[counter] = SortElement(polys[i][j]->vertices[0].z, i, j);
        counter++;
      }
    }
  }

  //sort them
  qsort(slices, total_rendered_slices, sizeof(SortElement), slice_sort);

  /*
  //debug
  for(int i=0; i<total_rendered_slices; i++){
    fprintf(stderr, "%f ", slices[i].z);
  }
  fprintf(stderr, "\n\n");
  */

  //Now we are ready to render all the slices from back to front
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  for(int i=0; i<total_rendered_slices; i++){
    int volume_index = slices[i].volume_id;
    int slice_index = slices[i].slice_id;
    ConvexPolygon* cur = polys[volume_index][slice_index];

    glPushMatrix();
    glScalef(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);
    
    activate_volume_shader(volume_index, false);
    glPopMatrix();

    glBegin(GL_POLYGON);
      cur->Emit(true); 
    glEnd();

    deactivate_volume_shader();
  }


  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  //Deallocate all the memory used
  for(int i=0; i<total_enabled_volumes; i++){
    for(int j=0; j<actual_slices[i]; j++){
      delete polys[i][j];
    }
    delete[] polys[i];
  }
  delete[] polys;
  delete[] actual_slices;
  free(slices);
}


void VolumeRenderer::render(int volume_index){
  int n_slices = volume[volume_index]->get_n_slice();

  //volume start location
  Vector4 shift_4d(volume[volume_index]->location.x, volume[volume_index]->location.y, volume[volume_index]->location.z, 0);

  double x0 = 0;
  double y0 = 0;
  double z0 = 0;

  Mat4x4 model_view_no_trans, model_view_trans;
  Mat4x4 model_view_no_trans_inverse, model_view_trans_inverse;

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

  GLfloat mv_no_trans[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv_no_trans);

  model_view_no_trans = Mat4x4(mv_no_trans);
  model_view_no_trans_inverse = model_view_no_trans.inverse();

  glPopMatrix();

  //get modelview matrix with translation
  glPushMatrix();
  glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
  glScalef(volume[volume_index]->aspect_ratio_width, 
	  volume[volume_index]->aspect_ratio_height, 
	  volume[volume_index]->aspect_ratio_depth);
  GLfloat mv_trans[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv_trans);

  model_view_trans = Mat4x4(mv_trans);
  model_view_trans_inverse = model_view_trans.inverse();

  //draw volume bounding box
  draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1, 0.8, 0.1, 0.1, 1.5);
  glPopMatrix();

  //transform volume_planes to eye coordinates.
  for(int i=0; i<6; i++)
    volume_planes[i].transform(model_view_no_trans);

  get_near_far_z(mv_no_trans, zNear, zFar);
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

  glEnable(GL_BLEND);

  if(slice_mode){
    glEnable(GL_DEPTH_TEST);

  //render the cut planes
  for(int i=0; i<volume[volume_index]->get_cutplane_count(); i++){
    float offset = volume[volume_index]->get_cutplane(i)->offset;
    int axis = volume[volume_index]->get_cutplane(i)->orient;

    if(axis==1){
      vert1 = Vector4(-10, -10, offset, 1);
      vert2 = Vector4(-10, +10, offset, 1);
      vert3 = Vector4(+10, +10, offset, 1);
      vert4 = Vector4(+10, -10, offset, 1);
    }
    else if(axis==2){
      vert1 = Vector4(offset, -10, -10, 1);
      vert2 = Vector4(offset, +10, -10, 1);
      vert3 = Vector4(offset, +10, +10, 1);
      vert4 = Vector4(offset, -10, +10, 1);
    }
    else if(axis==3){
      vert1 = Vector4(-10, offset, -10, 1);
      vert2 = Vector4(+10, offset, -10, 1);
      vert3 = Vector4(+10, offset, +10, 1);
      vert4 = Vector4(-10, offset, +10, 1);
    }

    vert1 = model_view_no_trans.transform(vert1);
    vert2 = model_view_no_trans.transform(vert2);
    vert3 = model_view_no_trans.transform(vert3);
    vert4 = model_view_no_trans.transform(vert4);

    ConvexPolygon *poly;
    poly = &staticPoly;
    poly->vertices.clear();

    poly->append_vertex(vert1);
    poly->append_vertex(vert2);
    poly->append_vertex(vert3);
    poly->append_vertex(vert4);

    for(int k=0; k<6; k++){
      poly->clip(volume_planes[k], true);
    }

    //poly->transform(model_view_inverse);
    //poly->translate(shift_4d);
    //poly->transform(model_view);
    poly->transform(model_view_no_trans_inverse);
    poly->transform(model_view_trans);

    glPushMatrix();
    glScalef(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);

    activate_volume_shader(volume_index, true);
    glPopMatrix();

    glBegin(GL_POLYGON);
      poly->Emit(true); 
    glEnd();

    deactivate_volume_shader();
  }
  } //slice_mode


  if(volume_mode){ 
  glEnable(GL_DEPTH_TEST);

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
      poly->clip(volume_planes[k], true);
    }

    //move the volume to the proper location
    //poly->transform(model_view_inverse);
    //poly->translate(shift_4d);
    //poly->transform(model_view);

    poly->transform(model_view_no_trans_inverse);
    poly->transform(model_view_trans);

    glPushMatrix();
    glScalef(volume[volume_index]->aspect_ratio_width, volume[volume_index]->aspect_ratio_height, volume[volume_index]->aspect_ratio_depth);
    
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
    
    activate_volume_shader(volume_index, false);
    glPopMatrix();

    glBegin(GL_POLYGON);
      poly->Emit(true); 
    glEnd();

    deactivate_volume_shader();
		
  }
  } //volume_mode

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
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



void VolumeRenderer::activate_volume_shader(int volume_index, bool slice_mode){

  cgGLSetStateMatrixParameter(m_mvp_vert_std_param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
  cgGLSetStateMatrixParameter(m_mvi_vert_std_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLBindProgram(m_vert_std_vprog);
  cgGLEnableProfile(CG_PROFILE_VP30);

  cgGLSetStateMatrixParameter(m_mvi_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
  cgGLSetStateMatrixParameter(m_mv_one_volume_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
  cgGLSetTextureParameter(m_vol_one_volume_param, volume[volume_index]->id);
  cgGLSetTextureParameter(m_tf_one_volume_param, tf[volume_index]->id);
  cgGLEnableTextureParameter(m_vol_one_volume_param);
  cgGLEnableTextureParameter(m_tf_one_volume_param);

  if(!slice_mode)
    cgGLSetParameter4f(m_render_param_one_volume_param, 
		  volume[volume_index]->get_n_slice(), 
		  volume[volume_index]->get_opacity_scale(), 
		  volume[volume_index]->get_diffuse(), 
		  volume[volume_index]->get_specular());
  else
    cgGLSetParameter4f(m_render_param_one_volume_param, 
		  0.,
		  volume[volume_index]->get_opacity_scale(), 
		  volume[volume_index]->get_diffuse(), 
		  volume[volume_index]->get_specular());

  cgGLBindProgram(m_one_volume_fprog);
  cgGLEnableProfile(CG_PROFILE_FP30);
}


void VolumeRenderer::deactivate_volume_shader(){
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

void VolumeRenderer::set_slice_mode(bool val) { slice_mode = val; }
void VolumeRenderer::set_volume_mode(bool val) { volume_mode = val; }
void VolumeRenderer::switch_slice_mode() { slice_mode = (!slice_mode); }
void VolumeRenderer::switch_volume_mode() { volume_mode = (!volume_mode); }

