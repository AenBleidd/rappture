/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

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
 * \brief Internal method to set up pipeline after a state change
 */
void Volume::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (vtkImageData::SafeDownCast(ds) != NULL) {
        // Image data required for these mappers
#ifdef USE_GPU_RAYCAST_MAPPER
        _volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
#else
        _volumeMapper = vtkSmartPointer<vtkVolumeTextureMapper3D>::New();
#endif
        _volumeMapper->SetInput(ds);
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
            _volumeMapper->SetInput(ds);
        } else {
            // Decompose to tetrahedra
            vtkSmartPointer<vtkDataSetTriangleFilter> filter = 
                vtkSmartPointer<vtkDataSetTriangleFilter>::New();
            filter->SetInput(ugrid);
            filter->TetrahedraOnlyOn();
            _volumeMapper->SetInputConnection(filter->GetOutputPort());
        }

        vtkUnstructuredGridVolumeMapper::SafeDownCast(_volumeMapper)->
            SetBlendModeToComposite();
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
