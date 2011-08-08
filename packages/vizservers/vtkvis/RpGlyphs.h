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
#include <vtkGlyph3D.h>
#include <vtkLookupTable.h>
#include <vtkPlaneCollection.h>

#include "RpVtkGraphicsObject.h"
#include "ColorMap.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Oriented and scaled 3D glyph shapes
 *
 * The DataSet must be a PolyData point set
 * with vectors and/or scalars
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
        OCTAHEDRON,
        SPHERE,
        TETRAHEDRON
    };
    enum ScalingMode {
        SCALE_BY_SCALAR,
        SCALE_BY_VECTOR,
        SCALE_BY_VECTOR_COMPONENTS,
        SCALING_OFF
    };
    enum ColorMode {
        COLOR_BY_SCALE,
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR
    };

    Glyphs();
    virtual ~Glyphs();

    virtual const char *getClassName() const
    {
        return "Glyphs";
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setScalingMode(ScalingMode mode);

    void setColorMode(ColorMode mode);

    void setGlyphShape(GlyphShape shape);

    void setScaleFactor(double scale);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

private:
    virtual void update();

    GlyphShape _glyphShape;
    double _scaleFactor;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkGlyph3D> _glyphGenerator;
    vtkSmartPointer<vtkPolyDataAlgorithm> _glyphSource;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
};

}
}

#endif
