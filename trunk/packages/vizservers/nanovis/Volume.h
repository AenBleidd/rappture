/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Volume.h: 3d volume class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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

    /**
     * \brief Volume data constructor
     *
     * Represents a 3D regular grid with uniform spacing along
     * each axis.  Sample spacing may differ between X, Y and Z
     *
     * \param x X location
     * \param y Y location
     * \param z Z location
     * \param width Number of samples in X
     * \param height Number of samples in Y
     * \param depth Number of samples in Z
     * \param size Scale factor
     * \param numComponents Number of components per sample
     * \param data width * height * depth * numComponent sample array
     * \param vmin Scalar value minimum
     * \param vmax Scalar value maximum
     * \param nonZeroMin Scalar minimum which is greater than zero
     */
    Volume(float x, float y, float z,
           int width, int height, int depth, 
           float size, int numComponents,
           float *data,
           double vmin, double vmax, 
           double nonZeroMin);

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
        return _isosurface;
    }

    void isosurface(int iso)
    {
        _isosurface = iso;
    }

    int numComponents() const
    {
        return _numComponents;
    }

    double nonZeroMin() const
    {
        return _nonZeroMin;
    }

    int volumeType() const
    {
        return _volumeType;
    }

    float *data()
    {
        return _data;
    }

    Texture3D *tex()
    {
        return _tex;
    }
    
    int numSlices() const
    {
        return _numSlices;
    }

    void numSlices(int n)
    {
        _numSlices = n;
    }

    /// set the drawing size of volume 
    void setSize(float s);

    // methods related to cutplanes
    /// add a plane and returns its index
    int addCutplane(int orientation, float location);

    void enableCutplane(int index);

    void disableCutplane(int index);

    void moveCutplane(int index, float location);

    CutPlane *getCutplane(int index);

    /// returns the number of cutplanes in the volume
    int getCutplaneCount();

    /// check if a cutplane is enabled
    bool isCutplaneEnabled(int index) const;

    // methods related to shading. These parameters are per volume 

    /// Get specular exponent
    float specular() const
    {
        return _specular;
    }

    /// Set specular exponent [0,128]
    void specular(float value)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 128.0f) value = 128.0f;
        _specular = value;
    }

    /// Get diffuse coefficient
    float diffuse() const
    {
        return _diffuse;
    }

    /// Set diffuse coefficient [0,1]
    void diffuse(float value)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        _diffuse = value;
    }

    float opacityScale() const
    {
        return _opacityScale;
    }

    void opacityScale(float value)
    {
        _opacityScale = value;
    }

    void dataEnabled(bool value)
    {
        _dataEnabled = value;
    }

    bool dataEnabled() const
    {
        return _dataEnabled;
    }

    void outline(bool value)
    {
        _outlineEnabled = value; 
    }

    bool outline()
    {
        return _outlineEnabled;
    }

    TransferFunction *transferFunction()
    {
        return _tfPtr;
    }

    void transferFunction(TransferFunction *tfPtr)
    {
        _tfPtr = tfPtr;
    }

    void setOutlineColor(float *rgb);

    void getOutlineColor(float *rgb);

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

    float aspectRatioWidth;
    float aspectRatioHeight;
    float aspectRatioDepth;

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

    static bool updatePending;
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
    float _opacityScale;

    const char *_name;
    Vector3 _physicalMin;
    Vector3 _physicalMax;
    float *_data;

    int _numComponents;

    double _nonZeroMin;

    std::vector<CutPlane> _plane; ///< cut planes

    Texture3D *_tex;		///< OpenGL texture storing the volume

    Vector3 _location;

    /**
     * Number of slices when rendered. The greater
     * the better quality, lower speed.
     */
    int _numSlices;
    bool _enabled; 
    bool _dataEnabled;		///< show/hide cloud of volume data
    bool _outlineEnabled;	///< show/hide outline around volume
    Color _outlineColor;	///< color for outline around volume
    int _volumeType;		///< cubic or zincblende
    int _isosurface;
};

inline int
Volume::addCutplane(int orientation, float location)
{
    _plane.push_back(CutPlane(orientation, location));
    return _plane.size() - 1;
}

inline void 
Volume::enableCutplane(int index)
{ 
    //assert(index < plane.size());
    _plane[index].enabled = true;
}
inline void 
Volume::disableCutplane(int index)
{
    //assert(index < plane.size());
    _plane[index].enabled = false;
}

inline void 
Volume::moveCutplane(int index, float location)
{
    //assert(index < plane.size());
    _plane[index].offset = location;
}

inline CutPlane * 
Volume::getCutplane(int index)
{
    //assert(index < plane.size());
    return &_plane[index];
}

inline int 
Volume::getCutplaneCount()
{
    return _plane.size();
}

inline bool 
Volume::isCutplaneEnabled(int index) const
{
    //assert(index < plane.size());
    return _plane[index].enabled; 
}

inline void 
Volume::setSize(float s) 
{ 
    size = s; 
    aspectRatioWidth  = s * _tex->aspectRatioWidth();
    aspectRatioHeight = s * _tex->aspectRatioHeight();
    aspectRatioDepth  = s * _tex->aspectRatioDepth();
}

inline void 
Volume::setOutlineColor(float *rgb) 
{
    _outlineColor = Color(rgb[0], rgb[1], rgb[2]);
}

inline void 
Volume::getOutlineColor(float *rgb) 
{
    _outlineColor.getRGB(rgb);
}

inline void 
Volume::setPhysicalBBox(const Vector3& min, const Vector3& max)
{
    _physicalMin = min;
    _physicalMax = max;

    /*
    aspectRatioWidth = size * 1;
    aspectRatioHeight = size * (max.y - min.y) / (max.x - min.x);
    aspectRatioDepth = size* (max.z - min.z) / (max.x - min.x);

    location.x = -0.5;
    location.y = -0.5* aspectRatioHeight;
    location.z = -0.5* aspectRatioDepth;
    */
}

inline const Vector3& 
Volume::getPhysicalBBoxMin() const
{
    return _physicalMin;
}

inline const Vector3& 
Volume::getPhysicalBBoxMax() const
{
    return _physicalMax;
}

#endif
