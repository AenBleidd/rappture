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
#include <vtkCellDataToPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkGaussianSplatter.h>
#include <vtkExtractVOI.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkVertexGlyphFilter.h>

#include "Warp.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

Warp::Warp() :
    GraphicsObject(),
    _warpScale(1.0),
    _cloudStyle(CLOUD_MESH),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

Warp::~Warp()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Warp for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Warp with NULL DataSet");
#endif
}

void Warp::setDataSet(DataSet *dataSet,
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
 * \brief Internal method to set up pipeline after a state change
 */
void Warp::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_warp == NULL) {
        _warp = vtkSmartPointer<vtkWarpVector>::New();
    }

    // Mapper, actor to render color-mapped data set
    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        // Map scalars through lookup table regardless of type
        _mapper->SetColorModeToMapScalars();
    }

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;
    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetVectors() == NULL) {
        TRACE("No vector point data in dataset %s", _dataSet->getName().c_str());
        if (ds->GetCellData() != NULL &&
            ds->GetCellData()->GetVectors() != NULL) {
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
            USER_ERROR("No vector field was found in the data set");
            return;
        }
    }

    if (_dataSet->isCloud()) {
        // DataSet is a point cloud
        if (_cloudStyle == CLOUD_POINTS) {
            vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
            vgf->SetInputData(ds);
#else
            vgf->SetInput(ds);
#endif
            vtkAlgorithmOutput *warpOutput = initWarp(vgf->GetOutputPort());
            _mapper->SetInputConnection(warpOutput);
        } else {
            PrincipalPlane plane;
            double offset;
            if (_dataSet->is2D(&plane, &offset)) {
                //_cloudStyle == CLOUD_MESH
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
                vtkAlgorithmOutput *warpOutput = initWarp(mesher->GetOutputPort());
                _mapper->SetInputConnection(warpOutput);
            } else {
                // Data Set is a 3D point cloud and _cloudStyle == CLOUD_MESH
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
                mesher->SetInputData(ds);
#else
                mesher->SetInput(ds);
#endif
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->SetInputConnection(mesher->GetOutputPort());
                vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                _mapper->SetInputConnection(warpOutput);
            }
        }
    } else if (vtkPolyData::SafeDownCast(ds) != NULL) {
        // DataSet is a vtkPolyData with lines and/or polygons
        vtkAlgorithmOutput *warpOutput = initWarp(ds);
        if (warpOutput != NULL) {
            _mapper->SetInputConnection(warpOutput);
        } else {
#ifdef USE_VTK6
            _mapper->SetInputData(vtkPolyData::SafeDownCast(ds));
#else
            _mapper->SetInput(vtkPolyData::SafeDownCast(ds));
#endif
        }
    } else {
        TRACE("Generating surface for data set");
        // DataSet is NOT a vtkPolyData
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
#ifdef USE_VTK6
        gf->SetInputData(ds);
#else
        gf->SetInput(ds);
#endif
        vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
        _mapper->SetInputConnection(warpOutput);
    }

    setInterpolateBeforeMapping(true);

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

    initProp();
    getActor()->SetMapper(_mapper);
    _mapper->Update();
}

void Warp::setCloudStyle(CloudStyle style)
{
    if (style != _cloudStyle) {
        _cloudStyle = style;
        if (_dataSet != NULL) {
            update();
        }
    }
}

void Warp::setInterpolateBeforeMapping(bool state)
{
    if (_mapper != NULL) {
        _mapper->SetInterpolateScalarsBeforeMapping((state ? 1 : 0));
    }
}

vtkAlgorithmOutput *Warp::initWarp(vtkAlgorithmOutput *input)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        return input;
    } else if (input == NULL) {
        ERROR("NULL input");
        return input;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpVector>::New();
        _warp->SetScaleFactor(_warpScale);
        _warp->SetInputConnection(input);
        return _warp->GetOutputPort();
    }
}

vtkAlgorithmOutput *Warp::initWarp(vtkDataSet *ds)
{
    TRACE("Warp scale: %g", _warpScale);
    if (_warpScale == 0.0) {
        _warp = NULL;
        return NULL;
    } else if (ds == NULL) {
        ERROR("NULL input");
        return NULL;
    } else {
        if (_warp == NULL)
            _warp = vtkSmartPointer<vtkWarpVector>::New();
        _warp->SetScaleFactor(_warpScale);
#ifdef USE_VTK6
        _warp->SetInputData(ds);
#else
        _warp->SetInput(ds);
#endif
        return _warp->GetOutputPort();
    }
}

/**
 * \brief Controls relative scaling of deformation
 */
void Warp::setWarpScale(double scale)
{
    if (_warpScale == scale)
        return;

    _warpScale = scale;
    if (_warp == NULL) {
        vtkAlgorithmOutput *warpOutput = initWarp(_mapper->GetInputConnection(0, 0));
        _mapper->SetInputConnection(warpOutput);
    } else if (scale == 0.0) {
        vtkAlgorithmOutput *warpInput = _warp->GetInputConnection(0, 0);
        _mapper->SetInputConnection(warpInput);
        _warp = NULL;
    } else {
        _warp->SetScaleFactor(_warpScale);
    }

    if (_mapper != NULL)
        _mapper->Update();
}

void Warp::updateRanges(Renderer *renderer)
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

void Warp::setColorMode(ColorMode mode)
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
    case COLOR_CONSTANT:
    default:
        setColorMode(mode, DataSet::POINT_DATA, NULL, NULL);
        break;
    }
}

void Warp::setColorMode(ColorMode mode,
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

void Warp::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
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

    if (_dataSet == NULL || _mapper == NULL)
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        _mapper->SetScalarModeToUsePointFieldData();
        break;
    case DataSet::CELL_DATA:
        _mapper->SetScalarModeToUseCellFieldData();
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (name != NULL && strlen(name) > 0) {
        _mapper->SelectColorArray(name);
    } else {
        _mapper->SetScalarModeToDefault();
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
        _mapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _mapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        _mapper->ScalarVisibilityOff();
        break;
    }
}

/**
 * \brief Called when the color map has been edited
 */
void Warp::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Warp::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_mapper != NULL) {
            _mapper->UseLookupTableScalarRangeOn();
            _mapper->SetLookupTable(_lut);
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
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Warp::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}
