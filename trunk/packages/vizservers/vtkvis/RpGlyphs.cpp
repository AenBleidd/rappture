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
    VtkGraphicsObject(),
    _glyphShape(ARROW),
    _scaleFactor(1.0)
{
    _backfaceCulling = true;
}

Glyphs::~Glyphs()
{
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
	_glyphGenerator->SetSourceConnection(_glyphSource->GetOutputPort());
    }
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
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
        WARN("No scalar point data in dataset %s", _dataSet->getName().c_str());
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetScalars() != NULL) {
            cellToPtData = 
                vtkSmartPointer<vtkCellDataToPointData>::New();
            cellToPtData->SetInput(ds);
            //cellToPtData->PassCellDataOn();
            cellToPtData->Update();
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

    if (ds->GetPointData()->GetScalars() == NULL) {
        _glyphGenerator->SetColorModeToColorByVector();
    } else {
        _glyphGenerator->SetColorModeToColorByScalar();
     }
    if (_glyphShape == SPHERE) {
	_glyphGenerator->OrientOff();
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
    }
    _pdMapper->SetInputConnection(_glyphGenerator->GetOutputPort());

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

    if (ds->GetPointData()->GetScalars() == NULL) {
        _pdMapper->UseLookupTableScalarRangeOff();
    } else {
        _lut->SetRange(dataRange);
        _pdMapper->UseLookupTableScalarRangeOn();
    }

    _pdMapper->SetLookupTable(_lut);

    initProp();

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

/**
 * \brief Control how glyphs are scaled
 */
void Glyphs::setScalingMode(ScalingMode mode)
{
    if (_glyphGenerator != NULL) {
        switch (mode) {
        case SCALE_BY_SCALAR:
            _glyphGenerator->SetScaleModeToScaleByScalar();
            _glyphGenerator->ScalingOn();
            break;
        case SCALE_BY_VECTOR:
            _glyphGenerator->SetScaleModeToScaleByVector();
            _glyphGenerator->ScalingOn();
            break;
        case SCALE_BY_VECTOR_COMPONENTS:
            _glyphGenerator->SetScaleModeToScaleByVectorComponents();
            _glyphGenerator->ScalingOn();
            break;
        case SCALING_OFF:
        default:
            _glyphGenerator->SetScaleModeToDataScalingOff();
            _glyphGenerator->ScalingOff();
        }
        _pdMapper->Update();
    }
}

/**
 * \brief Control how glyphs are colored
 */
void Glyphs::setColorMode(ColorMode mode)
{
    if (_glyphGenerator != NULL) {
        switch (mode) {
        case COLOR_BY_SCALE:
            _glyphGenerator->SetColorModeToColorByScale();
            break;
        case COLOR_BY_VECTOR:
            _glyphGenerator->SetColorModeToColorByVector();
            break;
        case COLOR_BY_SCALAR:
        default:
            _glyphGenerator->SetColorModeToColorByScalar();
            break;
        }
        _pdMapper->Update();
    }
}

/**
 * \brief Controls relative scaling of glyphs
 */
void Glyphs::setScaleFactor(double scale)
{
    _scaleFactor = scale;
    if (_glyphGenerator != NULL) {
        _glyphGenerator->SetScaleFactor(scale);
        _pdMapper->Update();
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
        _pdMapper->SetLookupTable(_lut);
    }
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
