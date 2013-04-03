/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>
#include <cfloat>
#include <cstring>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSetMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkCutter.h>
#include <vtkDataSetSurfaceFilter.h>

#include "Cutplane.h"
#include "Renderer.h"
#include "Trace.h"

#define MESH_POINT_CLOUDS

using namespace VtkVis;

Cutplane::Cutplane() :
    GraphicsObject(),
    _pipelineInitialized(false),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

Cutplane::~Cutplane()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Cutplane for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Cutplane with NULL DataSet");
#endif
}

void Cutplane::setDataSet(DataSet *dataSet,
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
 * \brief Create and initialize a VTK Prop to render the object
 */
void Cutplane::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();

        for (int i = 0; i < 3; i++) {
            _actor[i] = vtkSmartPointer<vtkActor>::New();
            _borderActor[i] = vtkSmartPointer<vtkActor>::New();
            //_actor[i]->VisibilityOff();
            vtkProperty *property = _actor[i]->GetProperty();
            property->SetColor(_color[0], _color[1], _color[2]);
            property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
            property->SetLineWidth(_edgeWidth);
            property->SetPointSize(_pointSize);
            property->EdgeVisibilityOff();
            property->SetOpacity(_opacity);
            property->SetAmbient(.2);
            if (!_lighting)
                property->LightingOff();
            if (_faceCulling && _opacity == 1.0) {
                setCulling(property, true);
            }
            property = _borderActor[i]->GetProperty();
            property->SetColor(_color[0], _color[1], _color[2]);
            property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
            property->SetLineWidth(_edgeWidth);
            property->SetPointSize(_pointSize);
            property->EdgeVisibilityOff();
            property->SetOpacity(_opacity);
            property->SetAmbient(.2);
            property->LightingOff();
            
            getAssembly()->AddPart(_borderActor[i]);
        }
    } else {
        for (int i = 0; i < 3; i++) {
            getAssembly()->RemovePart(_actor[i]);
        }
    }
}

/**
 * \brief Set the material color (sets ambient, diffuse, and specular)
 */
void Cutplane::setColor(float color[3])
{
    for (int i = 0; i < 3; i++)
        _color[i] = color[i];

    for (int i = 0; i < 3; i++) {
        if (_borderActor[i] != NULL) {
            _borderActor[i]->GetProperty()->SetColor(color[0], color[1], color[2]);
        }
    }
}

