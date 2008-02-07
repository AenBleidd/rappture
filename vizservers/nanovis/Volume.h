/*
 * ----------------------------------------------------------------------
 * Volume.h: 3d volume class
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

#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <string>
#include <vector>

#include "define.h"
#include "Color.h"
#include "Texture3D.h"
#include "Vector3.h"

struct CutPlane{
    int orient;			// orientation - 1: xy slice, 2: yz slice, 3:
				// xz slice
    float offset;		// normalized offset [0,1] in the volume
    bool enabled;

    CutPlane(int _orient, float _offset): orient(_orient), offset(_offset),
	enabled(true) {
    }
};

enum {CUBIC, VOLQD, ZINCBLENDE};

class Volume {
public:
    int width;			// The resolution of the data (how many points
				// in each direction.
    int height;			// It is different from the size of the volume
				// object drawn on screen.
    int depth;			// Width, height and depth together determing
				// the proprotion of the volume ONLY.
	
    float size;			// This is the scaling factor that will size
				// the volume on screen.  A render program
				// drawing different objects, always knows how
				// large an object is in relation to other
				// objects. This size is provided by the
				// render engine.


    int n_components;

    double vmin;		// minimum (unscaled) value in data
    double vmax;		// maximum (unscaled) value in data
    double nonzero_min;
    
    std::vector <CutPlane> plane; // cut planes

    Texture3D* tex;		// OpenGL texture storing the volume
    
    int pointsetIndex;

    Vector3 location;

    bool enabled; 

    int n_slice;		// Number of slices when rendered. The greater
				// the better quality, lower speed.

    float specular;		// specular lighting parameter
    float diffuse;		// diffuse lighting parameter
    float opacity_scale;	// The scale multiplied to the opacity assigned
				// by the transfer function.  Rule of thumb:
				// higher opacity_scale the object is to appear
				// like plastic
    
    bool data_enabled;		// show/hide cloud of volume data
    
    bool outline_enabled;	// show/hide outline around volume

    Color outline_color;	// color for outline around volume

    int volume_type;		// cubic or zincblende
    
    float aspect_ratio_width;
    float aspect_ratio_height;
    float aspect_ratio_depth;

    NVISid id;			//OpenGL textue identifier (==tex->id)

    int iso_surface;

    Volume(float x, float y, float z, int width, int height, int depth, 
	   float size, int n_component, float* data, double vmin, double vmax, 
	   double nonzero_min);
    ~Volume();
	
    void enable();		// VolumeRenderer will render an enabled
				// volume and its cutplanes
    void disable();
    void move(Vector3 _loc);
    bool is_enabled();
    Vector3* get_location();
    
	void set_isosurface(int iso);
	int get_isosurface() const;

    double range_min() { return vmin; }
    double range_max() { return vmax; }
    double range_nzero_min() { return nonzero_min; }
    
    void set_n_slice(int val);	// set number of slices
    int get_n_slice();		// return number of slices
    
    void set_size(float s);	//set the drawing size of volume 

    //methods related to cutplanes
    int add_cutplane(int _orientation, float _location); // add a plane and
							 // returns its index
    void enable_cutplane(int index);
    void disable_cutplane(int index);
    void move_cutplane(int index, float _location);
    CutPlane* get_cutplane(int index);
    int get_cutplane_count();  //returns the number of cutplanes in the volume
    bool cutplane_is_enabled(int index); //check if a cutplane is enabled

    //methods related to shading. These parameters are per volume 
    float get_specular();
    float get_diffuse();
    float get_opacity_scale();
    void set_specular(float s);
    void set_diffuse(float d);
    void set_opacity_scale(float s);
    
    void enable_data();
    void disable_data();
    bool data_is_enabled();
    
    void enable_outline();
    void disable_outline();
    bool outline_is_enabled();
    void set_outline_color(float* rgb);
    void get_outline_color(float* rgb);
    
    
    void set_label(int axis, const char* txt); // change the label displayed
					       // on an axis
    std::string label[3];	// the labels along each axis 0:x , 1:y, 2:z
};

inline void Volume::enable() { 
    enabled = true; 
}
inline void Volume::disable() { 
    enabled = false; 
}
inline void Volume::move(Vector3 _loc) { 
    location = _loc; 
}
inline bool Volume::is_enabled() { 
    return enabled; 
}
inline Vector3* Volume::get_location() { 
    return &location; 
}
inline 
int Volume::add_cutplane(int _orientation, float _location) {
    plane.push_back(CutPlane(1, 0.5));
    return plane.size() - 1;
}

inline void Volume::enable_cutplane(int index){ 
    //assert(index < plane.size());
    plane[index].enabled = true;
}
inline 
void Volume::disable_cutplane(int index){
    //assert(index < plane.size());
    plane[index].enabled = false;
}

inline void Volume::move_cutplane(int index, float location){
    //assert(index < plane.size());
    plane[index].offset = location;
}

inline CutPlane* Volume::get_cutplane(int index){
    //assert(index < plane.size());
    return &plane[index];
}

inline int Volume::get_cutplane_count(){
    return plane.size();
}

inline bool Volume::cutplane_is_enabled(int index){
    //assert(index < plane.size());
    return plane[index].enabled; 
}

inline void Volume::set_n_slice(int n) { 
    n_slice = n; 
}
inline int Volume::get_n_slice() { 
    return n_slice; 
}

inline void Volume::set_size(float s) { 
    size = s; 
    aspect_ratio_width = s*tex->aspect_ratio_width;
    aspect_ratio_height = s*tex->aspect_ratio_height;
    aspect_ratio_depth = s*tex->aspect_ratio_depth;
}

inline float Volume::get_specular() { 
    return specular; 
}

inline float Volume::get_diffuse() { 
    return diffuse; 
}

inline float Volume::get_opacity_scale() { 
    return opacity_scale; 
}

inline void Volume::set_specular(float s) { 
    specular = s; 
}

inline void Volume::set_diffuse(float d) { 
    diffuse = d; 
}

inline void Volume::set_opacity_scale(float s) { 
    opacity_scale = s; 
}

inline void Volume::enable_data() { 
    data_enabled = true; 
}

inline void Volume::disable_data() { 
    data_enabled = false; 
}

inline bool Volume::data_is_enabled() { 
    return data_enabled; 
}

inline void Volume::enable_outline() { 
    outline_enabled = true; 
}

inline void Volume::disable_outline() { 
    outline_enabled = false; 
}

inline bool Volume::outline_is_enabled() { 
    return outline_enabled; 
}

inline void Volume::set_outline_color(float *rgb) {
    outline_color = Color(rgb[0],rgb[1],rgb[2]);
}

inline void Volume::get_outline_color(float *rgb) {
    outline_color.GetRGB(rgb);
}

inline void Volume::set_label(int axis, const char* txt){
    label[axis] = txt;
}

inline int Volume::get_isosurface() const
{
	return iso_surface;
}

inline void Volume::set_isosurface(int iso) 
{
    iso_surface = iso;
}

#endif
