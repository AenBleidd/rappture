/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_MOLECULE_H__
#define __RAPPTURE_VTKVIS_MOLECULE_H__

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPropAssembly.h>
#include <vtkGlyph3D.h>

#include "ColorMap.h"
#include "RpVtkDataSet.h"

namespace Rappture {
namespace VtkVis {

/**
 * \brief Ball and stick Molecule
 *
 * The data format for this class is based on the VisIt Molecule
 * plot.  It consists of a vtkPolyData with atom coordinates as
 * points and vertices and bonds as lines.  A scalar array named
 * "element" can be included as point data to specify atomic 
 * numbers to use in coloring (via the "elementDefault" ColorMap)
 * and scaling atoms.  If the scalar array is not named 
 * "element," it will be color-mapped as a standard scalar field.
 */
class Molecule {
public:
    enum AtomScaling {
        NO_ATOM_SCALING,
        VAN_DER_WAALS_RADIUS,
        COVALENT_RADIUS,
        ATOMIC_RADIUS
    };

    Molecule();
    virtual ~Molecule();

    void setDataSet(DataSet *dataset);

    DataSet *getDataSet();

    vtkProp *getProp();

    void setLookupTable(vtkLookupTable *lut);

    vtkLookupTable *getLookupTable();

    void setAtomScaling(AtomScaling state);

    void setAtomVisibility(bool state);

    void setBondVisibility(bool state);

    void setVisibility(bool state);

    bool getVisibility();

    void setOpacity(double opacity);

    void setWireframe(bool state);

    void setEdgeVisibility(bool state);

    void setEdgeColor(float color[3]);

    void setEdgeWidth(float edgeWidth);

    void setClippingPlanes(vtkPlaneCollection *planes);

    void setLighting(bool state);

    static ColorMap *createElementColorMap();

private:
    void initProp();
    void update();

    static void addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling);

    DataSet *_dataSet;

    float _edgeColor[3];
    float _edgeWidth;
    double _opacity;
    bool _lighting;
    AtomScaling _atomScaling;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkPropAssembly> _props;
    vtkSmartPointer<vtkActor> _atomProp;
    vtkSmartPointer<vtkActor> _bondProp;
    vtkSmartPointer<vtkGlyph3D> _glypher;
    vtkSmartPointer<vtkPolyDataMapper> _atomMapper;
    vtkSmartPointer<vtkPolyDataMapper> _bondMapper;
};

}
}

#endif