void Cutplane::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    double bounds[6];
    _dataSet->getBounds(bounds);
    // Mapper, actor to render color-mapped data set
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] == NULL) {
            _mapper[i] = vtkSmartPointer<vtkDataSetMapper>::New();
            // Map scalars through lookup table regardless of type
            _mapper[i]->SetColorModeToMapScalars();
        }
        if (_cutPlane[i] == NULL) {
            _cutPlane[i] = vtkSmartPointer<vtkPlane>::New();
            switch (i) {
            case 0:
                _cutPlane[i]->SetNormal(1, 0, 0);
                _cutPlane[i]->SetOrigin(bounds[0] + (bounds[1]-bounds[0])/2.,
                                        0,
                                        0);
                break;
            case 1:
                _cutPlane[i]->SetNormal(0, 1, 0);
                _cutPlane[i]->SetOrigin(0,
                                        bounds[2] + (bounds[3]-bounds[2])/2.,
                                        0);
                break;
            case 2:
            default:
                _cutPlane[i]->SetNormal(0, 0, 1);
                _cutPlane[i]->SetOrigin(0,
                                        0,
                                        bounds[4] + (bounds[5]-bounds[4])/2.);
                break;
            }
        }
        if (_cutter[i] == NULL) {
            _cutter[i] = vtkSmartPointer<vtkCutter>::New();
            _cutter[i]->SetCutFunction(_cutPlane[i]);
        }
    }

    initProp();

    if (!_pipelineInitialized) {
        vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
        if (pd && 
            pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            PrincipalPlane plane;
            double offset;
            if (_dataSet->is2D(&plane, &offset)) {
                // DataSet is a 2D point cloud
#ifdef MESH_POINT_CLOUDS
                vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                if (plane == PLANE_ZY) {
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->RotateWXYZ(90, 0, 1, 0);
                    if (offset != 0.0) {
                        trans->Translate(-offset, 0, 0);
                    }
                    mesher->SetTransform(trans);
                    _actor[1]->VisibilityOff();
                    _actor[2]->VisibilityOff();
                } else if (plane == PLANE_XZ) {
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->RotateWXYZ(-90, 1, 0, 0);
                    if (offset != 0.0) {
                        trans->Translate(0, -offset, 0);
                    }
                    mesher->SetTransform(trans);
                    _actor[0]->VisibilityOff();
                    _actor[2]->VisibilityOff();
                } else if (offset != 0.0) {
                    // XY with Z offset
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->Translate(0, 0, -offset);
                    mesher->SetTransform(trans);
                    _actor[0]->VisibilityOff();
                    _actor[1]->VisibilityOff();
                }
#ifdef USE_VTK6
                mesher->SetInputData(pd);
#else
                mesher->SetInput(pd);
#endif
                for (int i = 0; i < 3; i++) {
                    _mapper[i]->SetInputConnection(mesher->GetOutputPort());
                }
#else
                if (_splatter == NULL) {
                    _splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                }
#ifdef USE_VTK6
                _splatter->SetInputData(pd);
#else
                _splatter->SetInput(pd);
#endif
                int dims[3];
                _splatter->GetSampleDimensions(dims);
                TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                if (plane == PLANE_ZY) {
                    dims[0] = 3;
                } else if (plane == PLANE_XZ) {
                    dims[1] = 3;
                } else {
                    dims[2] = 3;
                }
                _splatter->SetSampleDimensions(dims);
                for (int i = 0; i < 3; i++) {
                    _cutter[i]->SetInputConnection(_splatter->GetOutputPort());
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_cutter[i]->GetOutputPort());
                    _mapper[i]->SetInputConnection(gf->GetOutputPort());
                }
#endif
            } else {
#ifdef MESH_POINT_CLOUDS
                // Data Set is a 3D point cloud
                // Result of Delaunay3D mesher is unstructured grid
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
                mesher->SetInputData(pd);
#else
                mesher->SetInput(pd);
#endif
                // Sample a plane within the grid bounding box
                for (int i = 0; i < 3; i++) {
                    _cutter[i]->SetInputConnection(mesher->GetOutputPort());
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_cutter[i]->GetOutputPort());
                    _mapper[i]->SetInputConnection(gf->GetOutputPort());
                }
#else
                if (_splatter == NULL) {
                    _splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                }
#ifdef USE_VTK6
                _splatter->SetInputData(pd);
#else
                _splatter->SetInput(pd);
#endif
                int dims[3];
                dims[0] = dims[1] = dims[2] = 64;
                TRACE("Generating volume with dims (%d,%d,%d) from point cloud",
                      dims[0], dims[1], dims[2]);
                _splatter->SetSampleDimensions(dims);
                for (int i = 0; i < 3; i++) {
                    _cutter[i]->SetInputConnection(_splatter->GetOutputPort());
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_cutter[i]->GetOutputPort());
                    _mapper[i]->SetInputConnection(gf->GetOutputPort());
                }
