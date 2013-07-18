/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkLookupTable.h>

#include "PolyData.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

PolyData::PolyData() :
    GraphicsObject(),
    _colorMap(NULL),
    _colorMode(COLOR_CONSTANT),
    _colorFieldType(DataSet::POINT_DATA),
    _renderer(NULL),
    _cloudStyle(CLOUD_MESH)
{
    _colorFieldRange[0] = DBL_MAX;
    _colorFieldRange[1] = -DBL_MAX;
}

PolyData::~PolyData()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting PolyData for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting PolyData with NULL DataSet");
#endif
}

void PolyData::setDataSet(DataSet *dataSet,
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
void PolyData::update()
{
    if (_dataSet == NULL) {
        return;
    }

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        _mapper->SetResolveCoincidentTopologyToPolygonOffset();
        // If there are color scalars, use them without lookup table (if scalar visibility is on)
        _mapper->SetColorModeToDefault();
        // Use Point data if available, else cell data
        _mapper->SetScalarModeToDefault();
        _mapper->ScalarVisibilityOff();
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        TRACE("Points: %d Verts: %d Lines: %d Polys: %d Strips: %d",
              pd->GetNumberOfPoints(),
              pd->GetNumberOfVerts(),
              pd->GetNumberOfLines(),
              pd->GetNumberOfPolys(),
              pd->GetNumberOfStrips());
    }
    bool hasNormals = false;
    if ((ds->GetPointData() != NULL &&
         ds->GetPointData()->GetNormals() != NULL) ||
        (ds->GetCellData() != NULL &&
         ds->GetCellData()->GetNormals() != NULL)) {
        hasNormals = true;
    }

    if (_dataSet->isCloud()) {
        // DataSet is a point cloud
        PrincipalPlane plane;
        double offset;
        if (_cloudStyle == CLOUD_POINTS ||
            _dataSet->numDimensions() < 2 || ds->GetNumberOfPoints() < 3) { // 0D or 1D or not enough points to mesh
            vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
            vgf->SetInputData(ds);
#else
            vgf->SetInput(ds);
#endif
            _mapper->SetInputConnection(vgf->GetOutputPort());
        } else if (_dataSet->is2D(&plane, &offset)) {
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
            mesher->ReleaseDataFlagOn();
            mesher->Update();
            vtkPolyData *outpd = mesher->GetOutput();
            TRACE("Delaunay2D Verts: %d Lines: %d Polys: %d Strips: %d",
                  outpd->GetNumberOfVerts(),
                  outpd->GetNumberOfLines(),
                  outpd->GetNumberOfPolys(),
                  outpd->GetNumberOfStrips());
            if (outpd->GetNumberOfPolys() == 0) {
                WARN("Delaunay2D mesher failed");
                vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
                vgf->SetInputData(ds);
#else
                vgf->SetInput(ds);
#endif
                _mapper->SetInputConnection(vgf->GetOutputPort());
            } else {
                vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
                normalFilter->SetInputConnection(mesher->GetOutputPort());
                _mapper->SetInputConnection(normalFilter->GetOutputPort());
            }
        } else {
            vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
            mesher->SetInputData(ds);
#else
            mesher->SetInput(ds);
#endif
            mesher->ReleaseDataFlagOn();
            mesher->Update();
            vtkUnstructuredGrid *grid = mesher->GetOutput();
            TRACE("Delaunay3D Cells: %d",
                  grid->GetNumberOfCells());
            if (grid->GetNumberOfCells() == 0) {
                WARN("Delaunay3D mesher failed");
                vtkSmartPointer<vtkVertexGlyphFilter> vgf = vtkSmartPointer<vtkVertexGlyphFilter>::New();
#ifdef USE_VTK6
                vgf->SetInputData(ds);
#else
                vgf->SetInput(ds);
#endif
                _mapper->SetInputConnection(vgf->GetOutputPort());
            } else {
                // Delaunay3D returns an UnstructuredGrid, so feed it 
                // through a surface filter to get the grid boundary 
                // as a PolyData
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->ReleaseDataFlagOn();
                gf->SetInputConnection(mesher->GetOutputPort());
                vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
                normalFilter->SetInputConnection(gf->GetOutputPort());
                _mapper->SetInputConnection(normalFilter->GetOutputPort());
            }
        }
    } else if (pd) {
        // DataSet is a vtkPolyData with cells
        if (hasNormals) {
#ifdef USE_VTK6
            _mapper->SetInputData(pd);
#else
            _mapper->SetInput(pd);
#endif
        } else {
            vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
#ifdef USE_VTK6
            normalFilter->SetInputData(pd);
#else
            normalFilter->SetInput(pd);
#endif
            _mapper->SetInputConnection(normalFilter->GetOutputPort());
        }
    } else {
        // DataSet is NOT a vtkPolyData
        TRACE("DataSet is not a PolyData");
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->UseStripsOn();
        gf->ReleaseDataFlagOn();
#ifdef USE_VTK6
        gf->SetInputData(ds);
#else
        gf->SetInput(ds);
#endif
        vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
        normalFilter->SetInputConnection(gf->GetOutputPort());
        _mapper->SetInputConnection(normalFilter->GetOutputPort());
    }

    setInterpolateBeforeMapping(false);

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
        if (_dataSet->getActiveScalarsName() != NULL) {
            int numComp;
            _dataSet->getFieldInfo(_dataSet->getActiveScalarsName(),
                                   _dataSet->getActiveScalarsType(),
                                   &numComp);
            if (numComp > 1) {
                setColorMode(COLOR_BY_SCALAR);
            } else {
                setColorMode(_colorMode);
            }
        } else {
            setColorMode(_colorMode);
        }
    } else {
        double *rangePtr = _colorFieldRange;
        if (_colorFieldRange[0] > _colorFieldRange[1]) {
            rangePtr = NULL;
        }
        setColorMode(_colorMode, _colorFieldType, _colorFieldName.c_str(), rangePtr);
    }

    initProp();
    getActor()->SetMapper(_mapper);
    _mapper->Update();
#ifdef WANT_TRACE
    double *b = getBounds();
    TRACE("bounds: %g %g %g %g %g %g", b[0], b[1], b[2], b[3], b[4], b[5]);
#endif
}

void PolyData::setInterpolateBeforeMapping(bool state)
{
    if (_mapper != NULL) {
        _mapper->SetInterpolateScalarsBeforeMapping((state ? 1 : 0));
    }
}

void PolyData::updateRanges(Renderer *renderer)
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

void PolyData::setColorMode(ColorMode mode)
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

void PolyData::setColorMode(ColorMode mode,
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

void PolyData::setColorMode(ColorMode mode, DataSet::DataAttributeType type,
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
    case DataSet::FIELD_DATA:
        _mapper->SetScalarModeToUseFieldData();
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
void PolyData::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void PolyData::setColorMap(ColorMap *cmap)
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
void PolyData::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}

void PolyData::setCloudStyle(CloudStyle style)
{
    if (style != _cloudStyle) {
        _cloudStyle = style;
        if (_dataSet != NULL) {
            update();
        }
    }
}
