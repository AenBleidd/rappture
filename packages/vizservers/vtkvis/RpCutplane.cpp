/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

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

#include "RpCutplane.h"
#include "Trace.h"

#define MESH_POINT_CLOUDS

using namespace Rappture::VtkVis;

Cutplane::Cutplane() :
    VtkGraphicsObject(),
    _colorMode(COLOR_BY_SCALAR),
    _colorMap(NULL),
    _sliceAxis(Z_AXIS)
{
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

void Cutplane::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    // Mapper, actor to render color-mapped data set
    if (_mapper == NULL) {
        _mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        // Map scalars through lookup table regardless of type
        _mapper->SetColorModeToMapScalars();
        //_mapper->InterpolateScalarsBeforeMappingOn();
    }

    vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
    if (pd) {
        // DataSet is a vtkPolyData
        if (pd->GetNumberOfLines() == 0 &&
            pd->GetNumberOfPolys() == 0 &&
            pd->GetNumberOfStrips() == 0) {
            // DataSet is a point cloud
            DataSet::PrincipalPlane plane;
            double offset;
            if (_dataSet->is2D(&plane, &offset)) {
                // DataSet is a 2D point cloud
#ifdef MESH_POINT_CLOUDS
                vtkSmartPointer<vtkDelaunay2D> mesher = vtkSmartPointer<vtkDelaunay2D>::New();
                if (plane == DataSet::PLANE_ZY) {
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->RotateWXYZ(90, 0, 1, 0);
                    if (offset != 0.0) {
                        trans->Translate(-offset, 0, 0);
                    }
                    mesher->SetTransform(trans);
                    _sliceAxis = X_AXIS;
                } else if (plane == DataSet::PLANE_XZ) {
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->RotateWXYZ(-90, 1, 0, 0);
                    if (offset != 0.0) {
                        trans->Translate(0, -offset, 0);
                    }
                    mesher->SetTransform(trans);
                    _sliceAxis = Y_AXIS;
                } else if (offset != 0.0) {
                    // XY with Z offset
                    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
                    trans->Translate(0, 0, -offset);
                    mesher->SetTransform(trans);
                }
                mesher->SetInput(pd);
                _mapper->SetInputConnection(mesher->GetOutputPort());
#else
                vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                splatter->SetInput(pd);
                int dims[3];
                splatter->GetSampleDimensions(dims);
                TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                dims[2] = 3;
                splatter->SetSampleDimensions(dims);
                double bounds[6];
                splatter->Update();
                splatter->GetModelBounds(bounds);
                TRACE("Model bounds: %g %g %g %g %g %g",
                      bounds[0], bounds[1],
                      bounds[2], bounds[3],
                      bounds[4], bounds[5]);
                vtkSmartPointer<vtkExtractVOI> slicer = vtkSmartPointer<vtkExtractVOI>::New();
                slicer->SetInputConnection(splatter->GetOutputPort());
                slicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                slicer->SetSampleRate(1, 1, 1);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(slicer->GetOutputPort());
                _mapper->SetInputConnection(gf->GetOutputPort());
#endif
            } else {
#ifdef MESH_POINT_CLOUDS
                // Data Set is a 3D point cloud
                // Result of Delaunay3D mesher is unstructured grid
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
                mesher->SetInput(pd);
                // Run the mesher
                mesher->Update();
                // Get bounds of resulting grid
                double bounds[6];
                mesher->GetOutput()->GetBounds(bounds);
                // Sample a plane within the grid bounding box
                vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
                cutter->SetInputConnection(mesher->GetOutputPort());
                if (_cutPlane == NULL) {
                    _cutPlane = vtkSmartPointer<vtkPlane>::New();
                }
                _cutPlane->SetNormal(0, 0, 1);
                _cutPlane->SetOrigin(0,
                                     0,
                                     bounds[4] + (bounds[5]-bounds[4])/2.);
                cutter->SetCutFunction(_cutPlane);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(cutter->GetOutputPort());
#else
                if (_pointSplatter == NULL)
                    _pointSplatter = vtkSmartPointer<vtkGaussianSplatter>::New();
                _pointSplatter->SetInput(pd);
                int dims[3];
                _pointSplatter->GetSampleDimensions(dims);
                TRACE("Sample dims: %d %d %d", dims[0], dims[1], dims[2]);
                dims[2] = 3;
                _pointSplatter->SetSampleDimensions(dims);
                double bounds[6];
                _pointSplatter->Update();
                _pointSplatter->GetModelBounds(bounds);
                TRACE("Model bounds: %g %g %g %g %g %g",
                      bounds[0], bounds[1],
                      bounds[2], bounds[3],
                      bounds[4], bounds[5]);
                if (_volumeSlicer == NULL)
                    _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
                _volumeSlicer->SetInputConnection(_pointSplatter->GetOutputPort());
                _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, 1, 1);
                _volumeSlicer->SetSampleRate(1, 1, 1);
                vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
                gf->UseStripsOn();
                gf->SetInputConnection(_volumeSlicer->GetOutputPort());
#endif
                _mapper->SetInputConnection(gf->GetOutputPort());
             }
        } else {
            // DataSet is a vtkPolyData with lines and/or polygons
            _mapper->SetInput(ds);
        }
    } else {
        // DataSet is NOT a vtkPolyData
        // Can be: image/volume/uniform grid, structured grid, unstructured grid, rectilinear grid
        vtkSmartPointer<vtkDataSetSurfaceFilter> gf = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        gf->UseStripsOn();
        vtkImageData *imageData = vtkImageData::SafeDownCast(ds);
        if (!_dataSet->is2D() && imageData != NULL) {
            // 3D image/volume/uniform grid
            if (_volumeSlicer == NULL)
                _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
            int dims[3];
            imageData->GetDimensions(dims);
            TRACE("Image data dimensions: %d %d %d", dims[0], dims[1], dims[2]);
            _volumeSlicer->SetInput(ds);
            _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, (dims[2]-1)/2, (dims[2]-1)/2);
            _volumeSlicer->SetSampleRate(1, 1, 1);
            gf->SetInputConnection(_volumeSlicer->GetOutputPort());
        } else if (!_dataSet->is2D() && imageData == NULL) {
            // 3D structured grid, unstructured grid, or rectilinear grid
            double bounds[6];
            ds->GetBounds(bounds);
            // Sample a plane within the grid bounding box
            vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
            cutter->SetInput(ds);
            if (_cutPlane == NULL) {
                _cutPlane = vtkSmartPointer<vtkPlane>::New();
            }
            _cutPlane->SetNormal(0, 0, 1);
            _cutPlane->SetOrigin(0,
                                 0,
                                 bounds[4] + (bounds[5]-bounds[4])/2.);
            cutter->SetCutFunction(_cutPlane);
            gf->SetInputConnection(cutter->GetOutputPort());
        } else {
            // 2D data
            gf->SetInput(ds);
        }
        _mapper->SetInputConnection(gf->GetOutputPort());
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    initProp();
    getActor()->SetMapper(_mapper);
    _mapper->Update();
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

    if (_volumeSlicer == NULL &&
        _cutPlane == NULL) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    _sliceAxis = axis;

    if (_cutPlane != NULL) {
        double bounds[6];
        _dataSet->getBounds(bounds);
        switch (axis) {
        case X_AXIS:
            _cutPlane->SetNormal(1, 0, 0);
            _cutPlane->SetOrigin(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                 0,
                                 0);
            break;
        case Y_AXIS:
            _cutPlane->SetNormal(0, 1, 0);
            _cutPlane->SetOrigin(0,
                                 bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                 0);
            break;
        case Z_AXIS:
            _cutPlane->SetNormal(0, 0, 1);
            _cutPlane->SetOrigin(0,
                                 0,
                                 bounds[4] + (bounds[5]-bounds[4]) * ratio);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }
    } else {
        int dims[3];
        if (_pointSplatter != NULL) {
            _pointSplatter->GetSampleDimensions(dims);
        } else {
            vtkImageData *imageData = vtkImageData::SafeDownCast(_dataSet->getVtkDataSet());
            if (imageData == NULL) {
                ERROR("Not a volume data set");
                return;
            }
            imageData->GetDimensions(dims);
        }
        int voi[6];

        switch (axis) {
        case X_AXIS:
            voi[0] = voi[1] = (int)((dims[0]-1) * ratio);
            voi[2] = 0;
            voi[3] = dims[1]-1;
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Y_AXIS:
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = voi[3] = (int)((dims[1]-1) * ratio);
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Z_AXIS:
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = 0;
            voi[3] = dims[1]-1;
            voi[4] = voi[5] = (int)((dims[2]-1) * ratio);
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }

        _volumeSlicer->SetVOI(voi);
    }

    if (_mapper != NULL)
        _mapper->Update();
}

