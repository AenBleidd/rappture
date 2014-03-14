/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkImageData.h>
#include <vtkProbeFilter.h>
#include <vtkVolumeProperty.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolumeTextureMapper3D.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkCellType.h>
#include <vtkUnstructuredGridVolumeMapper.h>
#include <vtkUnstructuredGridVolumeRayCastMapper.h>
#include <vtkProjectedTetrahedraMapper.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkGaussianSplatter.h>

#include "Volume.h"
#include "Trace.h"

using namespace VtkVis;

Volume::Volume() :
    GraphicsObject(),
    _useUgridMapper(false),
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
    TRACE("Spacing: %g %g %g", spacing[0], spacing[1], spacing[2]);
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
        USER_ERROR("Volume rendering requires a 3D data set");
        _dataSet = NULL;
        return;
    }

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        USER_ERROR("No scalar field was found in the data set");
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
    } else if (_dataSet->isCloud()) {
        // DataSet is a 3D point cloud
        vtkSmartPointer<vtkGaussianSplatter> splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
#ifdef USE_VTK6
        splatter->SetInputData(ds);
#else
        splatter->SetInput(ds);
#endif
        int dims[3];
        dims[0] = dims[1] = dims[2] = 64;
        if (vtkStructuredGrid::SafeDownCast(ds) != NULL) {
            vtkStructuredGrid::SafeDownCast(ds)->GetDimensions(dims);
        } else if (vtkRectilinearGrid::SafeDownCast(ds) != NULL) {
            vtkRectilinearGrid::SafeDownCast(ds)->GetDimensions(dims);
        }
        TRACE("Generating volume with dims (%d,%d,%d) from %d points",
              dims[0], dims[1], dims[2], ds->GetNumberOfPoints());
        splatter->SetSampleDimensions(dims);
        splatter->Update();
        TRACE("Done generating volume");
#ifdef USE_GPU_RAYCAST_MAPPER
        _volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->AutoAdjustSampleDistancesOff();
#else
        _volumeMapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
#endif
        _volumeMapper->SetInputConnection(splatter->GetOutputPort());
        vtkVolumeMapper::SafeDownCast(_volumeMapper)->SetBlendModeToComposite();
    } else if (vtkUnstructuredGrid::SafeDownCast(ds) == NULL || !_useUgridMapper) {
        // (Slow) Resample using ProbeFilter
        double bounds[6];
        ds->GetBounds(bounds);
        double xLen = bounds[1] - bounds[0];
        double yLen = bounds[3] - bounds[2];
        double zLen = bounds[5] - bounds[4];

        int dims[3];
        dims[0] = dims[1] = dims[2] = 64;
        if (xLen == 0.0) dims[0] = 1;
        if (yLen == 0.0) dims[1] = 1;
        if (zLen == 0.0) dims[2] = 1;
        if (vtkStructuredGrid::SafeDownCast(ds) != NULL) {
            vtkStructuredGrid::SafeDownCast(ds)->GetDimensions(dims);
        } else if (vtkRectilinearGrid::SafeDownCast(ds) != NULL) {
            vtkRectilinearGrid::SafeDownCast(ds)->GetDimensions(dims);
        }
        TRACE("Generating volume with dims (%d,%d,%d) from %d points",
              dims[0], dims[1], dims[2], ds->GetNumberOfPoints());

        double xSpacing = (dims[0] == 1 ? 0.0 : xLen/((double)(dims[0]-1)));
        double ySpacing = (dims[1] == 1 ? 0.0 : yLen/((double)(dims[1]-1)));
        double zSpacing = (dims[2] == 1 ? 0.0 : zLen/((double)(dims[2]-1)));

        vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
        imageData->SetDimensions(dims[0], dims[1], dims[2]);
        imageData->SetOrigin(bounds[0], bounds[2], bounds[4]);
        imageData->SetSpacing(xSpacing, ySpacing, zSpacing); 

        vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();
        probe->SetInputData(imageData);
        probe->SetSourceData(ds);
        probe->Update();

        TRACE("Done generating volume");

#ifdef USE_GPU_RAYCAST_MAPPER
        _volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        vtkGPUVolumeRayCastMapper::SafeDownCast(_volumeMapper)->AutoAdjustSampleDistancesOff();
#else
        _volumeMapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
#endif
        _volumeMapper->SetInputConnection(probe->GetOutputPort());
        vtkVolumeMapper::SafeDownCast(_volumeMapper)->SetBlendModeToComposite();
    } else {
        // Unstructured grid with cells (not a cloud)
        vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(ds);
        assert(ugrid != NULL);
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
            TRACE("Decomposing grid to tets");
            filter->Update();
            TRACE("Decomposing done");
            _volumeMapper->SetInputConnection(filter->GetOutputPort());
        }

        vtkUnstructuredGridVolumeMapper::SafeDownCast(_volumeMapper)->
            SetBlendModeToComposite();
    }

    TRACE("Using mapper type: %s", _volumeMapper->GetClassName());

    initProp();

    if (_colorMap == NULL) {
        setColorMap(ColorMap::getVolumeDefault());
    }

    setSampleDistance(getAverageSpacing());

    getVolume()->SetMapper(_volumeMapper);
    _volumeMapper->Update();
}

void Volume::updateRanges(Renderer *renderer)
{
    GraphicsObject::updateRanges(renderer);

    if (getVolume() != NULL) {
        getVolume()->GetProperty()->SetColor(_colorMap->getColorTransferFunction(_dataRange));
#ifdef USE_GPU_RAYCAST_MAPPER
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange, _opacity));
#else
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange));
#endif
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
#ifdef USE_GPU_RAYCAST_MAPPER
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange, _opacity));
#else
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange));
#endif
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
#ifdef USE_GPU_RAYCAST_MAPPER
        getVolume()->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(_dataRange, opacity));
#else
        if (opacity < 1.0e-6)
            opacity = 1.0e-6;
        getVolume()->GetProperty()->SetScalarOpacityUnitDistance(1.0/opacity);
#endif
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

/**
 * \brief Set Volume renderer blending mode
 */
void Volume::setBlendMode(BlendMode mode)
{
    if (_volumeMapper == NULL)
        return;

    vtkVolumeMapper *mapper = vtkVolumeMapper::SafeDownCast(_volumeMapper);
    if (mapper == NULL) {
        vtkUnstructuredGridVolumeMapper *ugmapper = vtkUnstructuredGridVolumeMapper::SafeDownCast(_volumeMapper);
        if (ugmapper == NULL) {
            TRACE("Mapper does not support BlendMode");
            return;
        }
        switch (mode) {
        case BLEND_COMPOSITE:
            ugmapper->SetBlendModeToComposite();
            break;
        case BLEND_MAX_INTENSITY:
            ugmapper->SetBlendModeToMaximumIntensity();
            break;
        case BLEND_MIN_INTENSITY:
        case BLEND_ADDITIVE:
        default:
            ERROR("Unknown BlendMode");
        }
    } else {
        switch (mode) {
        case BLEND_COMPOSITE:
            mapper->SetBlendModeToComposite();
            break;
        case BLEND_MAX_INTENSITY:
            mapper->SetBlendModeToMaximumIntensity();
            break;
        case BLEND_MIN_INTENSITY:
            mapper->SetBlendModeToMinimumIntensity();
            break;
        case BLEND_ADDITIVE:
            mapper->SetBlendModeToAdditive();
            break;
        default:
            ERROR("Unknown BlendMode");
        }
    }
}
