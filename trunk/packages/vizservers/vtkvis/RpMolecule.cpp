/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkStringArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTubeFilter.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>
#include <vtkPointSetToLabelHierarchy.h>
#include <vtkLabelPlacementMapper.h>
#include <vtkTextProperty.h>

#include "RpMolecule.h"
#include "RpMoleculeData.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Molecule::Molecule() :
    VtkGraphicsObject(),
    _radiusScale(0.3),
    _atomScaling(VAN_DER_WAALS_RADIUS),
    _colorMap(NULL),
    _labelsOn(false)
{
    _bondColor[0] = _bondColor[1] = _bondColor[2] = 1.0f;
    _faceCulling = true;
}

Molecule::~Molecule()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Molecule for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Molecule with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize VTK Props to render a Molecule
 */
void Molecule::initProp()
{
    if (_atomProp == NULL) {
        _atomProp = vtkSmartPointer<vtkActor>::New();
        if (_faceCulling && _opacity == 1.0)
            setCulling(_atomProp->GetProperty(), true);
        _atomProp->GetProperty()->EdgeVisibilityOff();
        _atomProp->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _atomProp->GetProperty()->SetLineWidth(_edgeWidth);
        _atomProp->GetProperty()->SetOpacity(_opacity);
        _atomProp->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _atomProp->GetProperty()->LightingOff();
    }
    if (_bondProp == NULL) {
        _bondProp = vtkSmartPointer<vtkActor>::New();
        if (_faceCulling && _opacity == 1.0)
            setCulling(_bondProp->GetProperty(), true);
        _bondProp->GetProperty()->EdgeVisibilityOff();
        _bondProp->GetProperty()->SetColor(_bondColor[0], _bondColor[1], _bondColor[2]);
        _bondProp->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _bondProp->GetProperty()->SetLineWidth(_edgeWidth);
        _bondProp->GetProperty()->SetOpacity(_opacity);
        _bondProp->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _bondProp->GetProperty()->LightingOff();
    }
    if (_labelProp == NULL) {
        _labelProp = vtkSmartPointer<vtkActor2D>::New();
    }
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();
    }
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void Molecule::update()
{
    if (_dataSet == NULL) {
        return;
    }
    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_atomMapper == NULL) {
        _atomMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _atomMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _atomMapper->ScalarVisibilityOn();
    }
    if (_bondMapper == NULL) {
        _bondMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _bondMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _bondMapper->ScalarVisibilityOn();
    }
    if (_labelMapper == NULL) {
        _labelMapper = vtkSmartPointer<vtkLabelPlacementMapper>::New();
        _labelMapper->SetShapeToRoundedRect();
        _labelMapper->SetBackgroundColor(1.0, 1.0, 0.7);
        _labelMapper->SetBackgroundOpacity(0.8);
        _labelMapper->SetMargin(3);
    }

    if (_lut == NULL) {
        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL ||
            strcmp(ds->GetPointData()->GetScalars()->GetName(), "element") != 0) {
            WARN("No element array in dataset %s", _dataSet->getName().c_str());
            setColorMap(ColorMap::getDefault());
            if (_atomScaling != NO_ATOM_SCALING)
                _atomScaling = NO_ATOM_SCALING;
        } else {
            TRACE("Using element default colormap");
            setColorMap(ColorMap::getElementDefault());
        }
    }

    initProp();

    addLabelArray(ds);
    addRadiusArray(ds, _atomScaling, _radiusScale);

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        TRACE("Verts: %d Lines: %d Polys: %d Strips: %d",
                  pd->GetNumberOfVerts(),
                  pd->GetNumberOfLines(),
                  pd->GetNumberOfPolys(),
                  pd->GetNumberOfStrips());
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() > 0) {
            // Bonds
            if (_tuber == NULL)
                _tuber = vtkSmartPointer<vtkTubeFilter>::New();
            _tuber->SetInput(pd);
            _tuber->SetNumberOfSides(12);
            _tuber->CappingOff();
            _tuber->SetRadius(0.075);
            _tuber->SetVaryRadiusToVaryRadiusOff();
            _bondMapper->SetInputConnection(_tuber->GetOutputPort());
            _bondProp->SetMapper(_bondMapper);
            getAssembly()->AddPart(_bondProp);
        }
        if (pd->GetNumberOfVerts() > 0) {
            vtkSmartPointer<vtkPointSetToLabelHierarchy> hier = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
            hier->SetInput(pd);
            hier->SetLabelArrayName("labels");
            hier->GetTextProperty()->SetColor(0, 0, 0);
            _labelMapper->SetInputConnection(hier->GetOutputPort());
            _labelProp->SetMapper(_labelMapper);

            // Atoms
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetRadius(1.0);
            sphereSource->SetThetaResolution(14);
            sphereSource->SetPhiResolution(14);
            if (_glypher == NULL)
                _glypher = vtkSmartPointer<vtkGlyph3D>::New();
            _glypher->SetSourceConnection(sphereSource->GetOutputPort());
            _glypher->SetInput(pd);
            if (_atomScaling != NO_ATOM_SCALING &&
                ds->GetPointData() != NULL &&
                ds->GetPointData()->GetVectors() != NULL) {
                _glypher->SetScaleModeToScaleByVector();
                _glypher->ScalingOn();
            } else {
                _glypher->SetScaleModeToDataScalingOff();
                _glypher->ScalingOff();
            }
            _glypher->SetColorModeToColorByScalar();
            _glypher->OrientOff();
            _atomMapper->SetInputConnection(_glypher->GetOutputPort());
            _atomProp->SetMapper(_atomMapper);
            getAssembly()->AddPart(_atomProp);
        }
    } else {
        // DataSet is NOT a vtkPolyData
        ERROR("DataSet is not a PolyData");
        return;
    }

    setAtomLabelVisibility(_labelsOn);

    _atomMapper->Update();
    _bondMapper->Update();
    _labelMapper->Update();
}

