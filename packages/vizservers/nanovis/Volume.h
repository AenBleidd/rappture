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
#include "AxisRange.h"
#include "R2/R2Object.h"

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

class Volume : public R2Object {
public:
    int 			_volumeDataID;

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
    Vector3 _physical_min;
    Vector3 _physical_max;

    AxisRange xAxis, yAxis, zAxis, wAxis;
    static bool update_pending;
    static double valueMin, valueMax;

    int n_components;

    double nonzero_min;
    
    std::vector <CutPlane> plane; // cut planes

    Texture3D* tex;		// OpenGL texture storing the volume

    float* _data;
    
    int pointsetIndex;

    Vector3 location;

    bool enabled; 

    int n_slice;		// Number of slices when rendered. The greater
				// the better quality, lower speed.

    float _specular;		// specular lighting parameter
    float _diffuse;		// diffuse lighting parameter
    float _opacity_scale;	// The scale multiplied to the opacity assigned
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

public :
    Volume(float x, float y, float z, int width, int height, int depth, 
	   float size, int n_component, float* data, double vmin, double vmax, 
	   double nonzero_min);

protected :
    virtual ~Volume();
	
public :
    void visible(bool value) { 
	enabled = value; 
    }
    bool visible(void) { 
	return enabled; 
    }
    void move(Vector3 _loc) { 
	location = _loc; 
    }

    Vector3* get_location();
    
    void set_isosurface(int iso);
    int get_isosurface() const;

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
    float specular(void) {
	return _specular;
    }
    void specular(float value) {
	_specular = value;
    }
    float diffuse(void) {
	return _diffuse;
    }
    void diffuse(float value) {
	_diffuse = value;
    }
    float opacity_scale(void) {
	return _opacity_scale;
    }
    void opacity_scale(float value) {
	_opacity_scale = value;
    }
    void data(bool value) {
	data_enabled = value;
    }
    bool data(void) {
	return data_enabled;
    }
    void outline(bool value) {
	outline_enabled = value; 
    }
    bool outline(void) {
	return outline_enabled;
    }
    void set_outline_color(float* rgb);
    void get_outline_color(float* rgb);
    
    void set_label(int axis, const char* txt); // change the label displayed
					       // on an axis
    std::string label[3];	// the labels along each axis 0:x , 1:y, 2:z

    void setPhysicalBBox(const Vector3& min, const Vector3& max);
    Vector3& getPhysicalBBoxMin();
    Vector3& getPhysicalBBoxMax();

    void setDataID(int id);
    int getDataID() const;
};

inline Vector3* Volume::get_location() { 
    return &location; 
}
inline 
int Volume::add_cutplane(int _orientation, float _location) {
    plane.push_back(CutPlane(1, 0.5));
    return plane.size() - 1;
}

inline void 
Volume::enable_cutplane(int index)
{ 
    //assert(index < plane.size());
    plane[index].enabled = true;
}
inline void 
Volume::disable_cutplane(int index)
{
    //assert(index < plane.size());
    plane[index].enabled = false;
}

inline void 
Volume::move_cutplane(int index, float location)
{
    //assert(index < plane.size());
    plane[index].offset = location;
}

inline CutPlane* 
Volume::get_cutplane(int index)
{
    //assert(index < plane.size());
    return &plane[index];
}

inline int 
Volume::get_cutplane_count()
{
    return plane.size();
}

inline bool 
Volume::cutplane_is_enabled(int index)
{
    //assert(index < plane.size());
    return plane[index].enabled; 
}

inline void 
Volume::set_n_slice(int n) 
{ 
    n_slice = n; 
}

inline int 
Volume::get_n_slice() 
{ 
    return n_slice; 
}

inline void 
Volume::set_size(float s) 
{ 
    size = s; 
    aspect_ratio_width = s*tex->aspect_ratio_width;
    aspect_ratio_height = s*tex->aspect_ratio_height;
    aspect_ratio_depth = s*tex->aspect_ratio_depth;
}

inline void 
Volume::set_outline_color(float *rgb) 
{
    outline_color = Color(rgb[0],rgb[1],rgb[2]);
}

inline void 
Volume::get_outline_color(float *rgb) 
{
    outline_color.GetRGB(rgb);
}

inline void 
Volume::set_label(int axis, const char* txt)
{
    label[axis] = txt;
}

inline int 
Volume::get_isosurface() const
{
    return iso_surface;
}

inline void 
Volume::set_isosurface(int iso) 
{
    iso_surface = iso;
}

inline void 
Volume::setPhysicalBBox(const Vector3& min, const Vector3& max)
{
	_physical_min = min;
	_physical_max = max;

/*
	aspect_ratio_width = size * 1;
	aspect_ratio_height = size * (max.y - min.y) / (max.x - min.x);
    	aspect_ratio_depth = size* (max.z - min.z) / (max.x - min.x);

        location.x = -0.5;
        location.y = -0.5* aspect_ratio_height;
        location.z = -0.5* aspect_ratio_depth;
*/
}

inline Vector3& 
Volume::getPhysicalBBoxMin()
{
    return _physical_min;
}

inline Vector3& 
Volume::getPhysicalBBoxMax()
{
    return _physical_max;
}

inline void 
Volume::setDataID(int id)
{
	_volumeDataID = id;
}

inline int 
Volume::getDataID() const
{
	return _volumeDataID;
}

#endif
