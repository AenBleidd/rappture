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

#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include "nanovis.h"
#include <R2/R2string.h>
#include <R2/R2FilePath.h>
#include "VolumeRenderer.h"
#include "VolumeInterpolator.h"
#include "NvStdVertexShader.h"
#include "Trace.h"
#include "Grid.h"

#define NUMDIGITS	6

VolumeRenderer::VolumeRenderer():
  slice_mode(false),
  volume_mode(true)
{
    volumes.clear();

    init_shaders();

    const char *path = R2FilePath::getInstance()->getPath("Font.bmp");
    if (path == NULL) {
	fprintf(stderr, "can't find Font.bmp\n");
	assert(path != NULL);
    }
    init_font(path);
    delete [] path;
    _volumeInterpolator = new VolumeInterpolator();
}

VolumeRenderer::~VolumeRenderer()
{
    remove_all_volumes();

    delete _zincBlendeShader;
    delete _regularVolumeShader;
    delete _stdVertexShader;
    delete _volumeInterpolator;
}

//initialize the volume shaders
void VolumeRenderer::init_shaders(){
  
  //standard vertex program
  _stdVertexShader = new NvStdVertexShader();

  //volume rendering shader: one cubic volume
  _regularVolumeShader = new NvRegularVolumeShader();

  //volume rendering shader: one zincblende orbital volume.
  //This shader renders one orbital of the simulation.
  //A sim has S, P, D, SS orbitals. thus a full rendering requires 4 zincblende orbital volumes. 
  //A zincblende orbital volume is decomposed into 2 "interlocking" cubic 4-component volumes and passed to the shader. 
  //We render each orbital with a independent transfer functions then blend the result.
  //
  //The engine is already capable of rendering multiple volumes and combine them. Thus, we just invoke this shader on
  //S, P, D and SS orbitals with different transfor functions. The result is a multi-orbital rendering.
  _zincBlendeShader = new NvZincBlendeVolumeShader();
}

void 
VolumeRenderer::remove_all_volumes()
{
    vector<VolumeData*>::iterator iter;
    for (iter = volumes.begin(); iter != volumes.end(); ++iter)
    {
        delete (*iter);
    }
    volumes.clear();
}

void 
VolumeRenderer::add_volume(Volume* vol, TransferFunction* tf)
{
    VolumeData * volData = new VolumeData(vol, tf); 
    volumes.push_back(volData);
    printf("volume[%d] added\n", volData->volume->getDataID());
}

/*
 * FIXME:  This is a good example of the wrong data structures being used.
 *	   The volumes should be in a list not a vector.  Each list
 *	   element should contain pointers to both the volume and the
 *	   transfer function.  The shouldn't be parallel vectors.  
 * 
 *	   And there shouldn't be one vector for "all" volumes and another
 *	   for those being rendered, unless you work to keep them in sync.  
 *	   Use one master list instead.
 */
void
VolumeRenderer::remove_volume(size_t volDataID)
{
    std::vector<VolumeData*>::iterator iter;
    for (iter = volumes.begin(); iter != volumes.end(); ++iter)
    {
        if ((*iter)->volume->getDataID() == volDataID)
        {
        	delete (*iter);
        	volumes.erase(iter);
                break;
        }
    }
}

void
VolumeRenderer::shade_volume(Volume* _vol, TransferFunction* _tf)
{
    if (_vol == 0) printf("shade volume : null vol parameter\n");
    for (unsigned int i=0; i < volumes.size(); i++) {
        if (volumes[i]->volume == _vol) {
	    if (_tf != volumes[i]->tf)
            {
                _tf->ref();

                if (volumes[i]->tf)
                    volumes[i]->tf->unref();

                volumes[i]->tf = _tf;
		printf("transfer function changed [volid %d]\n", volumes[i]->volume->getDataID());
	    }
        }
    }
}


TransferFunction*
VolumeRenderer::get_volume_shading(Volume* _vol)
{
    for (unsigned int i=0; i < volumes.size(); i++) {
        if (volumes[i]->volume == _vol) {
            return volumes[i]->tf;
        }
    }
    return NULL;
}