void Molecule::updateRanges(Renderer *renderer)
{
    VtkGraphicsObject::updateRanges(renderer);

    if (_lut != NULL && _dataSet != NULL) {
        vtkDataSet *ds = _dataSet->getVtkDataSet();
        if (ds == NULL)
            return;
        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL ||
            strcmp(ds->GetPointData()->GetScalars()->GetName(), "element") != 0) {
            _lut->SetRange(_dataRange);
        }
    }
}

/**
 * \brief Called when the color map has been edited
 */
void Molecule::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Molecule::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_atomMapper != NULL) {
            _atomMapper->UseLookupTableScalarRangeOn();
            _atomMapper->SetLookupTable(_lut);
        }
        if (_bondMapper != NULL) {
            _bondMapper->UseLookupTableScalarRangeOn();
            _bondMapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());
    _lut->Modified();

    // Element color maps need to retain their range
    // Only set LUT range if we are not coloring by element
    vtkDataSet *ds = _dataSet->getVtkDataSet();
    if (ds == NULL)
        return;
    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL ||
        strcmp(ds->GetPointData()->GetScalars()->GetName(), "element") != 0) {
        _lut->SetRange(_dataRange);
    }
}

/**
 * \brief Turn on/off rendering of atom labels
 */
void Molecule::setAtomLabelVisibility(bool state)
{
    _labelsOn = state;
    if (_labelProp != NULL) {
        _labelProp->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Turn on/off rendering of the atoms
 */
void Molecule::setAtomVisibility(bool state)
{
    if (_atomProp != NULL) {
        _atomProp->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Turn on/off rendering of the bonds
 */
void Molecule::setBondVisibility(bool state)
{
    if (_bondProp != NULL) {
        _bondProp->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Toggle visibility of the prop
 */
void Molecule::setVisibility(bool state)
{
    VtkGraphicsObject::setVisibility(state);
    if (_labelProp != NULL) {
        if (!state)
            _labelProp->SetVisibility(0);
        else
            setAtomLabelVisibility(_labelsOn);
    }
}

/**
 * \brief Set opacity of molecule
 */
void Molecule::setOpacity(double opacity)
{
    VtkGraphicsObject::setOpacity(opacity);
    if (_labelMapper != NULL) {
        _labelMapper->SetBackgroundOpacity(opacity);
    }
}

/**
 * \brief Set constant bond color
 */
void Molecule::setBondColor(float color[3])
{
    _bondColor[0] = color[0];
    _bondColor[1] = color[1];
    _bondColor[2] = color[2];
    if (_bondProp != NULL) {
        _bondProp->GetProperty()->SetColor(_bondColor[0], _bondColor[1], _bondColor[2]);
    }
}

/**
 * \brief Set mode to determine how bonds are colored
 */
void Molecule::setBondColorMode(BondColorMode mode)
{
    if (_bondMapper == NULL) return;

    switch (mode) {
    case BOND_COLOR_BY_ELEMENTS:
        _bondMapper->ScalarVisibilityOn();
        break;
    case BOND_COLOR_CONSTANT:
        _bondMapper->ScalarVisibilityOff();
        break;
    default:
        WARN("Unknown bond color mode");
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Molecule::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_atomMapper != NULL) {
        _atomMapper->SetClippingPlanes(planes);
    }
    if (_bondMapper != NULL) {
        _bondMapper->SetClippingPlanes(planes);
    }
}

/**
 * \brief Set the radius type used for scaling atoms
 */
void Molecule::setAtomScaling(AtomScaling state)
{
    _atomScaling = state;
    if (_dataSet != NULL) {
        vtkDataSet *ds = _dataSet->getVtkDataSet();
        addRadiusArray(ds, _atomScaling, _radiusScale);
        if (_glypher != NULL) {
            assert(ds->GetPointData() != NULL &&
                   ds->GetPointData()->GetVectors() != NULL);
            _glypher->SetScaleModeToScaleByVector();
            _glypher->ScalingOn();
        }
    }
}

/**
 * \brief Set the constant radius scaling factor for atoms.  This
 * can be used to convert from Angstroms to atom coordinates units.
 */
void Molecule::setAtomRadiusScale(double scale)
{
    _radiusScale = scale;
    if (_dataSet != NULL) {
        vtkDataSet *ds = _dataSet->getVtkDataSet();
        addRadiusArray(ds, _atomScaling, _radiusScale);
    }
}

/**
 * \brief Set the constant radius scaling factor for bonds.
 */
void Molecule::setBondRadiusScale(double scale)
{
    if (_tuber != NULL)
        _tuber->SetRadius(scale);
}

void Molecule::addLabelArray(vtkDataSet *dataSet)
{
    vtkDataArray *elements = NULL;
    if (dataSet->GetPointData() != NULL &&
        dataSet->GetPointData()->GetScalars() != NULL &&
        strcmp(dataSet->GetPointData()->GetScalars()->GetName(), "element") == 0) {
        elements = dataSet->GetPointData()->GetScalars();
    } else {
        WARN("Can't label atoms without an element array");
    }

    vtkSmartPointer<vtkStringArray> labelArray = vtkSmartPointer<vtkStringArray>::New();
    labelArray->SetName("labels");
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dataSet);
    if (pd == NULL) {
        ERROR("DataSet not a PolyData");
        return;
    }
    for (int i = 0; i < pd->GetNumberOfVerts(); i++) {
        char buf[32];
        if (elements != NULL) {
            int elt = (int)elements->GetComponent(i, 0);
            sprintf(buf, "%s%d", g_elementNames[elt], i);
        } else {
            sprintf(buf, "%d", i);
        }
        labelArray->InsertNextValue(buf);
    }
    dataSet->GetPointData()->AddArray(labelArray);
}

/**
 * \brief Add a scalar array to dataSet with sizes for the elements
 * specified in the "element" scalar array
 */
void Molecule::addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling, double scaleFactor)
{
    vtkDataArray *elements = NULL;
    if (dataSet->GetPointData() != NULL &&
        dataSet->GetPointData()->GetScalars() != NULL &&
        strcmp(dataSet->GetPointData()->GetScalars()->GetName(), "element") == 0) {
        elements = dataSet->GetPointData()->GetScalars();
    } else if (scaling != NO_ATOM_SCALING) {
        WARN("Can't use non-constant scaling without an element array");
    }
    const float *radiusSource = NULL;
    switch (scaling) {
    case VAN_DER_WAALS_RADIUS:
        radiusSource = g_vdwRadii;
        break;
    case COVALENT_RADIUS:
        radiusSource = g_covalentRadii;
        break;
    case ATOMIC_RADIUS:
        radiusSource = g_atomicRadii;
        break;
    case NO_ATOM_SCALING:
    default:
        ;
    }
    vtkSmartPointer<vtkFloatArray> radii = vtkSmartPointer<vtkFloatArray>::New();
    radii->SetName("radius");
    radii->SetNumberOfComponents(3);
    vtkPolyData *pd = vtkPolyData::SafeDownCast(dataSet);
    if (pd == NULL) {
        ERROR("DataSet not a PolyData");
        return;
    }
    for (int i = 0; i < pd->GetNumberOfVerts(); i++) {
        float tuple[3];
        tuple[1] = tuple[2] = 0;
        if (elements != NULL && radiusSource != NULL) {
            int elt = (int)elements->GetComponent(i, 0);
            tuple[0] = radiusSource[elt] * scaleFactor;
        } else {
            tuple[0] = scaleFactor;
        }
        radii->InsertNextTupleValue(tuple);
    }
    dataSet->GetPointData()->SetVectors(radii);
}

/**
 * \brief Create a color map to map atomic numbers to element colors
 */
ColorMap *Molecule::createElementColorMap()
{
    ColorMap *elementCmap = new ColorMap("elementDefault");
    ColorMap::ControlPoint cp[NUM_ELEMENTS+1];

    elementCmap->setNumberOfTableEntries(NUM_ELEMENTS+1);
    for (int i = 0; i <= NUM_ELEMENTS; i++) {
        cp[i].value = i/((double)(NUM_ELEMENTS+1));
        for (int c = 0; c < 3; c++) {
            cp[i].color[c] = ((double)g_elementColors[i][c])/255.;
        }
        elementCmap->addControlPoint(cp[i]);
    }
    ColorMap::OpacityControlPoint ocp[2];
    ocp[0].value = 0;
    ocp[0].alpha = 1.0;
    ocp[1].value = 1.0;
    ocp[1].alpha = 1.0;
    elementCmap->addOpacityControlPoint(ocp[0]);
    elementCmap->addOpacityControlPoint(ocp[1]);
    elementCmap->build();
    double range[2];
    range[0] = 0;
    range[1] = NUM_ELEMENTS+1;
    elementCmap->getLookupTable()->SetRange(range);

    return elementCmap;
}
