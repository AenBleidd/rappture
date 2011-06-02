/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkProp.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkGlyph3D.h>
#include <vtkArrowSource.h>
#include <vtkConeSource.h>
#include <vtkCylinderSource.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>

#include "RpGlyphs.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Glyphs::Glyphs() :
    _dataSet(NULL),
    _opacity(1.0),
    _lighting(true),
    _glyphShape(ARROW),
    _scaleFactor(1.0)
{
}

Glyphs::~Glyphs()
{
}

/**
 * \brief Get the VTK Prop for the Glyphs
 */
vtkProp *Glyphs::getProp()
{
    return _prop;
}

/**
 * \brief Create and initialize a VTK Prop to render Glyphs
 */
void Glyphs::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        _prop->GetProperty()->EdgeVisibilityOff();
        _prop->GetProperty()->SetOpacity(_opacity);
        if (!_lighting)
            _prop->GetProperty()->LightingOff();
    }
}

/**
 * \brief Specify input DataSet
 *
 * The DataSet must be a PolyData point set
 * with vectors and/or scalars
 */
void Glyphs::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Returns the DataSet this Glyphs renders
 */
DataSet *Glyphs::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Set the shape of the glyphs
 */
void Glyphs::setGlyphShape(GlyphShape shape)
{
    _glyphShape = shape;
    switch (_glyphShape) {
    case ARROW:
	_glyphSource = vtkSmartPointer<vtkArrowSource>::New();
	break;
    case CONE:
	_glyphSource = vtkSmartPointer<vtkConeSource>::New();
	break;
    case CUBE:
	_glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
	vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToCube();
	break;
    case CYLINDER:
	_glyphSource = vtkSmartPointer<vtkCylinderSource>::New();
	break;
    case DODECAHEDRON:
	_glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
	vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToDodecahedron();
	break;
    case ICOSAHEDRON:
	_glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
	vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToIcosahedron();
	break;
    case OCTAHEDRON:
	_glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
	vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToOctahedron();
	break;
    case SPHERE:
	_glyphSource = vtkSmartPointer<vtkSphereSource>::New();
	break;
    case TETRAHEDRON:
	_glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
	vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToTetrahedron();
	break;
    default:
	ERROR("Unknown glyph shape: %d", _glyphShape);
	return;
    }
    if (_glyphGenerator != NULL) {
	_glyphGenerator->SetSource(_glyphSource->GetOutput());
    }
}

void Glyphs::update()
{
    if (_dataSet == NULL) {
        return;
    }

    vtkDataSet *ds = _dataSet->getVtkDataSet();
    double dataRange[2];
    _dataSet->getDataRange(dataRange);

    if (_glyphGenerator == NULL) {
	_glyphGenerator = vtkSmartPointer<vtkGlyph3D>::New();
    }

    setGlyphShape(_glyphShape);

     vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        ERROR("No scalar point data in dataset %s", _dataSet->getName().c_str());
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetScalars() != NULL) {
            cellToPtData = 
                vtkSmartPointer<vtkCellDataToPointData>::New();
            cellToPtData->SetInput(ds);
            ds = cellToPtData->GetOutput();
        } else {
            ERROR("No scalar cell data in dataset %s", _dataSet->getName().c_str());
        }
    }

    _glyphGenerator->SetInput(ds);
    if (ds->GetPointData()->GetVectors() != NULL) {
	_glyphGenerator->SetScaleModeToScaleByVector();
    } else {
	_glyphGenerator->SetScaleModeToScaleByScalar();
    }
    _glyphGenerator->SetScaleFactor(_scaleFactor);
    _glyphGenerator->ScalingOn();
    _glyphGenerator->SetColorModeToColorByScalar();
    if (_glyphShape == CUBE ||
	_glyphShape == SPHERE) {
	_glyphGenerator->OrientOff();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
    }

    _pdMapper->SetInput(_glyphGenerator->GetOutput());

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        if (_lut == NULL) {
            _lut = vtkSmartPointer<vtkLookupTable>::New();
        }
    } else {
        vtkLookupTable *lut = ds->GetPointData()->GetScalars()->GetLookupTable();
        TRACE("Data set scalars lookup table: %p\n", lut);
        if (_lut == NULL) {
            if (lut)
                _lut = lut;
            else
                _lut = vtkSmartPointer<vtkLookupTable>::New();
        }
    }

    _lut->SetRange(dataRange);

    _pdMapper->UseLookupTableScalarRangeOn();
    _pdMapper->SetLookupTable(_lut);

    initProp();

    _prop->SetMapper(_pdMapper);
}

void Glyphs::setScaleFactor(double scale)
{
    _scaleFactor = scale;
    if (_glyphGenerator != NULL) {
	_glyphGenerator->SetScaleFactor(scale);
    }
}

/**
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *Glyphs::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Glyphs::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_pdMapper != NULL) {
        _pdMapper->UseLookupTableScalarRangeOn();
        _pdMapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Turn on/off rendering of this Glyphs
 */
void Glyphs::setVisibility(bool state)
{
    if (_prop != NULL) {
        _prop->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the Glyphs
 * 
 * \return Are the glyphs visible?
 */
bool Glyphs::getVisibility()
{
    if (_prop == NULL) {
        return false;
    } else {
        return (_prop->GetVisibility() != 0);
    }
}

/**
 * \brief Set opacity used to render the Glyphs
 */
void Glyphs::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_prop != NULL)
        _prop->GetProperty()->SetOpacity(opacity);
}

/**
 * \brief Get opacity used to render the Glyphs
 */
double Glyphs::getOpacity()
{
    return _opacity;
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Glyphs::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
}

/**
 * \brief Turn on/off lighting of this object
 */
void Glyphs::setLighting(bool state)
{
    _lighting = state;
    if (_prop != NULL)
        _prop->GetProperty()->SetLighting((state ? 1 : 0));
}
