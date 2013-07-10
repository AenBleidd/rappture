/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <cfloat>

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
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkPointSource.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>

#include "Glyphs.h"
#include "Renderer.h"
#include "Math.h"
#include "Trace.h"

using namespace VtkVis;

Glyphs::Glyphs(GlyphShape shape) :
    GraphicsObject(),
    _glyphShape(shape),
    _scalingMode(SCALE_BY_VECTOR_MAGNITUDE),
    _dataScale(1.0),
    _scaleFactor(1.0),
    _normalizeScale(true),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR)
{
    _faceCulling = true;
    _scalingFieldRange[0] = DBL_MAX;
    _scalingFieldRange[1] = -DBL_MAX;
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

Glyphs::~Glyphs()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Glyphs for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Glyphs with NULL DataSet");
#endif
}

void Glyphs::setDataSet(DataSet *dataSet,
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
 * \brief Set the shape of the glyphs
 */
void Glyphs::setGlyphShape(GlyphShape shape)
{
    _glyphShape = shape;

    // Note: using vtkTransformPolyDataFilter instead of the vtkGlyph3D's 
    // SourceTransform because of a bug: vtkGlyph3D doesn't transform normals
    // by the SourceTransform, so the lighting would be wrong

    bool needsNormals = false;
    double featureAngle = 90.;

    switch (_glyphShape) {
    case LINE:
        // Length of 1, centered at origin
        _glyphSource = vtkSmartPointer<vtkLineSource>::New();
        break;
    case ARROW: {
        // Height of 1, Tip radius .1, tip length .35, shaft radius .03
        _glyphSource = vtkSmartPointer<vtkArrowSource>::New();
        vtkSmartPointer<vtkArrowSource> arrow = vtkArrowSource::SafeDownCast(_glyphSource);
        arrow->SetTipResolution(8);
        arrow->SetShaftResolution(8);
        needsNormals = true;
    }
        break;
    case CONE: {
        // base length of 1, height 1, centered at origin
        _glyphSource = vtkSmartPointer<vtkConeSource>::New();
        vtkSmartPointer<vtkConeSource> cone = vtkConeSource::SafeDownCast(_glyphSource);
        cone->SetResolution(8);
        needsNormals = true;
    }
        break;
    case CUBE:
        // Sides of length 1
        _glyphSource = vtkSmartPointer<vtkCubeSource>::New();
        break;
    case CYLINDER: {
        // base length of 1, height 1, centered at origin
        vtkSmartPointer<vtkCylinderSource> csource = vtkSmartPointer<vtkCylinderSource>::New();
        csource->SetResolution(8);
        _glyphSource = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        _glyphSource->SetInputConnection(csource->GetOutputPort());
        vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
        trans->RotateZ(-90.0);
        vtkTransformPolyDataFilter::SafeDownCast(_glyphSource)->SetTransform(trans);
      }
        break;
    case DODECAHEDRON:
        // Radius of 1
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToDodecahedron();
        needsNormals = true;
        featureAngle = 30.;
        break;
    case ICOSAHEDRON:
        // Radius of 1
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToIcosahedron();
        needsNormals = true;
        featureAngle = 30.;
        break;
    case OCTAHEDRON:
        // Radius of 1
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToOctahedron();
        needsNormals = true;
        featureAngle = 30.;
        break;
    case POINT:
        _glyphSource = vtkSmartPointer<vtkPointSource>::New();
        vtkPointSource::SafeDownCast(_glyphSource)->SetNumberOfPoints(1);
        vtkPointSource::SafeDownCast(_glyphSource)->SetCenter(0, 0, 0);
        vtkPointSource::SafeDownCast(_glyphSource)->SetRadius(0);
        vtkPointSource::SafeDownCast(_glyphSource)->SetDistributionToUniform();
        break;
    case SPHERE: {
        // Default radius 0.5
        _glyphSource = vtkSmartPointer<vtkSphereSource>::New();
        vtkSmartPointer<vtkSphereSource> sphere = vtkSphereSource::SafeDownCast(_glyphSource);
        sphere->SetThetaResolution(14);
        sphere->SetPhiResolution(14);
    }
        break;
    case TETRAHEDRON:
        // Radius of 1
        // FIXME: need to rotate inital orientation
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToTetrahedron();
        needsNormals = true;
        featureAngle = 30.;
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

    if (needsNormals && _glyphMapper != NULL) {
        vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
        normalFilter->SetFeatureAngle(featureAngle);
        normalFilter->SetInputConnection(_glyphSource->GetOutputPort());
        _glyphMapper->SetSourceConnection(normalFilter->GetOutputPort());
    } else if (_glyphMapper != NULL) {
        _glyphMapper->SetSourceConnection(_glyphSource->GetOutputPort());
    }
}

void Glyphs::setQuality(double quality)
{
    if (quality > 10.0)
        quality = 10.0;

    switch (_glyphShape) {
    case ARROW: {
        vtkSmartPointer<vtkArrowSource> arrow = vtkArrowSource::SafeDownCast(_glyphSource);
        int res = (int)(quality * 8.);
        if (res < 3) res = 3;
        arrow->SetTipResolution(res);
        arrow->SetShaftResolution(res);
    }
        break;
    case CONE: {
        vtkSmartPointer<vtkConeSource> cone = vtkConeSource::SafeDownCast(_glyphSource);
        int res = (int)(quality * 8.);
        if (res < 3) res = 3;
        cone->SetResolution(res);
    }
        break;
    case CYLINDER: {
        vtkSmartPointer<vtkCylinderSource> csource = 
            vtkCylinderSource::SafeDownCast(_glyphSource->GetInputAlgorithm(0, 0));
        if (csource == NULL) {
            ERROR("Can't get cylinder glyph source");
            return;
        }
        int res = (int)(quality * 8.);
        if (res < 3) res = 3;
        csource->SetResolution(res);
    }
        break;
    case SPHERE: {
        vtkSmartPointer<vtkSphereSource> sphere = vtkSphereSource::SafeDownCast(_glyphSource);
        int thetaRes = (int)(quality * 14.);
        int phiRes = thetaRes;
        if (thetaRes < 4) thetaRes = 4;
        if (phiRes < 3) phiRes = 3;

        sphere->SetThetaResolution(thetaRes);
        sphere->SetPhiResolution(phiRes);
    }
        break;
    default:
        break;
    }
    if (_glyphMapper != NULL) {
        _glyphMapper->Modified();
        _glyphMapper->Update();
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

    if (_glyphMapper == NULL) {
        _glyphMapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
        _glyphMapper->SetResolveCoincidentTopologyToPolygonOffset();
        // If there are color scalars, use them without lookup table (if scalar visibility is on)
        _glyphMapper->SetColorModeToDefault();
        _glyphMapper->ScalarVisibilityOn();
    }

    initProp();

    setGlyphShape(_glyphShape);

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetScalars() != NULL) {
            TRACE("Generating point data scalars from cell data for: %s", _dataSet->getName().c_str());
            cellToPtData = 
                vtkSmartPointer<vtkCellDataToPointData>::New();
#ifdef USE_VTK6
            cellToPtData->SetInputData(ds);
#else
            cellToPtData->SetInput(ds);
#endif
            //cellToPtData->PassCellDataOn();
            cellToPtData->Update();
            ds = cellToPtData->GetOutput();
        } else {
            TRACE("No scalar data in dataset %s", _dataSet->getName().c_str());
        }
    }

#ifdef USE_VTK6
    _glyphMapper->SetInputData(ds);
#else
    _glyphMapper->SetInputConnection(ds->GetProducerPort());
#endif

    if (ds->GetPointData()->GetVectors() != NULL) {
        TRACE("Setting scale mode to vector magnitude");
        setScalingMode(SCALE_BY_VECTOR_MAGNITUDE);
    } else {
        TRACE("Setting scale mode to scalar");
        setScalingMode(SCALE_BY_SCALAR);
    }

    if (_dataSet->isCloud()) {
        PrincipalPlane plane;
        double offset;
        if (_dataSet->is2D(&plane, &offset)) {
            // 2D cloud
            vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
            if (plane == PLANE_ZY) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(90, 0, 1, 0);
                if (offset != 0.0) {
                    trans->Translate(-offset, 0, 0);
                }
                mesher->SetTransform(trans);
            } else if (plane == PLANE_XZ) {
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->RotateWXYZ(-90, 1, 0, 0);
                if (offset != 0.0) {
                    trans->Translate(0, -offset, 0);
                }
                mesher->SetTransform(trans);
            } else if (offset != 0.0) {
                // XY with Z offset
                vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                trans->Translate(0, 0, -offset);
                mesher->SetTransform(trans);
            }
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            mesher->Update();
            vtkPolyData *pd = mesher->GetOutput();
            if (pd->GetNumberOfPolys() == 0) {
                // Meshing failed, fall back to scale based on bounds
                double bounds[6];
                _dataSet->getBounds(bounds);
                double xlen = bounds[1] - bounds[0];
                double ylen = bounds[3] - bounds[2];
                double zlen = bounds[5] - bounds[4];
                double max = max3(xlen, ylen, zlen);
                _dataScale = max / 64.0;
            } else {
                double cellSizeRange[2];
                double avgSize;
                DataSet::getCellSizeRange(pd, cellSizeRange, &avgSize);
                _dataScale = avgSize * 2.0;
            }
        } else {
            // 3D cloud
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            mesher->Update();
            vtkUnstructuredGrid *ugrid = mesher->GetOutput();
            if (ugrid->GetNumberOfCells() == 0) {
                // Meshing failed, fall back to scale based on bounds
                double bounds[6];
                _dataSet->getBounds(bounds);
                double xlen = bounds[1] - bounds[0];
                double ylen = bounds[3] - bounds[2];
                double zlen = bounds[5] - bounds[4];
                double max = max3(xlen, ylen, zlen);
                _dataScale = max / 64.0;
            } else {
                double cellSizeRange[2];
                double avgSize;
                DataSet::getCellSizeRange(ugrid, cellSizeRange, &avgSize);
                _dataScale = avgSize * 2.0;
            }
        }
    } else {
        double cellSizeRange[2];
        double avgSize;
        _dataSet->getCellSizeRange(cellSizeRange, &avgSize);
        _dataScale = avgSize * 2.0;
    }

    TRACE("Data scale factor: %g", _dataScale);

    // Normalize sizes to [0,1] * ScaleFactor
    _glyphMapper->SetClamping(_normalizeScale ? 1 : 0);
    if (_normalizeScale) {
        _glyphMapper->SetScaleFactor(_scaleFactor * _dataScale);
        TRACE("Setting scale factor: %g", _scaleFactor * _dataScale);
    } else {
        _glyphMapper->SetScaleFactor(_scaleFactor);
        TRACE("Setting scale factor: %g", _scaleFactor);
    }
    _glyphMapper->ScalingOn();

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    if (ds->GetPointData()->GetScalars() == NULL) {
        TRACE("Setting color mode to vector magnitude");
        setColorMode(COLOR_BY_VECTOR_MAGNITUDE);
    } else {
        TRACE("Setting color mode to scalar");
        setColorMode(COLOR_BY_SCALAR);
    }

    getActor()->SetMapper(_glyphMapper);
    _glyphMapper->Update();
}