struct SortElement {
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

void VolumeRenderer::render_all_points()
{
#ifdef notdef
    double x0 = 0;
    double y0 = 0;
    double z0 = 0;
    
    for(int i=0; i<n_volumes; i++){
	int volume_index = i;
	if(!volume[i]->visible())
	    continue; //skip this volume
	
	Vector3* location = volume[volume_index]->get_location();
	Vector4 shift_4d(location->x, location->y, location->z, 0);
	
	glPushMatrix();
	glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
	
	if (volume[volume_index]->outline()) {
	    float olcolor[3];
	    volume[volume_index]->get_outline_color(olcolor);
	    draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1,
		(double)olcolor[0], (double)olcolor[1], (double)olcolor[2], 1.5);
	}
	
	glPopMatrix();
    }
#endif
}

void 
VolumeRenderer::render_all()
{
    Volume* cur_vol = 0;
    Volume* ani_vol = 0;
    TransferFunction* cur_tf = 0;
    TransferFunction* ani_tf = 0;
    int total_rendered_slices = 0;
    int num_volumes = volumes.size();

    if (_volumeInterpolator->is_started()) {
        ++num_volumes;
        ani_tf = volumes[_volumeInterpolator->getReferenceVolumeID()]->tf;
        ani_vol = _volumeInterpolator->getVolume();
    }

    //two dimension pointer array
    ConvexPolygon*** polys = new ConvexPolygon**[num_volumes];	
    //number of actual slices for each volume
    int* actual_slices = new int[num_volumes]; 


    Volume* vol;
    int vol_index = 0;
    std::vector<VolumeData*>::iterator iter;
    for (iter = volumes.begin(); iter != volumes.end(); ++iter, ++vol_index) {
	cur_vol = NULL;
	cur_tf = NULL;
#ifdef notdef
        if (vol_index != n_volumes) {
            cur_vol = (*iter)->volume;
            cur_tf = (*iter)->tf;
        } else {
            cur_vol = ani_vol;
            cur_tf = ani_tf;
        }
#else
	cur_vol = (*iter)->volume;
	cur_tf = (*iter)->tf;
#endif

        polys[vol_index] = NULL;
        actual_slices[vol_index] = 0;

        if(!cur_vol->visible()) {
            //printf("volume [%d] skipped\n", cur_vol->getDataID());
            continue; //skip this volume
	}

        int n_slices = cur_vol->get_n_slice();
        if (cur_vol->get_isosurface()) {
	    // double the number of slices
    	    n_slices <<= 1;
	}
	
        //volume start location
        Vector3* location = cur_vol->get_location();
        Vector4 shift_4d(location->x, location->y, location->z, 0);
	
        double x0 = 0;
        double y0 = 0;
        double z0 = 0;
	
        Mat4x4 model_view_no_trans, model_view_trans;
        Mat4x4 model_view_no_trans_inverse, model_view_trans_inverse;
	
        double zNear, zFar;
	
	//initialize volume plane with world coordinates
        Plane volume_planes[6];
        volume_planes[0].set_coeffs( 1,  0,  0, -x0);
        volume_planes[1].set_coeffs(-1,  0,  0,  x0+1);
        volume_planes[2].set_coeffs( 0,  1,  0, -y0);
        volume_planes[3].set_coeffs( 0, -1,  0,  y0+1);
        volume_planes[4].set_coeffs( 0,  0,  1, -z0);
        volume_planes[5].set_coeffs( 0,  0, -1,  z0+1);
	
	//get modelview matrix with no translation
        glPushMatrix();
        glScalef(cur_vol->aspect_ratio_width, 
		 cur_vol->aspect_ratio_height, 
		 cur_vol->aspect_ratio_depth);
	
        glEnable(GL_DEPTH_TEST);
	
        GLfloat mv_no_trans[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mv_no_trans);
	
        model_view_no_trans = Mat4x4(mv_no_trans);
        model_view_no_trans_inverse = model_view_no_trans.inverse();
	
        glPopMatrix();
	
	//get modelview matrix with translation
        glPushMatrix();
        glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
        glScalef(cur_vol->aspect_ratio_width, 
		 cur_vol->aspect_ratio_height, 
		 cur_vol->aspect_ratio_depth);
        GLfloat mv_trans[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mv_trans);
	
        model_view_trans = Mat4x4(mv_trans);
        model_view_trans_inverse = model_view_trans.inverse();
	
	//draw volume bounding box with translation (the correct location in
	//space)
        if (cur_vol->outline()) {
            float olcolor[3];
            cur_vol->get_outline_color(olcolor);
            draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1,
		(double)olcolor[0], (double)olcolor[1], (double)olcolor[2],
		1.5);
        }
        glPopMatrix();
	
        //draw labels
        glPushMatrix();
        glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
        if(cur_vol->outline()) {
	    //draw_label(i);
        }
        glPopMatrix();
	
	//transform volume_planes to eye coordinates.
        for(int i=0; i<6; i++)
            volume_planes[i].transform(model_view_no_trans);
	
        get_near_far_z(mv_no_trans, zNear, zFar);
	
	//compute actual rendering slices
        float z_step = fabs(zNear-zFar)/n_slices;		
        int n_actual_slices;
	
        if (cur_vol->data()) {
            n_actual_slices = (int)(fabs(zNear-zFar)/z_step + 1);
            polys[vol_index] = new ConvexPolygon*[n_actual_slices];
        } else {
            n_actual_slices = 0;
            polys[vol_index] = NULL;
        }
        actual_slices[vol_index] = n_actual_slices;
	
        Vector4 vert1 = (Vector4(-10, -10, -0.5, 1));
        Vector4 vert2 = (Vector4(-10, +10, -0.5, 1));
        Vector4 vert3 = (Vector4(+10, +10, -0.5, 1));
        Vector4 vert4 = (Vector4(+10, -10, -0.5, 1));
	
	
	// Render cutplanes first with depth test enabled.  They will mark the
	// image with their depth values.  Then we render other volume slices.
	// These volume slices will be occluded correctly by the cutplanes and
	// vice versa.
	
	ConvexPolygon static_poly;
	for(int i=0; i<cur_vol->get_cutplane_count(); i++) {
	    if(!cur_vol->cutplane_is_enabled(i))
		continue;
	    
	    float offset = cur_vol->get_cutplane(i)->offset;
	    int axis = cur_vol->get_cutplane(i)->orient;
	    
	    if(axis==3){
		vert1 = Vector4(-10, -10, offset, 1);
		vert2 = Vector4(-10, +10, offset, 1);
		vert3 = Vector4(+10, +10, offset, 1);
		vert4 = Vector4(+10, -10, offset, 1);
		//continue;
	    } else if(axis==1){
		vert1 = Vector4(offset, -10, -10, 1);
		vert2 = Vector4(offset, +10, -10, 1);
		vert3 = Vector4(offset, +10, +10, 1);
		vert4 = Vector4(offset, -10, +10, 1);
		//continue;
	    } else if(axis==2){
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
	    glScalef(cur_vol->aspect_ratio_width, cur_vol->aspect_ratio_height,
		     cur_vol->aspect_ratio_depth);
	    
	    activate_volume_shader(cur_vol, cur_tf, true);
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
        float slice_z;
        for (int i=0; i<n_actual_slices; i++) {
	    slice_z = zFar + i * z_step;	//back to front
	    
            ConvexPolygon *poly = new ConvexPolygon();
            polys[vol_index][counter] = poly;
            counter++;
	    
            poly->vertices.clear();
            poly->set_id(vol_index);
	    
            //Setting Z-coordinate 
            vert1.z = slice_z;
            vert2.z = slice_z;
            vert3.z = slice_z;
            vert4.z = slice_z;
	    
            poly->append_vertex(vert1);
            poly->append_vertex(vert2);
            poly->append_vertex(vert3);
            poly->append_vertex(vert4);
	    
            for(int k=0; k<6; k++) {
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
    
    SortElement* slices = (SortElement*)malloc(sizeof(SortElement)*total_rendered_slices);
    
    int counter = 0;
    for(int i=0; i<num_volumes; i++){
        for(int j=0; j<actual_slices[i]; j++){
            if(polys[i][j]->vertices.size() >= 3){
                slices[counter] = SortElement(polys[i][j]->vertices[0].z, i, j);
                counter++;
            }
        }
    }
    
    //sort them
    qsort(slices, total_rendered_slices, sizeof(SortElement), slice_sort);
    
    //Now we are ready to render all the slices from back to front
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    
    for(int i=0; i<total_rendered_slices; i++){
        int volume_index = slices[i].volume_id;
        int slice_index = slices[i].slice_id;
        ConvexPolygon* cur = polys[volume_index][slice_index];
	
#ifdef notdef
        if (volume_index == n_volumes) {
	    cur_vol = ani_vol;
	    cur_tf = ani_tf;
	} else {
            cur_vol = volumes[volume_index]->volume;
            cur_tf = volumes[volume_index]->tf;
        }
#else
        cur_vol = volumes[volume_index]->volume;
        cur_tf = volumes[volume_index]->tf;
#endif
	
	
        glPushMatrix();
        glScalef(cur_vol->aspect_ratio_width, cur_vol->aspect_ratio_height, cur_vol->aspect_ratio_depth);
	
        activate_volume_shader(cur_vol, cur_tf, false);
        glPopMatrix();
	
        glBegin(GL_POLYGON);
        cur->Emit(true); 
        glEnd();

        deactivate_volume_shader();
    }
    
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    //Deallocate all the memory used
    for(int i=0; i<num_volumes; i++){
        for(int j=0; j<actual_slices[i]; j++){
            delete polys[i][j];
        }
	
        if (polys[i]) {
            delete[] polys[i];
        }
    }
    
    delete[] polys;
    delete[] actual_slices;
    free(slices);
}

void VolumeRenderer::render(int volDataID)
{
    Volume* vol = 0;
    TransferFunction* tf = 0;
    std::vector<VolumeData*>::iterator iter;
    for (iter = volumes.begin(); iter != volumes.end(); ++iter)
    {
        if ((*iter)->volume && ((*iter)->volume->getDataID() == volDataID))
        {
            vol = (*iter)->volume;
            tf = (*iter)->tf;
	    break;
 	}
    }

    if (vol == 0) return;

  int n_slices = vol->get_n_slice();

  //volume start location
  Vector4 shift_4d(vol->location.x, 
		   vol->location.y, 
		   vol->location.z, 0);

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

  glScalef(vol->aspect_ratio_width, 
	  vol->aspect_ratio_height, 
	  vol->aspect_ratio_depth);

  glEnable(GL_DEPTH_TEST);

  GLfloat mv_no_trans[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv_no_trans);

  model_view_no_trans = Mat4x4(mv_no_trans);
  model_view_no_trans_inverse = model_view_no_trans.inverse();

  glPopMatrix();

  //get modelview matrix with translation
  glPushMatrix();
  glTranslatef(shift_4d.x, shift_4d.y, shift_4d.z);
  glScalef(vol->aspect_ratio_width, 
	  vol->aspect_ratio_height, 
	  vol->aspect_ratio_depth);
  GLfloat mv_trans[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv_trans);

  model_view_trans = Mat4x4(mv_trans);
  model_view_trans_inverse = model_view_trans.inverse();

  //draw volume bounding box
  if (vol->outline()) {
      draw_bounding_box(x0, y0, z0, x0+1, y0+1, z0+1, 0.8, 0.1, 0.1, 1.5);
  }
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
  for(int i=0; i<vol->get_cutplane_count(); i++){
    float offset = vol->get_cutplane(i)->offset;
    int axis = vol->get_cutplane(i)->orient;

    if (axis==1) {
      vert1 = Vector4(-10, -10, offset, 1);
      vert2 = Vector4(-10, +10, offset, 1);
      vert3 = Vector4(+10, +10, offset, 1);
      vert4 = Vector4(+10, -10, offset, 1);
    } else if(axis==2){
      vert1 = Vector4(offset, -10, -10, 1);
      vert2 = Vector4(offset, +10, -10, 1);
      vert3 = Vector4(offset, +10, +10, 1);
      vert4 = Vector4(offset, -10, +10, 1);
    } else if(axis==3) {
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
    glScalef(vol->aspect_ratio_width, vol->aspect_ratio_height, vol->aspect_ratio_depth);

    activate_volume_shader(vol, tf, true);
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
    glScalef(vol->aspect_ratio_width, vol->aspect_ratio_height, vol->aspect_ratio_depth);
    
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
    
    activate_volume_shader(vol, tf, true);
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

void 
VolumeRenderer::draw_bounding_box(
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float r, float g, float b, 
    float line_width)
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4d(r, g, b, 1.0);
    glLineWidth(line_width);
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y1, z0);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z1);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x0, y0, z1);
	glVertex3d(x0, y1, z1);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x1, y1, z0);
    }
    glEnd();

#ifdef notdef
    /* Rappture doesn't supply axis units yet. So turn labeling off until we
     * can display the proper units with the distance of each bounding box
     * dimension.
     */
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f); 

    if (NanoVis::fonts != NULL) {
	double mv[16], prjm[16];
	int viewport[4];
	double wx, wy, wz;
	double dx, dy, dz;

	glGetDoublev(GL_MODELVIEW_MATRIX, mv);
	glGetDoublev(GL_PROJECTION_MATRIX, prjm);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	NanoVis::fonts->begin();
	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;
	if (gluProject((x0 + x1) * 0.5, y0, z0, mv, prjm, viewport, 
		       &wx, &wy, &wz)) {
	    char buff[20];
	    double min, max;

	    NanoVis::grid->xAxis.GetDataLimits(min, max);
	    glLoadIdentity();
	    glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);
	    const char *units;
	    units = NanoVis::grid->xAxis.GetUnits();
	    sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
	    NanoVis::fonts->draw(buff);
	}
	if (gluProject(x0, (y0 + y1) * 0.5, z0, mv, prjm, viewport, &
		       wx, &wy, &wz)) {
	    char buff[20];
	    double min, max;

	    NanoVis::grid->yAxis.GetDataLimits(min, max);
	    glLoadIdentity();
	    glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);
	    const char *units;
	    units = NanoVis::grid->yAxis.GetUnits();
	    sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
	    NanoVis::fonts->draw(buff);
	}
	if (gluProject(x0, y0, (z0 + z1) * 0.5, mv, prjm, viewport, 
		       &wx, &wy, &wz)) {
	    glLoadIdentity();
	    glTranslatef((int)wx, viewport[3] - (int) wy, 0.0f);

	    double min, max;
	    NanoVis::grid->zAxis.GetDataLimits(min, max);
	    const char *units;
	    units = NanoVis::grid->zAxis.GetUnits();
	    char buff[20];
	    sprintf(buff, "%.*g %s", NUMDIGITS, max - min, units);
	    NanoVis::fonts->draw(buff);
	}
	NanoVis::fonts->end();
    };
