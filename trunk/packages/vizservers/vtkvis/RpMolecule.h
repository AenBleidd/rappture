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
#include <vtkActor2D.h>
#include <vtkAssembly.h>
#include <vtkTubeFilter.h>
#include <vtkGlyph3D.h>
#include <vtkLabelPlacementMapper.h>

#include "ColorMap.h"
#include "RpVtkGraphicsObject.h"

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
class Molecule : public VtkGraphicsObject {
public:
    enum AtomScaling {
        NO_ATOM_SCALING,
        VAN_DER_WAALS_RADIUS,
        COVALENT_RADIUS,
        ATOMIC_RADIUS
    };
    enum BondColorMode {
        BOND_COLOR_BY_ELEMENTS,
        BOND_COLOR_CONSTANT
    };

    Molecule();
    virtual ~Molecule();

    virtual const char *getClassName() const
    {
        return "Molecule";
    }

    virtual vtkProp *getOverlayProp()
    {
        return _labelProp;
    }

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

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

    void setAtomScaling(AtomScaling state);

    void setAtomRadiusScale(double scale);

    void setBondRadiusScale(double scale);

    virtual void setVisibility(bool state);

    virtual void setOpacity(double opacity);

    void setAtomVisibility(bool state);

    void setAtomLabelVisibility(bool state);

    void setBondVisibility(bool state);

    void setBondColor(float color[3]);

    void setBondColorMode(BondColorMode mode);

    static ColorMap *createElementColorMap();

private:
    virtual void initProp();
    virtual void update();

    static void addLabelArray(vtkDataSet *dataSet);

    static void addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling, double scaleFactor);

    float _bondColor[3];
    double _radiusScale;
    AtomScaling _atomScaling;
    ColorMap *_colorMap;
    bool _labelsOn;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _atomProp;
    vtkSmartPointer<vtkActor> _bondProp;
    vtkSmartPointer<vtkActor2D> _labelProp;
    vtkSmartPointer<vtkGlyph3D> _glypher;
    vtkSmartPointer<vtkTubeFilter> _tuber;
    vtkSmartPointer<vtkPolyDataMapper> _atomMapper;
    vtkSmartPointer<vtkPolyDataMapper> _bondMapper;
    vtkSmartPointer<vtkLabelPlacementMapper> _labelMapper;
};

}
}

#endif