/** 
 * \brief Control if field data range is normalized to [0,1] before
 * applying scale factor
 */
void Glyphs::setNormalizeScale(bool normalize)
{
    if (_normalizeScale != normalize) {
        _normalizeScale = normalize;
        if (_glyphMapper != NULL) {
            _glyphMapper->SetClamping(_normalizeScale ? 1 : 0);
            if (_normalizeScale) {
                _glyphMapper->SetScaleFactor(_scaleFactor * _dataScale);
                TRACE("Setting scale factor: %g", _scaleFactor * _dataScale);
            } else {
                _glyphMapper->SetScaleFactor(_scaleFactor);
                TRACE("Setting scale factor: %g", _scaleFactor);
            }
        }
    }
}

/**
 * \brief Turn on/off orienting glyphs from a vector field
 */
void Glyphs::setOrientMode(bool mode, const char *name)
{
    if (_glyphMapper != NULL) {
        _glyphMapper->SetOrient(mode ? 1 : 0);
        if (name != NULL && strlen(name) > 0) {
            _glyphMapper->SetOrientationArray(name);
        } else {
            _glyphMapper->SetOrientationArray(vtkDataSetAttributes::VECTORS);
        }
    }
}

/**
 * \brief Control how glyphs are scaled
 */
void Glyphs::setScalingMode(ScalingMode mode, const char *name, double range[2])
{
    _scalingMode = mode;

    if (_dataSet == NULL || _glyphMapper == NULL)
        return;

    if (name != NULL && strlen(name) > 0) {
        if (!_dataSet->hasField(name, DataSet::POINT_DATA)) {
            ERROR("Field not found: %s", name);
            return;
        }
        _scalingFieldName = name;
    } else
        _scalingFieldName.clear();
    if (range == NULL) {
        _scalingFieldRange[0] = DBL_MAX;
        _scalingFieldRange[1] = -DBL_MAX;
    } else {
        memcpy(_scalingFieldRange, range, sizeof(double)*2);
    }

    if (name != NULL && strlen(name) > 0) {
        _glyphMapper->SetScaleArray(name);
    } else {
        if (mode == SCALE_BY_SCALAR) {
            _glyphMapper->SetScaleArray(vtkDataSetAttributes::SCALARS);
        } else {
            _glyphMapper->SetScaleArray(vtkDataSetAttributes::VECTORS);
        }
    }

    if (range != NULL) {
        TRACE("Setting size range to: %g,%g", range[0], range[1]);
        _glyphMapper->SetRange(range);
    } else if (name != NULL && strlen(name) > 0) {
        double r[2];
        DataSet::DataAttributeType type = DataSet::POINT_DATA;
        int comp = -1;

        if (_renderer->getUseCumulativeRange()) {
            int numComponents;
            if  (!_dataSet->getFieldInfo(name, type, &numComponents)) {
                ERROR("Field not found: %s, type: %d", name, type);
                return;
            } else if (mode == SCALE_BY_VECTOR_COMPONENTS && numComponents < 3) {
                ERROR("Field %s needs 3 components but has only %d components",
                      name, numComponents);
                return;
            }
            if (mode == SCALE_BY_VECTOR_COMPONENTS) {
                double tmpR[2];
                _renderer->getCumulativeDataRange(tmpR, name, type, numComponents, 0);
                r[0] = tmpR[0];
                r[1] = tmpR[1];
                _renderer->getCumulativeDataRange(tmpR, name, type, numComponents, 1);
                r[0] = min2(r[0], tmpR[0]);
                r[1] = max2(r[1], tmpR[1]);
                _renderer->getCumulativeDataRange(tmpR, name, type, numComponents, 2);
                r[0] = min2(r[0], tmpR[0]);
                r[1] = max2(r[1], tmpR[1]);
            } else {
                _renderer->getCumulativeDataRange(r, name, type, numComponents, comp);
            }
        } else {
            if (mode == SCALE_BY_VECTOR_COMPONENTS) {
                double tmpR[2];
                _dataSet->getDataRange(tmpR, name, type, 0);
                r[0] = tmpR[0];
                r[1] = tmpR[1];
                _dataSet->getDataRange(tmpR, name, type, 1);
                r[0] = min2(r[0], tmpR[0]);
                r[1] = max2(r[1], tmpR[1]);
                _dataSet->getDataRange(tmpR, name, type, 2);
                r[0] = min2(r[0], tmpR[0]);
                r[1] = max2(r[1], tmpR[1]);
            } else {
                _dataSet->getDataRange(r, name, type, comp);
            }
        }
        TRACE("Setting size range to: %g,%g", r[0], r[1]);
        _glyphMapper->SetRange(r);
    } else {
        switch (mode) {
        case SCALE_BY_SCALAR:
            TRACE("Setting size range to: %g,%g", _dataRange[0], _dataRange[1]);
            _glyphMapper->SetRange(_dataRange);
            break;
        case SCALE_BY_VECTOR_MAGNITUDE:
            TRACE("Setting size range to: %g,%g", _vectorMagnitudeRange[0], _vectorMagnitudeRange[1]);
            _glyphMapper->SetRange(_vectorMagnitudeRange);
            break;
        case SCALE_BY_VECTOR_COMPONENTS: {
            double sizeRange[2];
            sizeRange[0] = _vectorComponentRange[0][0];
            sizeRange[1] = _vectorComponentRange[0][1];
            sizeRange[0] = min2(sizeRange[0], _vectorComponentRange[1][0]);
            sizeRange[1] = max2(sizeRange[1], _vectorComponentRange[1][1]);
            sizeRange[0] = min2(sizeRange[0], _vectorComponentRange[2][0]);
            sizeRange[1] = max2(sizeRange[1], _vectorComponentRange[2][1]);
            TRACE("Setting size range to: %g,%g", sizeRange[0], sizeRange[1]);
            _glyphMapper->SetRange(sizeRange);
        }
            break;
        case SCALING_OFF:
        default:
            ;
        }
    }

    switch (mode) {
    case SCALE_BY_SCALAR:
    case SCALE_BY_VECTOR_MAGNITUDE:
        _glyphMapper->SetScaleModeToScaleByMagnitude();
        break;
    case SCALE_BY_VECTOR_COMPONENTS:
        _glyphMapper->SetScaleModeToScaleByVectorComponents();
        break;
    case SCALING_OFF:
    default:
        _glyphMapper->SetScaleModeToNoDataScaling();
    }
}

