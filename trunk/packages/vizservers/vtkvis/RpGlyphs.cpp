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
#include <vtkLineSource.h>
#include <vtkArrowSource.h>
#include <vtkConeSource.h>
#include <vtkCylinderSource.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformPolyDataFilter.h>

#include "RpGlyphs.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Glyphs::Glyphs(GlyphShape shape) :
    VtkGraphicsObject(),
    _glyphShape(shape),
    _scalingMode(SCALE_BY_VECTOR_MAGNITUDE),
    _dataScale(1.0),
    _scaleFactor(1.0),
    _colorMode(COLOR_BY_SCALAR),
    _colorMap(NULL)
{
    _faceCulling = true;
}

Glyphs::~Glyphs()
{
}

void Glyphs::setDataSet(DataSet *dataSet,
                        bool useCumulative,
                        double scalarRange[2],
                        double vectorMagnitudeRange[2],
                        double vectorComponentRange[3][2])
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;

        if (useCumulative) {
            _dataRange[0] = scalarRange[0];
            _dataRange[1] = scalarRange[1];
            _vectorMagnitudeRange[0] = vectorMagnitudeRange[0];
            _vectorMagnitudeRange[1] = vectorMagnitudeRange[1];
            for (int i = 0; i < 3; i++) {
                _vectorComponentRange[i][0] = vectorComponentRange[i][0];
                _vectorComponentRange[i][1] = vectorComponentRange[i][1];
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
 * \brief Set the shape of the glyphs
 */
void Glyphs::setGlyphShape(GlyphShape shape)
{
    _glyphShape = shape;

    // Note: using vtkTransformPolyDataFilter instead of the vtkGlyph3D's 
    // SourceTransform because of a bug: vtkGlyph3D doesn't transform normals
    // by the SourceTransform, so the lighting would be wrong

    switch (_glyphShape) {
    case LINE:
        _glyphSource = vtkSmartPointer<vtkLineSource>::New();
        break;
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
    case CYLINDER: {
        vtkSmartPointer<vtkCylinderSource> csource = vtkSmartPointer<vtkCylinderSource>::New();
        vtkCylinderSource::SafeDownCast(csource)->SetResolution(6);
        _glyphSource = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        _glyphSource->SetInputConnection(csource->GetOutputPort());
        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
        trans->RotateZ(-90.0);
        vtkTransformPolyDataFilter::SafeDownCast(_glyphSource)->SetTransform(trans);
      }
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
        // FIXME: need to rotate inital orientation
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToTetrahedron();
        break;
    default:
        ERROR("Unknown glyph shape: %d", _glyphShape);
        return;
    }

    if (_glyphShape == ICOSAHEDRON ||
        _glyphShape == TETRAHEDRON) {
        // These shapes are created with front faces pointing inside
        setCullFace(CULL_FRONT);
    } else {
        setCullFace(CULL_BACK);
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

    if (_glyphGenerator == NULL) {
	_glyphGenerator = vtkSmartPointer<vtkGlyph3D>::New();
    }

    initProp();

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
        TRACE("Setting scale mode to vector magnitude");
        _scalingMode = SCALE_BY_VECTOR_MAGNITUDE;
        _glyphGenerator->SetScaleModeToScaleByVector();
        _glyphGenerator->SetRange(_vectorMagnitudeRange);
    } else {
        TRACE("Setting scale mode to scalar");
        _scalingMode = SCALE_BY_SCALAR;
        _glyphGenerator->SetScaleModeToScaleByScalar();
        _glyphGenerator->SetRange(_dataRange);
    }

    // Normalize sizes to [0,1] * ScaleFactor
    _glyphGenerator->ClampingOn();

    double cellSizeRange[2];
    double avgSize;
    _dataSet->getCellSizeRange(cellSizeRange, &avgSize);
    //_dataScale = cellSizeRange[0] + (cellSizeRange[1] - cellSizeRange[0])/2.;
    _dataScale = avgSize;

    TRACE("Cell size range: %g,%g, Data scale factor: %g",
          cellSizeRange[0], cellSizeRange[1], _dataScale);

    _glyphGenerator->SetScaleFactor(_scaleFactor * _dataScale);
    _glyphGenerator->ScalingOn();

    if (ds->GetPointData()->GetScalars() == NULL) {
        TRACE("Setting color mode to vector magnitude");
        _glyphGenerator->SetColorModeToColorByVector();
        _colorMode = COLOR_BY_VECTOR_MAGNITUDE;
    } else {
        TRACE("Setting color mode to scalar");
        _glyphGenerator->SetColorModeToColorByScalar();
        _colorMode = COLOR_BY_SCALAR;
    }

    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
    }

    _pdMapper->SetInputConnection(_glyphGenerator->GetOutputPort());

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
}

/**
 * \brief Control how glyphs are scaled
 */
void Glyphs::setScalingMode(ScalingMode mode)
{
    _scalingMode = mode;
    if (_glyphGenerator != NULL) {
        switch (mode) {
        case SCALE_BY_SCALAR: {
            _glyphGenerator->SetRange(_dataRange);
            _glyphGenerator->SetScaleModeToScaleByScalar();
        }
            break;
        case SCALE_BY_VECTOR_MAGNITUDE: {
            _glyphGenerator->SetRange(_vectorMagnitudeRange);
            _glyphGenerator->SetScaleModeToScaleByVector();
        }
            break;
        case SCALE_BY_VECTOR_COMPONENTS: {
            double sizeRange[2];
            sizeRange[0] = _vectorComponentRange[0][0];
            sizeRange[1] = _vectorComponentRange[0][1];
            sizeRange[0] = min2(sizeRange[0], _vectorComponentRange[1][0]);
            sizeRange[1] = max2(sizeRange[1], _vectorComponentRange[1][1]);
            sizeRange[0] = min2(sizeRange[0], _vectorComponentRange[2][0]);
            sizeRange[1] = max2(sizeRange[1], _vectorComponentRange[2][1]);
            _glyphGenerator->SetRange(sizeRange);
            _glyphGenerator->SetScaleModeToScaleByVectorComponents();
        }
            break;
        case SCALING_OFF:
        default:
            _glyphGenerator->SetScaleModeToDataScalingOff();
        }
    }
}

/**
 * \brief Control how glyphs are colored
 */
void Glyphs::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_glyphGenerator != NULL) {
        switch (mode) {
        case COLOR_BY_SCALE:
            _glyphGenerator->SetColorModeToColorByScale();
            _pdMapper->ScalarVisibilityOn();
            double dataRange[2];
            dataRange[0] = 0;
            dataRange[1] = 1;
            _lut->SetRange(dataRange);
            break;
        case COLOR_BY_VECTOR_MAGNITUDE: {
            _glyphGenerator->SetColorModeToColorByVector();
            _pdMapper->ScalarVisibilityOn();
            _lut->SetRange(_vectorMagnitudeRange);
        }
            break;
        case COLOR_BY_SCALAR: {
            _glyphGenerator->SetColorModeToColorByScalar();
            _pdMapper->ScalarVisibilityOn();
            _lut->SetRange(_dataRange);
        }
            break;
        case COLOR_CONSTANT:
        default:
            _pdMapper->ScalarVisibilityOff();
        }
     }
}

