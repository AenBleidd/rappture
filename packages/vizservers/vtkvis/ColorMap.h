/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_COLORMAP_H
#define VTKVIS_COLORMAP_H

#include <string>
#include <list>
#include <cstring>

#include <vtkSmartPointer.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkLookupTable.h>
#include <vtkUnsignedCharArray.h>

namespace VtkVis {

/**
 * \brief RGBA color map table created from transfer function
 *
 * ColorMap uses linear interpolation between ControlPoints to
 * create a color lookup table for VTK
 */
class ColorMap {
public:
    /**
     * \brief RGB inflection point in a transfer function
     */
    struct ControlPoint {
        ControlPoint() :
            value(0),
            midpoint(0.5),
            sharpness(0)
        {
            memset(color, 0, sizeof(double)*3);
        }
        ControlPoint(const ControlPoint& other) :
            value(other.value),
            midpoint(other.midpoint),
            sharpness(other.sharpness)
        {
            memcpy(color, other.color, sizeof(double)*3);
        }
        ControlPoint& operator=(const ControlPoint &other)
        {
            if (this != &other) {
                value = other.value;
                midpoint = other.midpoint;
                sharpness = other.sharpness;
                memcpy(color, other.color, sizeof(double)*3);
            }
            return *this;
        }

        double value; ///< Normalized scalar data value [0,1]
        double midpoint, sharpness;
        double color[3]; ///< RGB color
    };

    /**
     * \brief Opacity(alpha) inflection point in a transfer function
     */
    struct OpacityControlPoint {
        OpacityControlPoint() :
            value(0),
            midpoint(0.5),
            sharpness(0),
            alpha(1)
        {
        }
        OpacityControlPoint(const OpacityControlPoint& other) :
            value(other.value),
            midpoint(other.midpoint),
            sharpness(other.sharpness),
            alpha(other.alpha)
        {
        }
        OpacityControlPoint& operator=(const OpacityControlPoint &other)
        {
            if (this != &other) {
                value = other.value;
                midpoint = other.midpoint;
                sharpness = other.sharpness;
                alpha = other.alpha;
            }
            return *this;
        }

        double value; ///< Normalized scalar data value [0,1]
        double midpoint, sharpness;
        double alpha; ///< Opacity
    };

    ColorMap(const std::string& name);

    ColorMap(const ColorMap& other);

    ColorMap& operator=(const ColorMap& other);

    virtual ~ColorMap();

    const std::string& getName();

    vtkLookupTable *getLookupTable();

    vtkSmartPointer<vtkColorTransferFunction> getColorTransferFunction(double range[2]);

    vtkSmartPointer<vtkPiecewiseFunction> getOpacityTransferFunction(double range[2], double opacityScale = 1.0);

    void setNumberOfTableEntries(int numEntries);

    int getNumberOfTableEntries();

    void addControlPoint(ControlPoint& cp);

    void addOpacityControlPoint(OpacityControlPoint& cp);

    void build();

    void clear();

    static ColorMap *getDefault();
    static ColorMap *getGrayDefault();
    static ColorMap *getVolumeDefault();
    static ColorMap *getElementDefault();

    static void renderColorMap(ColorMap *map, int width, int height,
                               vtkUnsignedCharArray *imgData,
                               bool opaque, float bgColor[3],
                               bool bgr = false,
                               int bytesPerPixel = 3);

private:
    static ColorMap *_default;
    static ColorMap *_grayDefault;
    static ColorMap *_volumeDefault;
    static ColorMap *_elementDefault;

    ColorMap();

    void lerp(double *result, const ControlPoint& cp1, const ControlPoint& cp2, double value);
    void lerp(double *result, const OpacityControlPoint& cp1, const OpacityControlPoint& cp2, double value);

    std::string _name;
    std::list<ControlPoint> _controlPoints;
    std::list<OpacityControlPoint> _opacityControlPoints;
    bool _needsBuild;
    int _numTableEntries;
    vtkSmartPointer<vtkColorTransferFunction> _colorTF;
    vtkSmartPointer<vtkPiecewiseFunction> _opacityTF;
    vtkSmartPointer<vtkLookupTable> _lookupTable;
};

}

#endif
