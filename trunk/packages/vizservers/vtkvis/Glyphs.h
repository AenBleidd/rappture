/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_GLYPHS_H
#define VTKVIS_GLYPHS_H

#include <vector>

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkActor.h>
#include <vtkVersion.h>
#include <vtkGlyph3DMapper.h>
#include <vtkLookupTable.h>
#include <vtkPlaneCollection.h>

#include "GraphicsObject.h"
#include "ColorMap.h"

namespace VtkVis {

/**
 * \brief Oriented and scaled 3D glyph shapes
 *
 * The DataSet must have vectors and/or scalars
 */
class Glyphs : public GraphicsObject {
public:
    enum GlyphShape {
        ARROW,
        CONE,
        CUBE,
        CYLINDER,
        DODECAHEDRON,
        ICOSAHEDRON,
        LINE,
        OCTAHEDRON,
        SPHERE,
        TETRAHEDRON
    };
    enum ScalingMode {
        SCALE_BY_SCALAR,
        SCALE_BY_VECTOR_MAGNITUDE,
        SCALE_BY_VECTOR_COMPONENTS,
        SCALING_OFF
    };
    enum ColorMode {
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
    };

    Glyphs(GlyphShape shape);
    virtual ~Glyphs();

    virtual const char *getClassName() const
    {
        return "Glyphs";
    }

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setQuality(double quality);

    void setOrient(bool state);

    void setOrientMode(bool mode, const char *name);

    void setScalingMode(ScalingMode mode, const char *name, double range[2]);

    void setColorMode(ColorMode mode, const char *name, double range[2]);

    void setScalingMode(ScalingMode mode);

    void setNormalizeScale(bool normalize);

    void setColorMode(ColorMode mode);

    void setGlyphShape(GlyphShape shape);

    void setScaleFactor(double scale);

    void setColorMap(ColorMap *colorMap);

    /**
     * \brief Return the ColorMap in use
     */
    ColorMap *getColorMap()
    {
        return _colorMap;
    }

    void updateColorMap();

    virtual void updateRanges(Renderer *renderer);

private:
    Glyphs();
    virtual void update();
    static inline double min2(double a, double b)
    {
        return ((a < b) ? a : b);
    }
    static inline double max2(double a, double b)
    {
        return ((a > b) ? a : b);
    }

    GlyphShape _glyphShape;
    ScalingMode _scalingMode;
    std::string _scalingFieldName;
    double _scalingFieldRange[2];
    double _dataScale;
    double _scaleFactor;
    bool _normalizeScale;
    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataAlgorithm> _glyphSource;
    vtkSmartPointer<vtkGlyph3DMapper> _glyphMapper;
};

}

#endif