/**
 * \brief Controls relative scaling of glyphs
 */
void Glyphs::setScaleFactor(double scale)
{
    _scaleFactor = scale;
    if (_glyphGenerator != NULL) {
        _glyphGenerator->SetScaleFactor(_scaleFactor * _dataScale);
    }
}

void Glyphs::updateRanges(bool useCumulative,
                          double scalarRange[2],
                          double vectorMagnitudeRange[2],
                          double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
        _vectorMagnitudeRange[0] = vectorMagnitudeRange[0];
        _vectorMagnitudeRange[1] = vectorMagnitudeRange[1];
        for (int i = 0; i < 3; i++) {
            _vectorComponentRange[i][0] = vectorComponentRange[i][0];
            _vectorComponentRange[i][1] = vectorComponentRange[i][1];
        }
    } else {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges and/or active vector field
    setColorMode(_colorMode);
    setScalingMode(_scalingMode);
}

/**
 * \brief Called when the color map has been edited
 */
void Glyphs::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Glyphs::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_pdMapper != NULL) {
            _pdMapper->UseLookupTableScalarRangeOn();
            _pdMapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());

    switch (_colorMode) {
    case COLOR_BY_SCALE: {
        double dataRange[2];
        dataRange[0] = 0;
        dataRange[1] = 1;
        _lut->SetRange(dataRange);
    }
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _lut->SetRange(_vectorMagnitudeRange);
        break;
    case COLOR_BY_SCALAR:
    default:
        _lut->SetRange(_dataRange);
        break;
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