#endif
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}



void VolumeRenderer::activate_volume_shader(Volume* vol, TransferFunction* tf, bool slice_mode)
{
  //vertex shader
  _stdVertexShader->bind();

  if (vol->volume_type == CUBIC)
  {
    //regular cubic volume
    _regularVolumeShader->bind(tf->id, vol, slice_mode);
  }
  else if (vol->volume_type == ZINCBLENDE)
  {
    _zincBlendeShader->bind(tf->id, vol, slice_mode);
  }
}


void VolumeRenderer::deactivate_volume_shader()
{
    _stdVertexShader->unbind();
    _regularVolumeShader->unbind();
    _zincBlendeShader->unbind();
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

  for(int i=0;i<8;i++) {
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

void VolumeRenderer::enable_volume(int volDataID)
{
   printf("enable volume [%d]\n", volDataID);
   std::vector<VolumeData*>::iterator iter;
   for (iter = volumes.begin(); iter != volumes.end(); ++iter)
   {
       if ((*iter)->volume && ((*iter)->volume->getDataID() == volDataID))
       {
          (*iter)->volume->visible(true);
          printf("volume [%d] is visible\n", volDataID);
       }
   }
}

void VolumeRenderer::disable_volume(int volDataID)
{
   printf("disable volume [%d]\n", volDataID);
   std::vector<VolumeData*>::iterator iter;
   for (iter = volumes.begin(); iter != volumes.end(); ++iter)
   {
       if ((*iter)->volume && ((*iter)->volume->getDataID() == volDataID))
       {
          (*iter)->volume->visible(false);
          printf("volume [%d] is visible\n", volDataID);
       }
   }
}


bool 
VolumeRenderer::init_font(const char* filename) 
{
    FILE *f;
    unsigned short int bfType;
    int bfOffBits;
    short int biPlanes;
    short int biBitCount;
    int biSizeImage;
    int width, height;
    int i;
    unsigned char temp;
    unsigned char* data;
    /* make sure the file is there and open it read-only (binary) */
    f = fopen(filename, "rb");
    if (f == NULL) {
	fprintf(stderr, "can't open font file \"%s\"\n", filename);
	return false;
    }
    
    if (fread(&bfType, sizeof(short int), 1, f) != 1) {
	fprintf(stderr, "can't read %d bytes from font file \"%s\"\n", 
		sizeof(short int), filename);
	goto error;
    }
    
    /* check if file is a bitmap */
    if (bfType != 19778) {
	fprintf(stderr, "not a bmp file.\n");
	goto error;
    }
    
    /* get the file size */
    /* skip file size and reserved fields of bitmap file header */
    fseek(f, 8, SEEK_CUR);
    
    /* get the position of the actual bitmap data */
    if (fread(&bfOffBits, sizeof(int), 1, f) != 1) {
	fprintf(stderr, "error reading file.\n");
	goto error;
    }
    //printf("Data at Offset: %ld\n", bfOffBits);
    
    /* skip size of bitmap info header */
    fseek(f, 4, SEEK_CUR);
    
    /* get the width of the bitmap */
    if (fread(&width, sizeof(int), 1, f) != 1) {
	fprintf(stderr, "error reading file.\n");
	goto error;
    }
    //printf("Width of Bitmap: %d\n", texture->width);
    
    /* get the height of the bitmap */
    if (fread(&height, sizeof(int), 1, f) != 1) {
	fprintf(stderr, "error reading file.\n");
	goto error;
    }
    //printf("Height of Bitmap: %d\n", texture->height);
    
    /* get the number of planes (must be set to 1) */
    if (fread(&biPlanes, sizeof(short int), 1, f) != 1) {
	fprintf(stderr, "error reading file.\n");
	goto error;
    }
    if (biPlanes != 1) {
	fprintf(stderr, "Error: number of Planes not 1!\n");
	goto error;
    }
    
    /* get the number of bits per pixel */
    if (fread(&biBitCount, sizeof(short int), 1, f) != 1) {
 	fprintf(stderr, "error reading file.\n");
	goto error;
    }
    
    //printf("Bits per Pixel: %d\n", biBitCount);
    if (biBitCount != 24) {
	fprintf(stderr, "Bits per Pixel not 24\n");
	goto error;
    }


    /* calculate the size of the image in bytes */
    biSizeImage = width * height * 3 * sizeof(unsigned char);
    data = (unsigned char*) malloc(biSizeImage);
    if (data == NULL) {
	fprintf(stderr, "Can't allocate memory for image\n");
	goto error;
    }

    /* seek to the actual data */
    fseek(f, bfOffBits, SEEK_SET);
    if (fread(data, biSizeImage, 1, f) != 1) {
	fprintf(stderr, "Error loading file!\n");
	goto error;
    }
    fclose(f);

    /* swap red and blue (bgr -> rgb) */
    for (i = 0; i < biSizeImage; i += 3) {
       temp = data[i];
       data[i] = data[i + 2];
       data[i + 2] = temp;
    }

    //insert alpha channel
    unsigned char* data_with_alpha;
    data_with_alpha = (unsigned char*)
	malloc(width*height*4*sizeof(unsigned char));
    for(int i=0; i<height; i++){
	for(int j=0; j<width; j++){
	    unsigned char r, g, b, a;
	    r = data[3*(i*width+j)];
	    g = data[3*(i*width+j)+1];
	    b = data[3*(i*width+j)+2];
	    
	    if(r==0 && g==0 && b==0)
		a = 0;
	    else
		a = 255;
	    
	    data_with_alpha[4*(i*width+j)] = r;
	    data_with_alpha[4*(i*width+j) + 1] = g;
	    data_with_alpha[4*(i*width+j) + 2] = b;
	    data_with_alpha[4*(i*width+j) + 3] = a;
	    
	}
    }
    free(data);
    
    //create opengl texture 
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    //glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_with_alpha);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    free(data_with_alpha);

    build_font();
    return (glGetError()==0);

 error:
    fclose(f);
    return false;
}



void 
VolumeRenderer::draw_label(Volume* vol)
{

  //glEnable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);

  //x
  glColor3f(0.5, 0.5, 0.5);

  int length = vol->label[0].size();
  glPushMatrix();
  
    glTranslatef(.5*vol->aspect_ratio_width, vol->aspect_ratio_height, -0.1*vol->aspect_ratio_depth);
    glRotatef(180, 0, 0, 1);
    glRotatef(90, 1, 0, 0);

    glScalef(0.0008, 0.0008, 0.0008);
    for(int i=0; i<length; i++){
      glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[0].c_str()[i]);
      glTranslatef(0.04, 0., 0.);
    }
  glPopMatrix();

  //y
  length = vol->label[1].size();
  glPushMatrix();
    glTranslatef(vol->aspect_ratio_width, 0.5*vol->aspect_ratio_height, -0.1*vol->aspect_ratio_depth);
    glRotatef(90, 0, 1, 0);
    glRotatef(90, 0, 0, 1);

    glScalef(0.0008, 0.0008, 0.0008);
    for(int i=0; i<length; i++){
      glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[1].c_str()[i]);
      glTranslatef(0.04, 0., 0.);
    }
  glPopMatrix();


  //z
  length = vol->label[2].size();
  glPushMatrix();
    glTranslatef(0., 1.*vol->aspect_ratio_height, 0.5*vol->aspect_ratio_depth);
    glRotatef(90, 0, 1, 0);

    glScalef(0.0008, 0.0008, 0.0008);
    for(int i=0; i<length; i++){
      glutStrokeCharacter(GLUT_STROKE_ROMAN, vol->label[2].c_str()[i]);
      glTranslatef(0.04, 0., 0.);
    }
  glPopMatrix();

  glDisable(GL_TEXTURE_2D);
}



void VolumeRenderer::build_font() {

    GLfloat cx, cy;         /* the character coordinates in our texture */
    font_base = glGenLists(256);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    for (int loop = 0; loop < 256; loop++)
    {
        cx = (float) (loop % 16) / 16.0f;
        cy = (float) (loop / 16) / 16.0f;
        glNewList(font_base + loop, GL_COMPILE);
            glBegin(GL_QUADS);
                glTexCoord2f(cx, 1 - cy - 0.0625f);
                glVertex3f(0, 0, 0);
                glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);
                glVertex3f(0.04, 0, 0);
                glTexCoord2f(cx + 0.0625f, 1 - cy);
                glVertex3f(0.04, 0.04, 0);
                glTexCoord2f(cx, 1 - cy);
                glVertex3f(0, 0.04, 0);
            glEnd();
            glTranslated(0.04, 0, 0);
        glEndList();
    }
}


   
void VolumeRenderer::glPrint(char* string, int set){

    if(set>1) set=1;

    glBindTexture(GL_TEXTURE_2D, font_texture);

    glListBase(font_base - 32 + (128 * set));
    glCallLists(strlen(string), GL_BYTE, string);

}