#endif
            }
        } else {
            // DataSet can be: image/volume/uniform grid, structured grid, unstructured grid, rectilinear grid, or
            // PolyData with cells other than points
            PrincipalPlane plane;
            double offset;
            if (!_dataSet->is2D(&plane, &offset)) {
                // Sample a plane within the grid bounding box
                for (int i = 0; i < 3; i++) {
#ifdef USE_VTK6
                    _cutter[i]->SetInputData(ds);
#else
                    _cutter[i]->SetInput(ds);
#endif
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
                    gf->SetInputConnection(_cutter[i]->GetOutputPort());
                    _mapper[i]->SetInputConnection(gf->GetOutputPort());
                }
            } else {
                // 2D data
                if (plane == PLANE_ZY) {
                    _actor[1]->VisibilityOff();
                    _actor[2]->VisibilityOff();
                } else if (plane == PLANE_XZ) {
                    _actor[0]->VisibilityOff();
                    _actor[2]->VisibilityOff();
                } else if (offset != 0.0) {
                    // XY with Z offset
                    _actor[0]->VisibilityOff();
                    _actor[1]->VisibilityOff();
                }
                for (int i = 0; i < 3; i++) {
                    vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                    gf->UseStripsOn();
#ifdef USE_VTK6
                    gf->SetInputData(ds);
#else
                    gf->SetInput(ds);
#endif
                    _mapper[i]->SetInputConnection(gf->GetOutputPort());
                }
            }
        }
    }

    _pipelineInitialized = true;

    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL && _borderMapper[i] == NULL) {
            _borderMapper[i] = vtkSmartPointer<vtkPolyDataMapper>::New();
#ifdef CUTPLANE_TIGHT_OUTLINE
            _outlineFilter[i] = vtkSmartPointer<vtkOutlineFilter>::New();
            _outlineFilter[i]->SetInputConnection(_mapper[i]->GetInputConnection(0, 0));
            _borderMapper[i]->SetInputConnection(_outlineFilter[i]->GetOutputPort());
#else
            _outlineSource[i] = vtkSmartPointer<vtkOutlineSource>::New();
            switch (i) {
            case 0:
                _outlineSource[i]->SetBounds(bounds[0] + (bounds[1]-bounds[0])/2.,
                                             bounds[0] + (bounds[1]-bounds[0])/2.,
                                             bounds[2], bounds[3],
                                             bounds[4], bounds[5]);
                break;
            case 1:
                _outlineSource[i]->SetBounds(bounds[0], bounds[1],
                                             bounds[2] + (bounds[3]-bounds[2])/2.,
                                             bounds[2] + (bounds[3]-bounds[2])/2.,
                                             bounds[4], bounds[5]);
                break;
            case 2:
                _outlineSource[i]->SetBounds(bounds[0], bounds[1],
                                             bounds[2], bounds[3],
                                             bounds[4] + (bounds[5]-bounds[4])/2.,
                                             bounds[4] + (bounds[5]-bounds[4])/2.);
                break;
            default:
                ;
            }
            _borderMapper[i]->SetInputConnection(_outlineSource[i]->GetOutputPort());
#endif
            _borderMapper[i]->SetResolveCoincidentTopologyToPolygonOffset();
        }
    }

    setInterpolateBeforeMapping(true);

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
        setColorMode(_colorMode);
    }

    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _actor[i]->SetMapper(_mapper[i]);
            _mapper[i]->Update();
        }
        if (_borderMapper[i] != NULL) {
            _borderActor[i]->SetMapper(_borderMapper[i]);
            _borderMapper[i]->Update();
        }
        // Only add cutter actor to assembly if geometry was
        // produced, in order to prevent messing up assembly bounds
        double bounds[6];
        _actor[i]->GetBounds(bounds);
        if (bounds[0] <= bounds[1]) {
            getAssembly()->AddPart(_actor[i]);
        }
    }
}

void Cutplane::setInterpolateBeforeMapping(bool state)
{
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _mapper[i]->SetInterpolateScalarsBeforeMapping((state ? 1 : 0));
        }
    }
}

/**
 * \brief Select a 2D slice plane from a 3D DataSet
 *
 * \param[in] axis Axis of slice plane
 * \param[in] ratio Position [0,1] of slice plane along axis
 */
void Cutplane::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if ((axis == X_AXIS &&_cutPlane[0] == NULL) ||
        (axis == Y_AXIS &&_cutPlane[1] == NULL) ||
        (axis == Z_AXIS &&_cutPlane[2] == NULL)) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    double bounds[6];
    _dataSet->getBounds(bounds);
#if 0
    if (ratio == 0.0)
        ratio = 0.001;
    if (ratio == 1.0)
        ratio = 0.999;
#endif
    switch (axis) {
    case X_AXIS:
        _cutPlane[0]->SetOrigin(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                0,
                                0);
#ifndef CUTPLANE_TIGHT_OUTLINE
        _outlineSource[0]->SetBounds(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                     bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                     bounds[2], bounds[3],
                                     bounds[4], bounds[5]);
#endif
        break;
    case Y_AXIS:
        _cutPlane[1]->SetOrigin(0,
                                bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                0);
#ifndef CUTPLANE_TIGHT_OUTLINE
        _outlineSource[1]->SetBounds(bounds[0], bounds[1],
                                     bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                     bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                     bounds[4], bounds[5]);
#endif
        break;
    case Z_AXIS:
        _cutPlane[2]->SetOrigin(0,
                                0,
                                bounds[4] + (bounds[5]-bounds[4]) * ratio);
#ifndef CUTPLANE_TIGHT_OUTLINE
        _outlineSource[2]->SetBounds(bounds[0], bounds[1],
                                     bounds[2], bounds[3],
                                     bounds[4] + (bounds[5]-bounds[4]) * ratio,
                                     bounds[4] + (bounds[5]-bounds[4]) * ratio);
#endif
        break;
    default:
        ERROR("Invalid Axis");
        return;
    }
    update();
}

