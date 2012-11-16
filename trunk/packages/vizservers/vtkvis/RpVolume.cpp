/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 6)
#define USE_VTK6
#endif
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkImageData.h>
#include <vtkVolumeProperty.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeTextureMapper3D.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellType.h>
#include <vtkUnstructuredGridVolumeMapper.h>
#include <vtkUnstructuredGridVolumeRayCastMapper.h>
#include <vtkProjectedTetrahedraMapper.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkGaussianSplatter.h>

#include "RpVolume.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

Volume::Volume() :
    VtkGraphicsObject(),
    _colorMap(NULL)
{
}

Volume::~Volume()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting Volume for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting Volume with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize a VTK Prop to render the Volume
 */
void Volume::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkVolume>::New();
        getVolume()->GetProperty()->SetInterpolationTypeToLinear();
    }
}

/**
 * \brief Get the voxel dimensions (if the volume is not a
 * uniform grid, unit spacing is returned)
 */
void Volume::getSpacing(double spacing[3])
{
    spacing[0] = spacing[1] = spacing[2] = 1.0;
    if (_dataSet == NULL)
        return;
    vtkDataSet *ds = _dataSet->getVtkDataSet();
    if (ds != NULL && vtkImageData::SafeDownCast(ds) != NULL) {
        vtkImageData::SafeDownCast(ds)->GetSpacing(spacing);
    } else if (ds != NULL && vtkVolumeMapper::SafeDownCast(_volumeMapper) != NULL) {
        vtkImageData *imgData = vtkVolumeMapper::SafeDownCast(_volumeMapper)->GetInput();
        if (imgData != NULL) {
            imgData->GetSpacing(spacing);
        }
    }
}

/**
 * \brief Get the average voxel dimension (if the volume is not a
 * uniform grid, unit spacing is returned)
 */
double Volume::getAverageSpacing()
{
    double spacing[3];
    getSpacing(spacing);
    return (spacing[0] + spacing[1] + spacing[2]) * 0.333;
}

/**
 * \brief Internal method to set up pipeline after a state change
 */
