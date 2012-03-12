/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#ifndef VOLUME_H
#define VOLUME_H

#include <string>
#include <vector>

#include <R2/R2Object.h>

#include "define.h"
#include "Color.h"
#include "Texture3D.h"
#include "Vector3.h"
#include "AxisRange.h"
#include "TransferFunction.h"

struct CutPlane {
    /// orientation - 1: xy slice, 2: yz slice, 3: xz slice
    int orient;
    float offset;	///< normalized offset [0,1] in the volume
    bool enabled;

    CutPlane(int _orient, float _offset) :
        orient(_orient),
        offset(_offset),
        enabled(true)
    {
    }
};

class VolumeInterpolator;

class Volume
{
public:
    enum VolumeType {
        CUBIC,
        VOLQD,
        ZINCBLENDE
    };

    Volume(float x, float y, float z,
           int width, int height, int depth, 
           float size, int n_component,
           float *data,
           double vmin, double vmax, 
           double nonzero_min);

    virtual ~Volume();

    void visible(bool value)
    { 
        _enabled = value; 
    }

    bool visible() const
    { 
        return _enabled; 
    }

    void location(const Vector3& loc)
    { 
        _location = loc; 
    }

    Vector3 location() const
    {
        return _location;
    }

    int isosurface() const
    {
        return _iso_surface;
    }

    void isosurface(int iso)
    {
        _iso_surface = iso;
    }

    int n_components() const
    {
        return _n_components;
    }

    double nonzero_min() const
    {
        return _nonzero_min;
    }

    double range_nzero_min() const
    {
        return _nonzero_min;
    }

    int volume_type() const
    {
        return _volume_type;
    }

    float *data()
    {
        return _data;
    }

    Texture3D *tex()
    {
        return _tex;
    }
    
    int n_slices() const
    {
        return _n_slices;
    }

    void n_slices(int n)
    {
        _n_slices = n;
    }

    /// set the drawing size of volume 
    void set_size(float s);

    // methods related to cutplanes
    /// add a plane and returns its index
    int add_cutplane(int orientation, float location);

    void enable_cutplane(int index);

    void disable_cutplane(int index);

    void move_cutplane(int index, float location);

    CutPlane *get_cutplane(int index);

    /// returns the number of cutplanes in the volume
    int get_cutplane_count();

    /// check if a cutplane is enabled
    bool cutplane_is_enabled(int index) const;

    // methods related to shading. These parameters are per volume 
    float specular() const
    {
        return _specular;
    }

    void specular(float value)
    {
        _specular = value;
    }

    float diffuse() const
    {
        return _diffuse;
    }

    void diffuse(float value)
    {
        _diffuse = value;
    }

    float opacity_scale() const
    {
        return _opacity_scale;
    }

    void opacity_scale(float value)
    {
        _opacity_scale = value;
    }

    void data_enabled(bool value)
    {
        _data_enabled = value;
    }

    bool data_enabled() const
    {
        return _data_enabled;
    }

    void outline(bool value)
    {
        _outline_enabled = value; 
    }

    bool outline()
    {
        return _outline_enabled;
    }

    TransferFunction *transferFunction()
    {
        return _tfPtr;
    }

    void transferFunction(TransferFunction *tfPtr)
    {
        _tfPtr = tfPtr;
    }

    void set_outline_color(float *rgb);

    void get_outline_color(float *rgb);
    
    /// change the label displayed on an axis
    void set_label(int axis, const char *txt);

    void setPhysicalBBox(const Vector3& min, const Vector3& max);

    const Vector3& getPhysicalBBoxMin() const;

    const Vector3& getPhysicalBBoxMax() const;

    const char *name() const
    {
        return _name;
    }

    void name(const char *name)
    {
        _name = name;
    }

    float aspect_ratio_width;
    float aspect_ratio_height;
    float aspect_ratio_depth;

    GLuint id;		///< OpenGL textue identifier (==_tex->id)

    // Width, height and depth are point resolution, NOT physical
    // units
    /// The resolution of the data (how many points in X direction)
    int width;
    /// The resolution of the data (how many points in Y direction)
    int height;
    /// The resolution of the data (how many points in Z direction)
    int depth;
    /**
     * This is the scaling factor that will size the volume on screen.
     * A render program drawing different objects, always knows how
     * large an object is in relation to other objects. This size is 
     * provided by the render engine.
     */
    float size;

    int pointsetIndex;

    AxisRange xAxis, yAxis, zAxis, wAxis;
    std::string label[3]; ///< the labels along each axis 0:x, 1:y, 2:z

    static bool update_pending;
    static double valueMin, valueMax;

protected:
    /**
     * This is the designated transfer function to use to
     * render this volume.
     */
    TransferFunction *_tfPtr;

    float _specular;		///< Specular lighting parameter
    float _diffuse;		///< Diffuse lighting parameter
    /**
     * The scale multiplied to the opacity assigned by the 
     * transfer function. Rule of thumb: higher opacity_scale
     * the object is to appear like plastic
     */
    float _opacity_scale;

    const char *_name;
    Vector3 _physical_min;
    Vector3 _physical_max;
    float *_data;

    int _n_components;

    double _nonzero_min;

    std::vector<CutPlane> _plane; ///< cut planes

    Texture3D *_tex;		///< OpenGL texture storing the volume

    Vector3 _location;

    /**
     * Number of slices when rendered. The greater
     * the better quality, lower speed.
     */
    int _n_slices;
    bool _enabled; 
    bool _data_enabled;		///< show/hide cloud of volume data
    bool _outline_enabled;	///< show/hide outline around volume
    Color _outline_color;	///< color for outline around volume
    int _volume_type;		///< cubic or zincblende
    int _iso_surface;
};

inline int
Volume::add_cutplane(int orientation, float location)
{
    _plane.push_back(CutPlane(orientation, location));
    return _plane.size() - 1;
}

inline void 
Volume::enable_cutplane(int index)
{ 
    //assert(index < plane.size());
    _plane[index].enabled = true;
}
inline void 
Volume::disable_cutplane(int index)
{
    //assert(index < plane.size());
    _plane[index].enabled = false;
}

inline void 
Volume::move_cutplane(int index, float location)
{
    //assert(index < plane.size());
    _plane[index].offset = location;
}

inline CutPlane * 
Volume::get_cutplane(int index)
{
    //assert(index < plane.size());
    return &_plane[index];
}

inline int 
Volume::get_cutplane_count()
{
    return _plane.size();
}

inline bool 
Volume::cutplane_is_enabled(int index) const
{
    //assert(index < plane.size());
    return _plane[index].enabled; 
}

inline void 
Volume::set_size(float s) 
{ 
    size = s; 
    aspect_ratio_width  = s * _tex->aspect_ratio_width;
    aspect_ratio_height = s * _tex->aspect_ratio_height;
    aspect_ratio_depth  = s * _tex->aspect_ratio_depth;
}

inline void 
Volume::set_outline_color(float *rgb) 
{
    _outline_color = Color(rgb[0], rgb[1], rgb[2]);
}

inline void 
Volume::get_outline_color(float *rgb) 
{
    _outline_color.getRGB(rgb);
}

inline void 
Volume::set_label(int axis, const char* txt)
{
    label[axis] = txt;
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

inline const Vector3& 
Volume::getPhysicalBBoxMin() const
{
    return _physical_min;
}

inline const Vector3& 
Volume::getPhysicalBBoxMax() const
{
    return _physical_max;
}

#endif