void Cutplane::updateRanges(bool useCumulative,
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
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
        _dataSet->getVectorRange(_vectorMagnitudeRange);
        for (int i = 0; i < 3; i++) {
            _dataSet->getVectorRange(_vectorComponentRange[i], i);
        }
    }

    // Need to update color map ranges and/or active vector field
    setColorMode(_colorMode);
}

void Cutplane::setColorMode(ColorMode mode)
{
    _colorMode = mode;
    if (_dataSet == NULL || _mapper == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    switch (mode) {
    case COLOR_BY_SCALAR: {
        _mapper->ScalarVisibilityOn();
        _mapper->SetScalarModeToDefault();
        if (_lut != NULL) {
            _lut->SetRange(_dataRange);
        }
    }
        break;
    case COLOR_BY_VECTOR_MAGNITUDE: {
        _mapper->ScalarVisibilityOn();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUsePointFieldData();
            _mapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUseCellFieldData();
            _mapper->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorMagnitudeRange);
            _lut->SetVectorModeToMagnitude();
        }
    }
        break;
    case COLOR_BY_VECTOR_X:
        _mapper->ScalarVisibilityOn();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUsePointFieldData();
            _mapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUseCellFieldData();
            _mapper->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[0]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(0);
        }
        break;
    case COLOR_BY_VECTOR_Y:
        _mapper->ScalarVisibilityOn();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUsePointFieldData();
            _mapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUseCellFieldData();
            _mapper->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[1]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(1);
        }
        break;
    case COLOR_BY_VECTOR_Z:
        _mapper->ScalarVisibilityOn();
        if (ds->GetPointData() != NULL &&
            ds->GetPointData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUsePointFieldData();
            _mapper->SelectColorArray(ds->GetPointData()->GetVectors()->GetName());
        } else if (ds->GetCellData() != NULL &&
                   ds->GetCellData()->GetVectors() != NULL) {
            _mapper->SetScalarModeToUseCellFieldData();
            _mapper->SelectColorArray(ds->GetCellData()->GetVectors()->GetName());
        }
        if (_lut != NULL) {
            _lut->SetRange(_vectorComponentRange[2]);
            _lut->SetVectorModeToComponent();
            _lut->SetVectorComponent(2);
        }
        break;
    default:
        _mapper->ScalarVisibilityOff();
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
        if (_mapper != NULL) {
            _mapper->UseLookupTableScalarRangeOn();
            _mapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());

    switch (_colorMode) {
    case COLOR_BY_SCALAR:
        _lut->SetRange(_dataRange);
        break;
    case COLOR_BY_VECTOR_MAGNITUDE:
        _lut->SetVectorModeToMagnitude();
        _lut->SetRange(_vectorMagnitudeRange);
        break;
    case COLOR_BY_VECTOR_X:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(0);
        _lut->SetRange(_vectorComponentRange[0]);
        break;
    case COLOR_BY_VECTOR_Y:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(1);
        _lut->SetRange(_vectorComponentRange[1]);
        break;
    case COLOR_BY_VECTOR_Z:
        _lut->SetVectorModeToComponent();
        _lut->SetVectorComponent(2);
        _lut->SetRange(_vectorComponentRange[2]);
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
void Cutplane::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}
