/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2012, Purdue Research Foundation
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
#include <vtkDataSetSurfaceFilter.h>

#include "RpWarp.h"
#include "RpVtkRenderer.h"
#include "Trace.h"

#define MESH_POINT_CLOUDS

using namespace Rappture::VtkVis;

Warp::Warp() :
    VtkGraphicsObject(),
    _warpScale(1.0),
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
    if (_dsMapper == NULL) {
        _dsMapper = vtkSmartPointer<vtkDataSetMapper>::New();
        // Map scalars through lookup table regardless of type
        _dsMapper->SetColorModeToMapScalars();
        //_dsMapper->InterpolateScalarsBeforeMappingOn();
    }

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;
    if (ds->GetPointData() == NULL ||
	ds->GetPointData()->GetVectors() == NULL) {
	TRACE("No vector point data in dataset %s", _dataSet->getName().c_str());
	if (ds->GetCellData() != NULL &&
	    ds->GetCellData()->GetVectors() != NULL) {
	    cellToPtData = 
		vtkSmartPointer<vtkCellDataToPointData>::New();
	    cellToPtData->SetInput(ds);
	    //cellToPtData->PassCellDataOn();
	    cellToPtData->Update();
	    ds = cellToPtData->GetOutput();
	} else {
	    ERROR("No vector data in dataset %s", _dataSet->getName().c_str());
	    return;
	}
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            PrincipalPlane plane;
            double offset;
            if (_dataSet->is2D(&plane, &offset)) {
#ifdef MESH_POINT_CLOUDS
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
                mesher->SetInput(pd);
		vtkAlgorithmOutput *warpOutput = initWarp(mesher->GetOutputPort());
                _dsMapper->SetInputConnection(warpOutput);
#else
                vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                vtkSmartPointer<vtkExtractVOI> slicer = vtkSmartPointer<vtkExtractVOI>::New();
                splatter->SetInput(pd);
                int dims[3];
                splatter->GetSampleDimensions(dims);
                TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                if (plane == PLANE_ZY) {
                    dims[0] = 3;
                    slicer->SetVOI(1, 1, 0, dims[1]-1, 0, dims[1]-1);
                } else if (plane == PLANE_XZ) {
                    dims[1] = 3;
                    slicer->SetVOI(0, dims[0]-1, 1, 1, 0, dims[2]-1);
                } else {
                    dims[2] = 3;
                    slicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                }
                splatter->SetSampleDimensions(dims);
                double bounds[6];
                splatter->Update();
                splatter->GetModelBounds(bounds);
                TRACE("Model bounds: %g %g %g %g %g %g",
                      bounds[0], bounds[1],
                      bounds[2], bounds[3],
                      bounds[4], bounds[5]);
                slicer->SetInputConnection(splatter->GetOutputPort());
                slicer->SetSampleRate(1, 1, 1);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(slicer->GetOutputPort());
		vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                _dsMapper->SetInputConnection(warpOutput);
#endif
            } else {
		// Data Set is a 3D point cloud
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
		gf->SetInputConnection(mesher->GetOutputPort());
		vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
                _dsMapper->SetInputConnection(warpOutput);
             }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
	    vtkAlgorithmOutput *warpOutput = initWarp(pd);
            _dsMapper->SetInput(ds);
	    if (warpOutput != NULL) {
		_dsMapper->SetInputConnection(warpOutput);
	    } else {
		_dsMapper->SetInput(pd);
	    }
        }
    } else {
        TRACE("Generating surface for data set");
        // DataSet is NOT a vtkPolyData
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->SetInput(ds);
	vtkAlgorithmOutput *warpOutput = initWarp(gf->GetOutputPort());
        _dsMapper->SetInputConnection(warpOutput);
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    setColorMode(_colorMode);

    initProp();
    getActor()->SetMapper(_dsMapper);
    _dsMapper->Update();
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
        _warp->SetInput(ds);
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
        vtkAlgorithmOutput *warpOutput = initWarp(_dsMapper->GetInputConnection(0, 0));
        _dsMapper->SetInputConnection(warpOutput);
    } else if (scale == 0.0) {
        vtkAlgorithmOutput *warpInput = _warp->GetInputConnection(0, 0);
        _dsMapper->SetInputConnection(warpInput);
        _warp = NULL;
    } else {
        _warp->SetScaleFactor(_warpScale);
    }

    if (_dsMapper != NULL)
        _dsMapper->Update();
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

    if (_dataSet == NULL || _dsMapper == NULL)
        return;

    switch (type) {
    case DataSet::POINT_DATA:
        _dsMapper->SetScalarModeToUsePointFieldData();
        break;
    case DataSet::CELL_DATA:
        _dsMapper->SetScalarModeToUseCellFieldData();
        break;
    default:
        ERROR("Unsupported DataAttributeType: %d", type);
        return;
    }

    if (name != NULL && strlen(name) > 0) {
        _dsMapper->SelectColorArray(name);
    } else {
        _dsMapper->SetScalarModeToDefault();
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
        _dsMapper->ScalarVisibilityOn();
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _dsMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToMagnitude();
        }
        break;
    case COLOR_BY_VECTOR_X:
        _dsMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _dsMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _dsMapper->ScalarVisibilityOn();
        if (_lut != NULL) {
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    case COLOR_CONSTANT:
    default:
        _dsMapper->ScalarVisibilityOff();
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
        if (_dsMapper != NULL) {
            _dsMapper->UseLookupTableScalarRangeOn();
            _dsMapper->SetLookupTable(_lut);
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
    if (_dsMapper != NULL) {
        _dsMapper->SetClippingPlanes(planes);
    }
}