void Volume::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_dataSet->is2D()) {
        ERROR("Volume requires a 3D DataSet");
        _dataSet = NULL;
        return;
    }

    if (vtkImageData::SafeDownCast(ds) != NULL) {
        // Image data required for these mappers
#ifdef USE_GPU_RAYCAST_MAPPER
        _volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->AutoAdjustSampleDistancesOff();
#else
        _volumeMapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
#endif
#ifdef USE_VTK6
#ifdef USE_GPU_RAYCAST_MAPPER
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->SetInputData(ds);
#else
        vtkVolumeTextureMapper3D::SafeDownCast(_volumeMapper)->SetInputData(ds);
#endif
#else
        _volumeMapper->SetInput(ds);
#endif
        vtkVolumeMapper::SafeDownCast(_volumeMapper)->SetBlendModeToComposite();
    } else if (vtkUnstructuredGrid::SafeDownCast(ds) != NULL) {
        vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(ds);
        // DataSet is unstructured grid
        // Only works if cells are all tetrahedra
        _volumeMapper = vtkSmartPointer<vtkProjectedTetrahedraMapper>::New();
        // Software raycast rendering - requires all tetrahedra
        //_volumeMapper = vtkSmartPointer<vtkUnstructuredGridVolumeRayCastMapper>::New();
        if (ugrid->GetCellType(0) == VTK_TETRA &&
            ugrid->IsHomogeneous()) {
#ifdef USE_VTK6
            vtkProjectedTetrahedraMapper::SafeDownCast(_volumeMapper)->SetInputData(ds);
#else
            _volumeMapper->SetInput(ds);
#endif
        } else {
            // Decompose to tetrahedra
            vtkSmartPointer<vtkDataSetTriangleFilter> filter = 
                vtkSmartPointer<vtkDataSetTriangleFilter>::New();
#ifdef USE_VTK6
            filter->SetInputData(ugrid);
#else
            filter->SetInput(ugrid);
#endif
            filter->TetrahedraOnlyOn();
            _volumeMapper->SetInputConnection(filter->GetOutputPort());
        }

        vtkUnstructuredGridVolumeMapper::SafeDownCast(_volumeMapper)->
            SetBlendModeToComposite();
    } else if (vtkPolyData::SafeDownCast(ds) != NULL &&
               vtkPolyData::SafeDownCast(ds)->GetNumberOfLines() == 0 &&
               vtkPolyData::SafeDownCast(ds)->GetNumberOfPolys() == 0 &&
               vtkPolyData::SafeDownCast(ds)->GetNumberOfStrips() == 0 ) {
        // DataSet is a 3D point cloud
        vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
#ifdef USE_VTK6
        splatter->SetInputData(ds);
#else
        splatter->SetInput(ds);
#endif
        int dims[3];
        dims[0] = dims[1] = dims[2] = 64;
        TRACE("Generating volume with dims (%d,%d,%d) from point cloud",
              dims[0], dims[1], dims[2]);
        splatter->SetSampleDimensions(dims);
#ifdef USE_GPU_RAYCAST_MAPPER
        _volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->AutoAdjustSampleDistancesOff();
#else
        _volumeMapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
#endif
        _volumeMapper->SetInputConnection(splatter->GetOutputPort());
        vtkVolumeMapper::SafeDownCast(_volumeMapper)->SetBlendModeToComposite();
    } else {
        ERROR("Unsupported DataSet type: %s", _dataSet->getVtkType());
        _dataSet = NULL;
        return;
    }

    TRACE("Using mapper type: %s", _volumeMapper->GetClassName());

    initProp();

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        WARN("No scalar point data in dataset %s", _dataSet->getName().c_str());
    }

    if (_colorMap == NULL) {
        setColorMap(ColorMap::getVolumeDefault());
    }

    setSampleDistance(getAverageSpacing());

    getVolume()->SetMapper(_volumeMapper);
    _volumeMapper->Update();
}

void Volume::updateRanges(Renderer *renderer)
{
    VtkGraphicsObject::updateRanges(renderer);

    if (getVolume() != NULL) {
        getVolume()->GetProperty()->SetColor(_colorMap->getColorTransferFunction(_dataRange));
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange));
    }
}

void Volume::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Assign a color map (transfer function) to use in rendering the Volume
 */
void Volume::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;

    if (getVolume() != NULL) {
        getVolume()->GetProperty()->SetColor(_colorMap->getColorTransferFunction(_dataRange));
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange));
    }
}

/**
 * \brief Return the ColorMap assigned to this Volume
 */
ColorMap *Volume::getColorMap()
{
    return _colorMap;
}

/**
 * \brief Set opacity scaling used to render the Volume
 */
void Volume::setOpacity(double opacity)
{
    _opacity = opacity;
    // FIXME: There isn't really a good opacity scaling option that works
    // across the different mappers/algorithms.  This only works with the
    // 3D texture mapper, not the GPU raycast mapper
    if (getVolume() != NULL) {
        if (opacity < 1.0e-6)
            opacity = 1.0e-6;
        getVolume()->GetProperty()->SetScalarOpacityUnitDistance(1.0/opacity);
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void Volume::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_volumeMapper != NULL) {
        _volumeMapper->SetClippingPlanes(planes);
    }
}

/**
 * \brief Set distance between samples along rays
 */
void Volume::setSampleDistance(float d)
{
    TRACE("Sample distance: %g", d);
    if (_volumeMapper != NULL &&
#ifdef USE_GPU_RAYCAST_MAPPER
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper) != NULL) {
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->SetSampleDistance(d);
#else
        vtkVolumeTextureMapper3D::SafeDownCast(_volumeMapper) != NULL) {
        vtkVolumeTextureMapper3D::SafeDownCast(_volumeMapper)->SetSampleDistance(d);
#endif
    }
}