void Glyphs::setScalingMode(ScalingMode mode)
{
    setScalingMode(mode, NULL, NULL);
}

void Glyphs::setColorMode(ColorMode mode,
                          const char *name, double range[2])
{
    _colorMode = mode;
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

    if (_dataSet == NULL || _glyphMapper == NULL)
        return;

    if (name != NULL && strlen(name) > 0) {
        _glyphMapper->SetScalarModeToUsePointFieldData();
        _glyphMapper->SelectColorArray(name);
    } else {
        _glyphMapper->SetScalarModeToDefault();
    }

    if (_lut != NULL) {
        if (range != NULL) {
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

            DataSet::DataAttributeType type = DataSet::POINT_DATA;

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
            TRACE("Setting lut range to: %g,%g", r[0], r[1]);
            _lut->SetRange(r);
        }
    }

    switch (mode) {
    case COLOR_BY_SCALAR:
        _glyphMapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _glyphMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        _glyphMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _glyphMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _glyphMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        _glyphMapper->ScalarVisibilityOff();
        break;
    }
}

void Glyphs::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL)
        return;

    switch (mode) {
    case COLOR_BY_SCALAR:
        setColorMode(mode,
                     _dataSet->getActiveScalarsName(),
                     _dataRange);
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        setColorMode(mode,
                     _dataSet->getActiveVectorsName(),
                     _vectorMagnitudeRange);
        break;
    case COLOR_BY_VECTOR_X:
        setColorMode(mode,
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[0]);
        break;
    case COLOR_BY_VECTOR_Y:
        setColorMode(mode,
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[1]);
        break;
    case COLOR_BY_VECTOR_Z:
        setColorMode(mode,
                     _dataSet->getActiveVectorsName(),
                     _vectorComponentRange[2]);
        break;
    case COLOR_CONSTANT:
    default:
        setColorMode(mode, NULL, NULL);
        break;
    }
}

