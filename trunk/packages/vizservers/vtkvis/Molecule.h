/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_MOLECULE_H
#define VTKVIS_MOLECULE_H

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAssembly.h>
#include <vtkGlyph3DMapper.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkLineSource.h>
#include <vtkPointSetToLabelHierarchy.h>
#include <vtkLabelPlacementMapper.h>
#include <vtkTransform.h>

#include "ColorMap.h"
#include "GraphicsObject.h"

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
class Molecule : public GraphicsObject {
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

    virtual void setTransform(vtkMatrix4x4 *matrix)
    {
        GraphicsObject::setTransform(matrix);
        updateLabelTransform();
    }

    virtual void setOrigin(double origin[3])
    {
        GraphicsObject::setOrigin(origin);
        updateLabelTransform();
    }

    virtual void setOrientation(double quat[4])
    {
        GraphicsObject::setOrientation(quat);
        updateLabelTransform();
    }

    virtual void setOrientation(double angle, double axis[3])
    {
        GraphicsObject::setOrientation(angle, axis);
        updateLabelTransform();
    }

    virtual void setPosition(double pos[3])
    {
        GraphicsObject::setPosition(pos);
        updateLabelTransform();
    }

    virtual void setAspect(double aspect)
    {
        GraphicsObject::setAspect(aspect);
        updateLabelTransform();
    }

    virtual void setScale(double scale[3])
    {
        GraphicsObject::setScale(scale);
        updateLabelTransform();
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

    void setAtomLabelField(const char *fieldName);

    void setBondVisibility(bool state);

    void setBondStyle(BondStyle style);

    void setBondColor(float color[3]);

    void setBondColorMode(BondColorMode mode);

    void setAtomQuality(double quality);

    void setBondQuality(double quality);

    static ColorMap *createElementColorMap();

private:
    virtual void initProp();
    virtual void update();

    void setupBondPolyData();

    void updateLabelTransform();

    static void addLabelArray(vtkDataSet *dataSet);

    static void addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling, double scaleFactor);

    static int computeBonds(vtkDataSet *dataSet, float tolerance = 0.45f);

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
    vtkSmartPointer<vtkSphereSource> _sphereSource;
    vtkSmartPointer<vtkPolyData> _bondPD;
    vtkSmartPointer<vtkCylinderSource> _cylinderSource;
    vtkSmartPointer<vtkTransformPolyDataFilter>_cylinderTrans;
    vtkSmartPointer<vtkLineSource> _lineSource;
    vtkSmartPointer<vtkGlyph3DMapper> _atomMapper;
    vtkSmartPointer<vtkGlyph3DMapper> _bondMapper;
    vtkSmartPointer<vtkPointSetToLabelHierarchy> _labelHierarchy;
    vtkSmartPointer<vtkLabelPlacementMapper> _labelMapper;
    vtkSmartPointer<vtkTransform> _labelTransform;
};

}

#endif
