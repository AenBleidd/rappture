/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cfloat>
#include <cassert>

#include <vtkDataSet.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkStringArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPointSetToLabelHierarchy.h>
#include <vtkLabelPlacementMapper.h>
#include <vtkTextProperty.h>

#include "RpMolecule.h"
#include "RpMoleculeData.h"
#include "RpVtkRenderer.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Molecule::Molecule() :
    VtkGraphicsObject(),
    _radiusScale(0.3),
    _atomScaling(COVALENT_RADIUS),
    _labelsOn(false),
    _colorMap(NULL),
    _colorMode(COLOR_BY_ELEMENTS),
    _colorFieldType(DataSet::POINT_DATA)
{
    _bondColor[0] = _bondColor[1] = _bondColor[2] = 1.0f;
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
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

void Molecule::setDataSet(DataSet *dataSet,
                          Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        _renderer = renderer;

        if (renderer->getUseCumulativeRange()) {
            renderer->getCumulativeDataRange(_dataRange,
                                             _dataSet->getActiveScalarsName(),
                                             1);
            renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                             _dataSet->getActiveVectorsName(),
                                             3);
            for (int i = 0; i < 3; i++) {
                renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                 _dataSet->getActiveVectorsName(),
                                                 3, i);
            }
        } else {
            _dataSet->getScalarRange(_dataRange);
            _dataSet->getVectorRange(_vectorMagnitudeRange);
            for (int i = 0; i < 3; i++) {
                _dataSet->getVectorRange(_vectorComponentRange[i], i);
            }
        }

        update();
    }
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
        _atomMapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
        _atomMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _atomMapper->ScalarVisibilityOn();
    }
    if (_bondMapper == NULL) {
        _bondMapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
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
            setupBondPolyData();

            if (_cylinderSource == NULL) {
                _cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
                _cylinderSource->SetRadius(0.075);
                _cylinderSource->SetHeight(1.0);
                _cylinderSource->SetResolution(12);
                _cylinderSource->CappingOff();
                _cylinderTrans = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
                _cylinderTrans->SetInputConnection(_cylinderSource->GetOutputPort());
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateZ(-90.0);
                _cylinderTrans->SetTransform(trans);
            }
            if (_lineSource == NULL) {
                _lineSource = vtkSmartPointer<vtkLineSource>::New();
                _lineSource->SetPoint1(-0.5, 0, 0);
                _lineSource->SetPoint2(0.5, 0, 0);
            }

            _bondMapper->SetSourceConnection(_cylinderTrans->GetOutputPort());
#ifdef USE_VTK6
            _bondMapper->SetInputData(_bondPD);
#else
            _bondMapper->SetInputConnection(_bondPD->GetProducerPort());
#endif
            _bondMapper->SetOrientationArray("bond_orientations");
            _bondMapper->SetOrientationModeToDirection();
            _bondMapper->OrientOn();
            _bondMapper->SetScaleArray("bond_scales");
            _bondMapper->SetScaleModeToScaleByVectorComponents();
            _bondMapper->ScalingOn();
            _bondMapper->ClampingOff();

            _bondProp->SetMapper(_bondMapper);
            getAssembly()->AddPart(_bondProp);
        }
        if (pd->GetNumberOfVerts() > 0) {
            vtkSmartPointer<vtkPointSetToLabelHierarchy> hier = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
#ifdef USE_VTK6
            hier->SetInputData(pd);
#else
            hier->SetInput(pd);
#endif
            hier->SetLabelArrayName("labels");
            hier->GetTextProperty()->SetColor(0, 0, 0);
            _labelMapper->SetInputConnection(hier->GetOutputPort());
            _labelProp->SetMapper(_labelMapper);

            // Atoms
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetRadius(1.0);
            sphereSource->SetThetaResolution(14);
            sphereSource->SetPhiResolution(14);

            _atomMapper->SetSourceConnection(sphereSource->GetOutputPort());
#ifdef USE_VTK6
            _atomMapper->SetInputData(pd);
#else
            _atomMapper->SetInputConnection(pd->GetProducerPort());
#endif
            if (ds->GetPointData() != NULL &&
                ds->GetPointData()->GetVectors() != NULL) {
                _atomMapper->SetScaleArray(vtkDataSetAttributes::VECTORS);
                _atomMapper->SetScaleModeToScaleByMagnitude();
                _atomMapper->ScalingOn();
            } else {
                _atomMapper->SetScaleModeToNoDataScaling();
                _atomMapper->ScalingOff();
            }
            _atomMapper->OrientOff();

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
    if (_dataSet == NULL) {
        ERROR("called before setDataSet");
        return;
    }

    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
        renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                         _dataSet->getActiveVectorsName(),
                                         3);
        for (int i = 0; i < 3; i++) {
            renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                             _dataSet->getActiveVectorsName(),
                                             3, i);
        }
    } else {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges
    double *rangePtr = _colorFieldRange;
    if (_colorFieldRange[0] > _colorFieldRange[1]) {
        rangePtr = NULL;
    }
    setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);
}

