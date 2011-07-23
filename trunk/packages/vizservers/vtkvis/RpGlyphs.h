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

#include "RpVtkDataSet.h"
#include "ColorMap.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Oriented and scaled 3D glyph shapes
 */
class Glyphs {
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

    Glyphs();
    virtual ~Glyphs();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setGlyphShape(GlyphShape shape);

    void setScaleFactor(double scale);

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setOpacity(double opacity);

    double getOpacity();

    void setVisibility(bool state);

    bool getVisibility();

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

private:
    void initProp();
    void update();

    DataSet *_dataSet;

    double _opacity;
    bool _lighting;

    GlyphShape _glyphShape;
    double _scaleFactor;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _prop;
    vtkSmartPointer<vtkGlyph3D> _glyphGenerator;
    vtkSmartPointer<vtkPolyDataAlgorithm> _glyphSource;
    vtkSmartPointer<vtkPolyDataMapper> _pdMapper;
};

}
}

#endif
