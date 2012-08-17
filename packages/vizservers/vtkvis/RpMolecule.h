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
#if ((VTK_MAJOR_VERSION > 5) || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION >= 8))
#define MOLECULE_USE_GLYPH3D_MAPPER
#include <vtkGlyph3DMapper.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCylinderSource.h>
#include <vtkLineSource.h>
#else
#include <vtkGlyph3D.h>
#include <vtkTubeFilter.h>
#endif
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
    enum BondStyle {
        BOND_STYLE_CYLINDER,
        BOND_STYLE_LINE
    };
    enum ColorMode {
        COLOR_BY_ELEMENTS,
        COLOR_BY_SCALAR,
        COLOR_BY_VECTOR_MAGNITUDE,
        COLOR_BY_VECTOR_X,
        COLOR_BY_VECTOR_Y,
        COLOR_BY_VECTOR_Z,
        COLOR_CONSTANT
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

    virtual void setDataSet(DataSet *dataSet,
                            Renderer *renderer);

    virtual void setClippingPlanes(vtkPlaneCollection *planes);

    void setColorMode(ColorMode mode, DataSet::DataAttributeType type,
                      const char *name, double range[2] = NULL);

    void setColorMode(ColorMode mode,
                      const char *name, double range[2] = NULL);

    void setColorMode(ColorMode mode);

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

    void setBondStyle(BondStyle style);

    void setBondColor(float color[3]);

    void setBondColorMode(BondColorMode mode);

    static ColorMap *createElementColorMap();

private:
    virtual void initProp();
    virtual void update();

#ifdef MOLECULE_USE_GLYPH3D_MAPPER
    void setupBondPolyData();
#endif

    static void addLabelArray(vtkDataSet *dataSet);

    static void addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling, double scaleFactor);

    float _bondColor[3];
    double _radiusScale;
    AtomScaling _atomScaling;
    bool _labelsOn;
    ColorMap *_colorMap;
    ColorMode _colorMode;
    std::string _colorFieldName;
    DataSet::DataAttributeType _colorFieldType;
    double _colorFieldRange[2];
    double _vectorMagnitudeRange[2];
    double _vectorComponentRange[3][2];
    Renderer *_renderer;

    vtkSmartPointer<vtkLookupTable> _lut;
    vtkSmartPointer<vtkActor> _atomProp;
    vtkSmartPointer<vtkActor> _bondProp;
    vtkSmartPointer<vtkActor2D> _labelProp;
#ifndef MOLECULE_USE_GLYPH3D_MAPPER
    vtkSmartPointer<vtkGlyph3D> _glypher;
    vtkSmartPointer<vtkTubeFilter> _tuber;
    vtkSmartPointer<vtkPolyDataMapper> _atomMapper;
    vtkSmartPointer<vtkPolyDataMapper> _bondMapper;
#else
    vtkSmartPointer<vtkPolyData> _bondPD;
    vtkSmartPointer<vtkCylinderSource> _cylinderSource;
    vtkSmartPointer<vtkTransformPolyDataFilter>_cylinderTrans;
    vtkSmartPointer<vtkLineSource> _lineSource;
    vtkSmartPointer<vtkGlyph3DMapper> _atomMapper;
    vtkSmartPointer<vtkGlyph3DMapper> _bondMapper;
#endif
    vtkSmartPointer<vtkLabelPlacementMapper> _labelMapper;
};

}
}

#endif
