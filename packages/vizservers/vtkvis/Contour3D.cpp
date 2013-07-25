/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkVersion.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkUnstructuredGrid.h>
#include <vtkProperty.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>

#include "Contour3D.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

Contour3D::Contour3D(int numContours) :
    GraphicsObject(),
    _numContours(numContours),
    _pipelineInitialized(false),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

Contour3D::Contour3D(const std::vector<double>& contours) :
    GraphicsObject(),
    _numContours(contours.size()),
    _contours(contours),
    _pipelineInitialized(false),
    _colorMap(NULL),
    _colorMode(COLOR_BY_SCALAR),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL)
{
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

Contour3D::~Contour3D()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Contour3D for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Contour3D with NULL DataSet");
#endif
}

void Contour3D::setDataSet(DataSet *dataSet,
                           Renderer *renderer)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        _renderer = renderer;

        if (renderer->getUseCumulativeRange()) {
            renderer->getCumulativeDataRange(_dataRange,
                                             _dataSet->getActiveScalarsName(),
                                             1);
            const char *activeVectors = _dataSet->getActiveVectorsName();
            if (activeVectors != NULL) {
                renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                                 _dataSet->getActiveVectorsName(),
                                                 3);
                for (int i = 0; i < 3; i++) {
                    renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                     _dataSet->getActiveVectorsName(),
                                                     3, i);
                }
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
 * \brief Internal method to re-compute contours after a state change
 */
void Contour3D::update()
{
    if (_dataSet == NULL) {
        return;
    }
    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_dataSet->is2D()) {
        USER_ERROR("Cannot create isosurface(s) since the data set is not 3D");
        return;
    }

    // Contour filter to generate isosurfaces
    if (_contourFilter == NULL) {
        _contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    }

    if (!_pipelineInitialized) {
        vtkSmartPointer<vtkCellDataToPointData> cellToPtData;

        if (ds->GetPointData() == NULL ||
            ds->GetPointData()->GetScalars() == NULL) {
            TRACE("No scalar point data in dataset %s", _dataSet->getName().c_str());
            if (ds->GetCellData() != NULL &&
                ds->GetCellData()->GetScalars() != NULL) {
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
                USER_ERROR("No scalar field was found in the data set.");
            }
        }

        if (_dataSet->isCloud()) {
            // DataSet is a point cloud
            // Generate a 3D unstructured grid
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            _contourFilter->SetInputConnection(mesher->GetOutputPort());
        } else {
            // DataSet is 3D with cells.  If DataSet is a surface
            // (e.g. polydata or ugrid with 2D cells), we will 
            // generate lines instead of surfaces
#ifdef USE_VTK6
            _contourFilter->SetInputData(ds);
#else
            _contourFilter->SetInput(ds);
#endif
        }
    }

    _pipelineInitialized = true;

    _contourFilter->ComputeNormalsOff();
    _contourFilter->ComputeGradientsOff();

    // Speed up multiple contour computation at cost of extra memory use
    if (_numContours > 1) {
        _contourFilter->UseScalarTreeOn();
    } else {
        _contourFilter->UseScalarTreeOff();
    }

    _contourFilter->SetNumberOfContours(_numContours);

    if (_contours.empty()) {
        // Evenly spaced isovalues
        TRACE("Setting %d contours between %g and %g", _numContours, _dataRange[0], _dataRange[1]);
        _contourFilter->GenerateValues(_numContours, _dataRange[0], _dataRange[1]);
    } else {
        // User-supplied isovalues
        for (int i = 0; i < _numContours; i++) {
            _contourFilter->SetValue(i, _contours[i]);
        }
    }

    initProp();

    if (_normalsGenerator == NULL) {
        _normalsGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
        _normalsGenerator->SetInputConnection(_contourFilter->GetOutputPort());
        _normalsGenerator->SetFeatureAngle(90.);
        _normalsGenerator->AutoOrientNormalsOff();
        _normalsGenerator->ComputePointNormalsOn();
    }

    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _mapper->SetResolveCoincidentTopologyToPolygonOffset();
        _mapper->SetInputConnection(_normalsGenerator->GetOutputPort());
        _mapper->ScalarVisibilityOff();
        //_mapper->SetColorModeToMapScalars();
        getActor()->SetMapper(_mapper);
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
        setColorMode(_colorMode);
    }

    _mapper->Update();
    TRACE("Contour output %d polys, %d strips",
          _contourFilter->GetOutput()->GetNumberOfPolys(),
          _contourFilter->GetOutput()->GetNumberOfStrips());
}

void Contour3D::updateRanges(Renderer *renderer)
{
    if (_dataSet == NULL) {
        ERROR("called before setDataSet");
        return;
    }

    if (renderer->getUseCumulativeRange()) {
        renderer->getCumulativeDataRange(_dataRange,
                                         _dataSet->getActiveScalarsName(),
                                         1);
        const char *activeVectors = _dataSet->getActiveVectorsName();
        if (activeVectors != NULL) {
            renderer->getCumulativeDataRange(_vectorMagnitudeRange,
                                             _dataSet->getActiveVectorsName(),
                                             3);
            for (int i = 0; i < 3; i++) {
                renderer->getCumulativeDataRange(_vectorComponentRange[i],
                                                 _dataSet->getActiveVectorsName(),
                                                 3, i);
            }
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

    if (_contours.empty() && _numContours > 0) {
        // Contour isovalues need to be recomputed
        update();
    }
}

void Contour3D::setContourField(const char *name)
{
    if (_contourFilter != NULL) {
        _contourFilter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, name);
        update();
    }
}

void Contour3D::setColorMode(ColorMode mode)
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

void Contour3D::setColorMode(ColorMode mode,
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

void Contour3D::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
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
void Contour3D::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void Contour3D::setColorMap(ColorMap *cmap)
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
 * \brief Specify number of evenly spaced isosurfaces to render
 *
 * Will override any existing contours
 */
void Contour3D::setNumContours(int numContours)
{
    _contours.clear();
    _numContours = numContours;

    update();
}

/**
 * \brief Specify an explicit list of contour isovalues
 *
 * Will override any existing contours
 */
void Contour3D::setContourList(const std::vector<double>& contours)
{
    _contours = contours;
    _numContours = (int)_contours.size();

    update();
}

/**
 * \brief Get the number of contours
 */
int Contour3D::getNumContours() const
{
    return _numContours;
}

/**
 * \brief Get the contour list (may be empty if number of contours
 * was specified in place of a list)
 */
const std::vector<double>& Contour3D::getContourList() const
{
    return _contours;
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Contour3D::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}
