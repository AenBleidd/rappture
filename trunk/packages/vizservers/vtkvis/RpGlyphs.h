/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_GLYPHS_H__
#define __RAPPTURE_VTKVIS_GLYPHS_H__

#include <vector>

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkActor.h>
#include <vtkVersion.h>
#if ((VTK_MAJOR_VERSION > 5) || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION >= 8))
#define HAVE_GLYPH3D_MAPPER
#include <vtkGlyph3DMapper.h>
#else
#include <vtkGlyph3D.h>
#endif
#include <vtkLookupTable.h>
#include <vtkPlaneCollection.h>

#include "RpVtkGraphicsObject.h"
#include "ColorMap.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Oriented and scaled 3D glyph shapes
 *
 * The DataSet must have vectors and/or scalars
 */
class Glyphs : public VtkGraphicsObject {
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
        COLOR_CONSTANT
    };

    Glyphs(GlyphShape shape);
    virtual ~Glyphs();

    virtual const char *getClassName() const
    {
        return "Glyphs";
    }

    virtual void setDataSet(DataSet *dataSet,
                            bool useCumulative,
                            double scalarRange[2],
                            double vectorMagnitudeRange[2],
                            double vectorComponentRange[3][2]);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

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

    virtual void updateRanges(bool useCumulative,
                              double scalarRange[2],
                              double vectorMagnitudeRange[2],
                              double vectorComponentRange[3][2]);

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
    double _dataScale;
    double _scaleFactor;
    bool _normalizeScale;
    ColorMode _colorMode;
    ColorMap *_colorMap;
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPolyDataAlgorithm> _glyphSource;
#ifdef HAVE_GLYPH3D_MAPPER
    vtkSmartPointer<vtkGlyph3DMapper> _glyphMapper;
#else
    vtkSmartPointer<vtkGlyph3D> _glyphGenerator;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
#endif
};

}
}

#endif