void Cutplane::updateRanges(Renderer *renderer)
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

void Cutplane::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL)
        return;

    switch (mode) {
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
    default:
        ;
    }
}

void Cutplane::setColorMode(ColorMode mode,
                            const char *name, double range[2])
{
    if (_dataSet == NULL)
        return;
    DataSet::DataAttributeType type;
    int numComponents;
    if (!_dataSet->getFieldInfo(name, &type, &numComponents)) {
        ERROR("Field not found: %s", name);
        return;
    }
    setColorMode(mode, type, name, range);
}

void Cutplane::setColorMode(ColorMode mode,
                            DataSet::DataAttributeType type,
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

    if (_dataSet == NULL ||
        _mapper[0] == NULL ||
        _mapper[1] == NULL ||
        _mapper[2] == NULL)
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->SetScalarModeToUsePointFieldData();
        }
        break;
    case DataSet::CELL_DATA:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->SetScalarModeToUseCellFieldData();
        }
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (_splatter != NULL) {
        for (int i = 0; i < 3; i++) {
            _mapper[i]->SelectColorArray("SplatterValues");
        }
    } else if (name != NULL && strlen(name) > 0) {
        for (int i = 0; i < 3; i++) {
            _mapper[i]->SelectColorArray(name);
        }
    } else {
        for (int i = 0; i < 3; i++) {
            _mapper[i]->SetScalarModeToDefault();
        }
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
    case COLOR_BY_SCALAR:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOn();
        }
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    default:
        for (int i = 0; i < 3; i++) {
            _mapper[i]->ScalarVisibilityOff();
        }
        break;
    }
}

/**
 * \brief Called when the color map has been edited
 */
void Cutplane::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Cutplane::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        for (int i = 0; i < 3; i++) {
            if (_mapper[i] != NULL) {
                _mapper[i]->UseLookupTableScalarRangeOn();
                _mapper[i]->SetLookupTable(_lut);
            }
        }
        _lut->DeepCopy(cmap->getLookupTable());
        switch (_colorMode) {
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
 * \brief Turn on/off lighting of this object
 */
void Cutplane::setLighting(bool state)
{
    _lighting = state;
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL)
            _actor[i]->GetProperty()->SetLighting((state ? 1 : 0));
    }
}

/**
 * \brief Turn on/off rendering of mesh edges
 */
void Cutplane::setEdgeVisibility(bool state)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
        }
    }
}

/**
 * \brief Turn on/off rendering of outlines
 */
void Cutplane::setOutlineVisibility(bool state)
{
    for (int i = 0; i < 3; i++) {
        if (_borderActor[i] != NULL) {
            _borderActor[i]->SetVisibility((state ? 1 : 0));
        }
    }
}

/**
 * \brief Set visibility of cutplane on specified axis
 */
void Cutplane::setSliceVisibility(Axis axis, bool state)
{
    switch (axis) {
    case X_AXIS:
        if (_actor[0] != NULL)
            _actor[0]->SetVisibility((state ? 1 : 0));
        if (_borderActor[0] != NULL)
            _borderActor[0]->SetVisibility((state ? 1 : 0));
        break;
    case Y_AXIS:
        if (_actor[1] != NULL)
            _actor[1]->SetVisibility((state ? 1 : 0));
        if (_borderActor[1] != NULL)
            _borderActor[1]->SetVisibility((state ? 1 : 0));
        break;
    case Z_AXIS:
    default:
        if (_actor[2] != NULL)
            _actor[2]->SetVisibility((state ? 1 : 0));
        if (_borderActor[2] != NULL)
            _borderActor[2]->SetVisibility((state ? 1 : 0));
        break;
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Cutplane::setClippingPlanes(vtkPlaneCollection *planes)
{
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _mapper[i]->SetClippingPlanes(planes);
        }
        if (_borderMapper[i] != NULL) {
            _borderMapper[i]->SetClippingPlanes(planes);
        }
    }
}