void Molecule::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL)
        return;

    switch (mode) {
    case COLOR_BY_ELEMENTS: // Assume default scalar is "element" array
        setColorMode(mode,
                     DataSet::POINT_DATA,
                     "element");
        break;
    case COLOR_BY_SCALAR:
        setColorMode(mode,
                     _dataSet->getActiveScalarsType(),
                     _dataSet->getActiveScalarsName(),
                     _dataRange);
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorMagnitudeRange);
        break;
    case COLOR_BY_VECTOR_X:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[0]);
        break;
    case COLOR_BY_VECTOR_Y:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[1]);
        break;
    case COLOR_BY_VECTOR_Z:
        setColorMode(mode,
                     _dataSet->getActiveVectorsType(),
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[2]);
        break;
    case COLOR_CONSTANT:
    default:
        setColorMode(mode, DataSet::POINT_DATA, NULL, NULL);
        break;
    }
}

void Molecule::setColorMode(ColorMode mode,
                            const char *name, double range[2])
{
    if (_dataSet == NULL)
        return;
    DataSet::DataAttributeType type = DataSet::POINT_DATA;
    int numComponents = 1;
    if (name != NULL && strlen(name) > 0 &&
        !_dataSet->getFieldInfo(name, &type, &numComponents)) {
        ERROR("Field not found: %s", name);
        return;
    }
    setColorMode(mode, type, name, range);
}

