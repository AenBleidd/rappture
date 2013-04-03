/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Author:
 *   Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_VOLUME_H
#define NV_VOLUME_H

#include <cstring>
#include <string>
#include <vector>

#include <vrmath/Vector3f.h>

#include "Texture3D.h"
#include "AxisRange.h"
#include "TransferFunction.h"

namespace nv {

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
     * \param numComponents Number of components per sample
     * \param data width * height * depth * numComponent sample array
     * \param vmin Scalar value minimum
     * \param vmax Scalar value maximum
     * \param nonZeroMin Scalar minimum which is greater than zero
     */
    Volume(float x, float y, float z,
           int width, int height, int depth, 
           int numComponents,
           float *data,
           double vmin, double vmax, 
           double nonZeroMin);

    virtual ~Volume();

    int width() const
    {
        return _width;
    }

    int height() const
    {
        return _height;
    }

    int depth() const
    {
        return _depth;
    }

    void visible(bool value)
    { 
        _enabled = value; 
    }

    bool visible() const
    { 
        return _enabled; 
    }

    void location(const vrmath::Vector3f& loc)
    { 
        _location = loc; 
    }

    vrmath::Vector3f location() const
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

    const float *data() const
    {
        return _data;
    }

    const Texture3D *tex() const
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

    /// Get ambient coefficient
    float ambient() const
    {
        return _ambient;
    }

    /// Set ambient coefficient [0,1]
    void ambient(float value)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        _ambient = value;
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

    /// Get specular coefficient
    float specularLevel() const
    {
        return _specular;
    }

    /// Set specular coefficient [0,1]
    void specularLevel(float value)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        _specular = value;
    }

    /// Get specular exponent
    float specularExponent() const
    {
        return _specularExp;
    }

    /// Set specular exponent [0,128]
    void specularExponent(float value)
    {
        if (value < 0.0f) value = 0.0f;
        if (value > 128.0f) value = 128.0f;
        _specularExp = value;
    }

    bool twoSidedLighting() const
    {
        return _lightTwoSide;
    }

    void twoSidedLighting(bool value)
    {
        _lightTwoSide = value;
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
        return _transferFunc;
    }

    void transferFunction(TransferFunction *transferFunc)
    {
        _transferFunc = transferFunc;
    }

    void setOutlineColor(float *rgb);

    void getOutlineColor(float *rgb);

    vrmath::Vector3f getPhysicalScaling() const
    {
        vrmath::Vector3f scale;
        scale.x = 1;
        scale.y = yAxis.length() / xAxis.length();
        scale.z = zAxis.length() / xAxis.length();
        return scale;
    }

    void getWorldSpaceBounds(vrmath::Vector3f& bboxMin,
                             vrmath::Vector3f& bboxMax) const;
 
    double sampleDistanceX() const
    {
        return (xAxis.length() / ((double)_width-1.0));
    }

    double sampleDistanceY() const
    {
        return (yAxis.length() / ((double)_height-1.0));
    }

    double sampleDistanceZ() const
    {
        if (_depth == 1)
            return sampleDistanceX();
        return (zAxis.length() / ((double)_depth-1.0));
    }

    const char *name() const
    {
        return _name.c_str();
    }

    void name(const char *name)
    {
        _name = name;
    }

    GLuint textureID() const
    {
        return _id;
    }

    AxisRange xAxis, yAxis, zAxis, wAxis;

    static bool updatePending;
    static double valueMin, valueMax;

    friend class VolumeInterpolator;

protected:
    float *data()
    {
        return _data;
    }

    Texture3D *tex()
    {
        return _tex;
    }

    GLuint _id;		///< OpenGL textue identifier (==_tex->id)

    // Width, height and depth are point resolution, NOT physical
    // units
    /// The resolution of the data (how many points in X direction)
    int _width;
    /// The resolution of the data (how many points in Y direction)
    int _height;
    /// The resolution of the data (how many points in Z direction)
    int _depth;

    /**
     * This is the designated transfer function to use to
     * render this volume.
     */
    TransferFunction *_transferFunc;

    float _ambient;      ///< Ambient material coefficient
    float _diffuse;      ///< Diffuse material coefficient
    float _specular;     ///< Specular level material coefficient
    float _specularExp;  ///< Specular exponent
    bool _lightTwoSide;  ///< Two-sided lighting flag

    /**
     * The scale multiplied to the opacity assigned by the 
     * transfer function. Rule of thumb: higher opacity_scale
     * the object is to appear like plastic
     */
    float _opacityScale;

    std::string _name;

    float *_data;

    int _numComponents;

    double _nonZeroMin;

    std::vector<CutPlane> _plane; ///< cut planes

    Texture3D *_tex;		///< OpenGL texture storing the volume

    vrmath::Vector3f _location;

    /**
     * Number of slices when rendered. The greater
     * the better quality, lower speed.
     */
    int _numSlices;
    bool _enabled; 
    bool _dataEnabled;		///< show/hide cloud of volume data
    bool _outlineEnabled;	///< show/hide outline around volume
    float _outlineColor[3];	///< color for outline around volume
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
Volume::setOutlineColor(float *rgb) 
{
    memcpy(_outlineColor, rgb, sizeof(float)*3);
}

inline void 
Volume::getOutlineColor(float *rgb) 
{
    memcpy(rgb, _outlineColor, sizeof(float)*3);
}

}

#endif
