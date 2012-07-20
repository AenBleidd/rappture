/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
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
#include <vtkCylinderSource.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformPolyDataFilter.h>

#include "RpGlyphs.h"
#include "RpVtkRenderer.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Glyphs::Glyphs(GlyphShape shape) :
    VtkGraphicsObject(),
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

    switch (_glyphShape) {
    case LINE:
        _glyphSource = vtkSmartPointer<vtkLineSource>::New();
        break;
    case ARROW: {
        _glyphSource = vtkSmartPointer<vtkArrowSource>::New();
        vtkSmartPointer<vtkArrowSource> arrow = vtkArrowSource::SafeDownCast(_glyphSource);
        arrow->SetTipResolution(8);
        arrow->SetShaftResolution(8);
    }
        break;
    case CONE: {
        _glyphSource = vtkSmartPointer<vtkConeSource>::New();
        vtkSmartPointer<vtkConeSource> cone = vtkConeSource::SafeDownCast(_glyphSource);
        cone->SetResolution(8);
    }
        break;
    case CUBE:
        _glyphSource = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        vtkPlatonicSolidSource::SafeDownCast(_glyphSource)->SetSolidTypeToCube();
        break;
    case CYLINDER: {
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
    case SPHERE: {
        _glyphSource = vtkSmartPointer<vtkSphereSource>::New();
        vtkSmartPointer<vtkSphereSource> sphere = vtkSphereSource::SafeDownCast(_glyphSource);
        sphere->SetThetaResolution(14);
        sphere->SetPhiResolution(14);
    }
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

#ifdef HAVE_GLYPH3D_MAPPER
    if (_glyphMapper != NULL) {
        _glyphMapper->SetSourceConnection(_glyphSource->GetOutputPort());
    }
#else
    if (_glyphGenerator != NULL) {
        _glyphGenerator->SetSourceConnection(_glyphSource->GetOutputPort());
    }
#endif

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

#ifdef HAVE_GLYPH3D_MAPPER
    if (_glyphMapper == NULL) {
        _glyphMapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
        _glyphMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _glyphMapper->ScalarVisibilityOn();
    }
#else
    if (_glyphGenerator == NULL) {
        _glyphGenerator = vtkSmartPointer<vtkGlyph3D>::New();
    }
    if (_pdMapper == NULL) {
        _pdMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _pdMapper->SetResolveCoincidentTopologyToPolygonOffset();
        _pdMapper->ScalarVisibilityOn();
        _pdMapper->SetInputConnection(_glyphGenerator->GetOutputPort());
    }
#endif

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
            cellToPtData->SetInput(ds);
            //cellToPtData->PassCellDataOn();
            cellToPtData->Update();
            ds = cellToPtData->GetOutput();
        } else {
            TRACE("No scalar data in dataset %s", _dataSet->getName().c_str());
        }
    }

#ifdef HAVE_GLYPH3D_MAPPER
    _glyphMapper->SetInputConnection(ds->GetProducerPort());
#else
    _glyphGenerator->SetInput(ds);
#endif

    if (ds->GetPointData()->GetVectors() != NULL) {
        TRACE("Setting scale mode to vector magnitude");
        setScalingMode(SCALE_BY_VECTOR_MAGNITUDE);
    } else {
        TRACE("Setting scale mode to scalar");
        setScalingMode(SCALE_BY_SCALAR);
    }

    double cellSizeRange[2];
    double avgSize;
    _dataSet->getCellSizeRange(cellSizeRange, &avgSize);
    //_dataScale = cellSizeRange[0] + (cellSizeRange[1] - cellSizeRange[0])/2.;
    _dataScale = avgSize;

    TRACE("Cell size range: %g,%g, Data scale factor: %g",
          cellSizeRange[0], cellSizeRange[1], _dataScale);

    // Normalize sizes to [0,1] * ScaleFactor
#ifdef HAVE_GLYPH3D_MAPPER
    _glyphMapper->SetClamping(_normalizeScale ? 1 : 0);
    _glyphMapper->SetScaleFactor(_scaleFactor * _dataScale);
    _glyphMapper->ScalingOn();
#else
    _glyphGenerator->SetClamping(_normalizeScale ? 1 : 0);
    _glyphGenerator->SetScaleFactor(_scaleFactor * _dataScale);
    _glyphGenerator->ScalingOn();