void Molecule::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
                            const char *name, double range[2])
{
    _colorMode = mode;
    _colorFieldType = type;
    if (name == NULL)
        _colorFieldName.clear();
    else
        _colorFieldName = name;
    if (range == NULL) {
        _colorFieldRange[0] = DBL_MAX;
        _colorFieldRange[1] = -DBL_MAX;
    } else {
        memcpy(_colorFieldRange, range, sizeof(double)*2);
    }

    if (_dataSet == NULL || (_atomMapper == NULL && _bondMapper == NULL))
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        if (_atomMapper != NULL)
            _atomMapper->SetScalarModeToUsePointFieldData();
        if (_bondMapper != NULL)
            _bondMapper->SetScalarModeToUsePointFieldData();
        break;
    case DataSet::CELL_DATA:
        if (_atomMapper != NULL)
            _atomMapper->SetScalarModeToUseCellFieldData();
        if (_bondMapper != NULL)
            _bondMapper->SetScalarModeToUseCellFieldData();
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (name != NULL && strlen(name) > 0) {
        if (_atomMapper != NULL)
            _atomMapper->SelectColorArray(name);
        if (_bondMapper != NULL)
            _bondMapper->SelectColorArray(name);
    } else {
        if (_atomMapper != NULL)
            _atomMapper->SetScalarModeToDefault();
        if (_bondMapper != NULL)
            _bondMapper->SetScalarModeToDefault();
    }

    if (_lut != NULL) {
        if (range != NULL) {
            _lut->SetRange(range);
        } else if (mode == COLOR_BY_ELEMENTS) {
            double range[2];
            range[0] = 0;
            range[1] = NUM_ELEMENTS;
            _lut->SetRange(range);
        } else if (name != NULL && strlen(name) > 0) {
            double r[2];
            int comp = -1;
            if (mode == COLOR_BY_VECTOR_X)
                comp = 0;
            else if (mode == COLOR_BY_VECTOR_Y)
                comp = 1;
            else if (mode == COLOR_BY_VECTOR_Z)
                comp = 2;

            if (_renderer->getUseCumulativeRange()) {
                int numComponents;
                if  (!_dataSet->getFieldInfo(name, type, &numComponents)) {
                    ERROR("Field not found: %s, type: %d", name, type);
                    return;
                } else if (numComponents < comp+1) {
                    ERROR("Request for component %d in field with %d components",
                          comp, numComponents);
                    return;
                }
                _renderer->getCumulativeDataRange(r, name, type, numComponents, comp);
            } else {
                _dataSet->getDataRange(r, name, type, comp);
            }
            _lut->SetRange(r);
        }
    }

    switch (mode) {
    case COLOR_BY_ELEMENTS:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_SCALAR:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOn();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        if (_atomMapper != NULL)
            _atomMapper->ScalarVisibilityOff();
        if (_bondMapper != NULL)
            _bondMapper->ScalarVisibilityOff();
        break;
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
        _lut->DeepCopy(cmap->getLookupTable());
        switch (_colorMode) {
        case COLOR_BY_ELEMENTS: {
            double range[2];
            range[0] = 0;
            range[1] = NUM_ELEMENTS;
            _lut->SetRange(range);
        }
            break;
        case COLOR_CONSTANT:
        case COLOR_BY_SCALAR:
            _lut->SetRange(_dataRange);
            break;
        case COLOR_BY_VECTOR_MAGNITUDE:
            _lut->SetRange(_vectorMagnitudeRange);
            break;
        case COLOR_BY_VECTOR_X:
            _lut->SetRange(_vectorComponentRange[0]);
            break;
        case COLOR_BY_VECTOR_Y:
            _lut->SetRange(_vectorComponentRange[1]);
            break;
        case COLOR_BY_VECTOR_Z:
            _lut->SetRange(_vectorComponentRange[2]);
            break;
        default:
            break;
        }
    } else {
        double range[2];
        _lut->GetTableRange(range);
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(range);
        _lut->Modified();
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

void Molecule::setBondStyle(BondStyle style)
{
    switch (style) {
    case BOND_STYLE_CYLINDER:
        if (_bondProp != NULL) {
            _bondProp->GetProperty()->SetLineWidth(_edgeWidth);
            _bondProp->GetProperty()->SetLighting(_lighting ? 1 : 0);
        }
        if (_bondMapper != NULL && _cylinderTrans != NULL) {
            _cylinderTrans->Modified();
            _bondMapper->SetSourceConnection(_cylinderTrans->GetOutputPort());
            _bondMapper->Modified();
        }
        break;
    case BOND_STYLE_LINE:
        if (_bondProp != NULL) {
            _bondProp->GetProperty()->LightingOff();
        }
        if (_bondMapper != NULL && _lineSource != NULL) {
            _lineSource->Modified();
            _bondMapper->SetSourceConnection(_lineSource->GetOutputPort());
            _bondMapper->Modified();
        }
         break;
    default:
        WARN("Unknown bond style");    
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
        if (_atomMapper != NULL) {
             assert(ds->GetPointData() != NULL &&
                    ds->GetPointData()->GetVectors() != NULL);
            _atomMapper->SetScaleModeToScaleByMagnitude();
            _atomMapper->SetScaleArray(vtkDataSetAttributes::VECTORS);
            _atomMapper->ScalingOn();
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
    if (_cylinderSource != NULL) {
        _cylinderSource->SetRadius(scale);
        // Workaround bug with source modification not causing
        // mapper to be updated
        if (_bondMapper != NULL) {
            _bondMapper->Modified();
        }
    }
}

void Molecule::setupBondPolyData()
{
    if (_dataSet == NULL)
        return;
    if (_bondPD == NULL) {
        _bondPD = vtkSmartPointer<vtkPolyData>::New();
    } else {
        _bondPD->Initialize();
    }
    vtkPolyData *pd = vtkPolyData::SafeDownCast(_dataSet->getVtkDataSet());
    if (pd == NULL)
        return;
    vtkCellArray *lines = pd->GetLines();
    lines->InitTraversal();
    vtkIdType npts, *pts;
    vtkSmartPointer<vtkPoints> bondPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkIntArray> bondElements = vtkSmartPointer<vtkIntArray>::New();
    vtkSmartPointer<vtkDoubleArray> bondVectors = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> bondScales = vtkSmartPointer<vtkDoubleArray>::New();
    bondElements->SetName("element");
    bondVectors->SetName("bond_orientations");
    bondVectors->SetNumberOfComponents(3);
    bondScales->SetName("bond_scales");
    bondScales->SetNumberOfComponents(3);
    vtkDataArray *elements = NULL;
    if (pd->GetPointData() != NULL &&
        pd->GetPointData()->GetScalars() != NULL &&
        strcmp(pd->GetPointData()->GetScalars()->GetName(), "element") == 0) {
        elements = pd->GetPointData()->GetScalars();
    }
    for (int i = 0; lines->GetNextCell(npts, pts); i++) {
        assert(npts == 2);
        double pt0[3], pt1[3];
        double newPt0[3], newPt1[3];
        //double *pt0 = pd->GetPoint(pts[0]);
        //double *pt1 = pd->GetPoint(pts[1]);
        pd->GetPoint(pts[0], pt0);
        pd->GetPoint(pts[1], pt1);
        double center[3];

        for (int j = 0; j < 3; j++)
            center[j] = pt0[j] + (pt1[j] - pt0[j]) * 0.5;
        for (int j = 0; j < 3; j++)
            newPt0[j] = pt0[j] + (pt1[j] - pt0[j]) * 0.25;
        for (int j = 0; j < 3; j++)
            newPt1[j] = pt0[j] + (pt1[j] - pt0[j]) * 0.75;

        bondPoints->InsertNextPoint(newPt0);
        bondPoints->InsertNextPoint(newPt1);

        TRACE("Bond %d: (%g,%g,%g)-(%g,%g,%g)-(%g,%g,%g)", i,
              pt0[0], pt0[1], pt0[2], 
              center[0], center[1], center[2],
              pt1[0], pt1[1], pt1[2]);

        double vec[3];
        for (int j = 0; j < 3; j++)
            vec[j] = center[j] - pt0[j];

        bondVectors->InsertNextTupleValue(vec);
        bondVectors->InsertNextTupleValue(vec);
        TRACE("Bond %d, vec: %g,%g,%g", i, vec[0], vec[1], vec[2]);

        double scale[3];
        scale[0] = sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
        scale[1] = 1.0;
        scale[2] = 1.0;

        bondScales->InsertNextTupleValue(scale);
        bondScales->InsertNextTupleValue(scale);

        if (elements != NULL) {
            int element = (int)elements->GetComponent(pts[0], 0);
            TRACE("Bond %d, elt 0: %d", i, element);
            bondElements->InsertNextValue(element);
            element = (int)elements->GetComponent(pts[1], 0);
            TRACE("Bond %d, elt 1: %d", i, element);
            bondElements->InsertNextValue(element);
        }
    }
    _bondPD->SetPoints(bondPoints);
    if (elements != NULL)
        _bondPD->GetPointData()->SetScalars(bondElements);
    _bondPD->GetPointData()->AddArray(bondVectors);
    _bondPD->GetPointData()->AddArray(bondScales);
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
        cp[i].value = i/((double)NUM_ELEMENTS);
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
    range[1] = NUM_ELEMENTS;
    elementCmap->getLookupTable()->SetRange(range);

    return elementCmap;
}