/**
 * \brief Turn on/off orienting glyphs from a vector field
 */
void Glyphs::setOrient(bool state)
{
    if (_glyphMapper != NULL) {
        _glyphMapper->SetOrient(state ? 1 : 0);
    }
}

/**
 * \brief Controls relative scaling of glyphs
 */
void Glyphs::setScaleFactor(double scale)
{
    _scaleFactor = scale;
    if (_glyphMapper != NULL) {
        if (_normalizeScale) {
            _glyphMapper->SetScaleFactor(_scaleFactor * _dataScale);
            TRACE("Setting scale factor: %g", _scaleFactor * _dataScale);
        } else {
            _glyphMapper->SetScaleFactor(_scaleFactor);
            TRACE("Setting scale factor: %g", _scaleFactor);
        }
    }
}

void Glyphs::updateRanges(Renderer *renderer)
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

    // Need to update color map ranges and/or active vector field
    double *rangePtr = _colorFieldRange;
    if (_colorFieldRange[0] > _colorFieldRange[1]) {
        rangePtr = NULL;
    }
    setColorMode(_colorMode, _colorFieldName.c_str(), rangePtr);

    rangePtr = _scalingFieldRange;
    if (_scalingFieldRange[0] > _scalingFieldRange[1]) {
        rangePtr = NULL;
    }
    setScalingMode(_scalingMode, _scalingFieldName.c_str(), rangePtr);
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
        if (_glyphMapper != NULL) {
            _glyphMapper->UseLookupTableScalarRangeOn();
            _glyphMapper->SetLookupTable(_lut);
        }
        _lut->DeepCopy(cmap->getLookupTable());
        switch (_colorMode) {
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

    switch (_colorMode) {
    case COLOR_BY_VECTOR_MAGNITUDE:
        _lut->SetVectorModeToMagnitude();
        break;
    case COLOR_BY_VECTOR_X:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(0);
        break;
    case COLOR_BY_VECTOR_Y:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(1);
        break;
    case COLOR_BY_VECTOR_Z:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(2);
        break;
    default:
        break;
    }
}

