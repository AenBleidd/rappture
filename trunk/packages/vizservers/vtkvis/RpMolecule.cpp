/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTubeFilter.h>
#include <vtkSphereSource.h>
#include <vtkGlyph3D.h>

#include "RpMolecule.h"
#include "RpMoleculeData.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Molecule::Molecule() :
    VtkGraphicsObject(),
    _atomScaling(NO_ATOM_SCALING),
    _colorMap(NULL)
{
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
        _bondProp->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _bondProp->GetProperty()->SetLineWidth(_edgeWidth);
        _bondProp->GetProperty()->SetOpacity(_opacity);
        _bondProp->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _bondProp->GetProperty()->LightingOff();
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

    if (_lut == NULL) {
        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL ||
            strcmp(ds->GetPointData()->GetScalars()->GetName(), "element") != 0) {
            WARN("No element array in dataset %s", _dataSet->getName().c_str());
            setColorMap(ColorMap::getDefault());
        } else {
            TRACE("Using element default colormap");
            setColorMap(ColorMap::getElementDefault());
        }
    }

    initProp();

    addRadiusArray(ds, _atomScaling);

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
            vtkSmartPointer<vtkTubeFilter> tuber = vtkSmartPointer<vtkTubeFilter>::New();
            tuber->SetInput(pd);
            tuber->SetNumberOfSides(6);
            tuber->CappingOff();
            tuber->SetRadius(.03);
            tuber->SetVaryRadiusToVaryRadiusOff();
            _bondMapper->SetInputConnection(tuber->GetOutputPort());
            _bondProp->SetMapper(_bondMapper);
            getAssembly()->AddPart(_bondProp);
        }
        if (pd->GetNumberOfVerts() > 0) {
            // Atoms
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetRadius(.08);
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

    _atomMapper->Update();
    _bondMapper->Update();
}

void Molecule::updateRanges(Renderer *renderer)
{
    VtkGraphicsObject::updateRanges(renderer);

    if (_lut != NULL) {
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
        addRadiusArray(ds, _atomScaling);
        if (_glypher != NULL) {
            if (_atomScaling != NO_ATOM_SCALING &&
                ds->GetPointData() != NULL &&
                ds->GetPointData()->GetVectors() != NULL) {
                _glypher->SetScaleModeToScaleByVector();
                _glypher->ScalingOn();
            } else {
                _glypher->SetScaleModeToDataScalingOff();
                _glypher->ScalingOff();
            }
        }
    }
}

/**
 * \brief Add a scalar array to dataSet with sizes for the elements
 * specified in the "element" scalar array
 */
void Molecule::addRadiusArray(vtkDataSet *dataSet, AtomScaling scaling)
{
    if (dataSet->GetPointData() == NULL ||
        dataSet->GetPointData()->GetScalars() == NULL) {
        return;
    }
    vtkDataArray *elements = dataSet->GetPointData()->GetScalars();
    if (strcmp(elements->GetName(), "element") != 0) {
        return;
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
        return;
    }
    vtkSmartPointer<vtkFloatArray> radii = vtkSmartPointer<vtkFloatArray>::New();
    radii->SetName("radius");
    radii->SetNumberOfComponents(3);
    for (int i = 0; i < elements->GetNumberOfTuples(); i++) {
        int elt = (int)elements->GetComponent(i, 0);
        float tuple[3];
        tuple[0] = radiusSource[elt];
        tuple[1] = 0;
        tuple[2] = 0;
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