#endif

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

#ifdef HAVE_GLYPH3D_MAPPER
    getActor()->SetMapper(_glyphMapper);
    _glyphMapper->Update();
#else
    getActor()->SetMapper(_pdMapper);
    _pdMapper->Update();
#endif
}

/** 
 * \brief Control if field data range is normalized to [0,1] before
 * applying scale factor
 */
void Glyphs::setNormalizeScale(bool normalize)
{
    if (_normalizeScale != normalize) {
        _normalizeScale = normalize;
#ifdef HAVE_GLYPH3D_MAPPER
        if (_glyphMapper != NULL) {
            _glyphMapper->SetClamping(_normalizeScale ? 1 : 0);
        }
#else
        if (_glyphGenerator != NULL) {
            _glyphGenerator->SetClamping(_normalizeScale ? 1 : 0);
        }
#endif
    }
}

#ifdef HAVE_GLYPH3D_MAPPER
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

#else

/**
 * \brief Control how glyphs are scaled
 */
void Glyphs::setScalingMode(ScalingMode mode)
{
    _scalingMode = mode;
    if (_glyphGenerator != NULL) {
        switch (mode) {
        case SCALE_BY_SCALAR:
            _glyphGenerator->SetRange(_dataRange);
            _glyphGenerator->SetScaleModeToScaleByScalar();
            break;
        case SCALE_BY_VECTOR_MAGNITUDE:
            _glyphGenerator->SetRange(_vectorMagnitudeRange);
            _glyphGenerator->SetScaleModeToScaleByVector();
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
        case COLOR_BY_VECTOR_MAGNITUDE:
            _glyphGenerator->SetColorModeToColorByVector();
            _pdMapper->ScalarVisibilityOn();
            if (_lut != NULL) {
                _lut->SetVectorModeToMagnitude();
                _lut->SetRange(_vectorMagnitudeRange);
            }
            break;
        case COLOR_BY_SCALAR:
            _glyphGenerator->SetColorModeToColorByScalar();
            _pdMapper->ScalarVisibilityOn();
            if (_lut != NULL) {
                _lut->SetRange(_dataRange);
            }
            break;
        case COLOR_CONSTANT:
            _pdMapper->ScalarVisibilityOff();
            break;
        default:
            ERROR("Unsupported ColorMode: %d", mode);
        }
    }
}

#endif

/**
 * \brief Turn on/off orienting glyphs from a vector field
 */
void Glyphs::setOrient(bool state)
{
#ifdef HAVE_GLYPH3D_MAPPER
    if (_glyphMapper != NULL) {
        _glyphMapper->SetOrient(state ? 1 : 0);
    }
#else
    if (_glyphGenerator != NULL) {
        _glyphGenerator->SetOrient(state ? 1 : 0);
    }
#endif
}

/**
 * \brief Controls relative scaling of glyphs
 */
void Glyphs::setScaleFactor(double scale)
{
    _scaleFactor = scale;
#ifdef HAVE_GLYPH3D_MAPPER
    if (_glyphMapper != NULL) {
        _glyphMapper->SetScaleFactor(_scaleFactor * _dataScale);
    }
#else
    if (_glyphGenerator != NULL) {
        _glyphGenerator->SetScaleFactor(_scaleFactor * _dataScale);
    }
#endif
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
#ifdef HAVE_GLYPH3D_MAPPER
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
#else
    setColorMode(_colorMode);
    setScalingMode(_scalingMode);
#endif
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
#ifdef HAVE_GLYPH3D_MAPPER
        if (_glyphMapper != NULL) {
            _glyphMapper->UseLookupTableScalarRangeOn();
            _glyphMapper->SetLookupTable(_lut);
        }
#else
        if (_pdMapper != NULL) {
            _pdMapper->UseLookupTableScalarRangeOn();
            _pdMapper->SetLookupTable(_lut);
        }
#endif
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
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Glyphs::setClippingPlanes(vtkPlaneCollection *planes)
{
#ifdef HAVE_GLYPH3D_MAPPER
    if (_glyphMapper != NULL) {
        _glyphMapper->SetClippingPlanes(planes);
    }
#else
    if (_pdMapper != NULL) {
        _pdMapper->SetClippingPlanes(planes);
    }
#endif
}