/**
 * \brief Limit the number of glyphs displayed
 *
 * The choice of glyphs to display can be based on sampling every
 * n-th point (ratio) or by random sample
 *
 * \param max Maximum number of glyphs to display, negative means display all
 * \param random Flag to enable/disable random sampling
 * \param offset If random is false, this controls the first sample point
 * \param ratio If random is false, this ratio controls every n-th point sampling
 */
void Glyphs::setMaximumNumberOfGlyphs(int max, bool random, int offset, int ratio)
{
    if (_dataSet == NULL || _glyphMapper == NULL)
        return;

    if (max < 0) {
        if (_maskPoints != NULL) {
#ifdef USE_VTK6
            _glyphMapper->SetInputData(_dataSet->getVtkDataSet());
#else
            _glyphMapper->SetInputConnection(_dataSet->getVtkDataSet()->GetProducerPort());
#endif
            _maskPoints = NULL;
        }
    } else {
        if (_maskPoints == NULL) {
            _maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
        }
#ifdef USE_VTK6
        _maskPoints->SetInputData(_dataSet->getVtkDataSet());
#else
        _maskPoints->SetInput(_dataSet->getVtkDataSet());
#endif
        _maskPoints->SetMaximumNumberOfPoints(max);
        _maskPoints->SetOffset(offset);
        _maskPoints->SetOnRatio(ratio);
        _maskPoints->SetRandomMode((random ? 1 : 0));
        _maskPoints->GenerateVerticesOff();
        _glyphMapper->SetInputConnection(_maskPoints->GetOutputPort());
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Glyphs::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_glyphMapper != NULL) {
        _glyphMapper->SetClippingPlanes(planes);
    }
}
