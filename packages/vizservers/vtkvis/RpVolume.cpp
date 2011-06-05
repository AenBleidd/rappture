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
    _dataSet(NULL),
    _opacity(1.0),
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
 * \brief Specify input DataSet with scalars
 *
 * Currently the DataSet must be image data (2D uniform grid),
 * or an UnstructuredGrid
 */
void Volume::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Returns the DataSet this Volume renders
 */
DataSet *Volume::getDataSet()
{
    return _dataSet;
}

/**
 * \brief Internal method to set up volume mapper after a state change
 */
void Volume::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    double dataRange[2];
    _dataSet->getDataRange(dataRange);

    TRACE("DataSet type: %s, range: %g - %g", _dataSet->getVtkType(),
          dataRange[0], dataRange[1]);

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
        _colorMap = ColorMap::getVolumeDefault();
    }

    _volumeProp->GetProperty()->SetColor(_colorMap->getColorTransferFunction(dataRange));
    _volumeProp->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(dataRange));

    _volumeProp->SetMapper(_volumeMapper);
    _volumeMapper->Update();
}

/**
 * \brief Get the VTK Prop for the Volume
 */
vtkProp *Volume::getProp()
{
    return _volumeProp;
}

/**
 * \brief Create and initialize a VTK Prop to render the Volume
 */
void Volume::initProp()
{
    if (_volumeProp == NULL) {
        _volumeProp = vtkSmartPointer<vtkVolume>::New();
        _volumeProp->GetProperty()->SetInterpolationTypeToLinear();
    }
}

/**
 * \brief Assign a color map (transfer function) to use in rendering the Volume
 */
void Volume::setColorMap(ColorMap *cmap)
{
    _colorMap = cmap;
    if (_volumeProp != NULL) {
        double dataRange[2];
        _dataSet->getDataRange(dataRange);
        _volumeProp->GetProperty()->SetColor(_colorMap->getColorTransferFunction(dataRange));
        _volumeProp->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(dataRange));
    }
}

/**
 * \brief Assign a color map (transfer function) to use in rendering the Volume and
 * specify a scalar range for the map
 */
void Volume::setColorMap(ColorMap *cmap, double dataRange[2])
{
    _colorMap = cmap;
    if (_volumeProp != NULL) {
        _volumeProp->GetProperty()->SetColor(_colorMap->getColorTransferFunction(dataRange));
        _volumeProp->GetProperty()->SetScalarOpacity(_colorMap->getOpacityTransferFunction(dataRange));
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
    if (_volumeProp != NULL) {
        if (opacity < 1.0e-6)
            opacity = 1.0e-6;
        _volumeProp->GetProperty()->SetScalarOpacityUnitDistance(1.0/opacity);
    }
}

/**
 * \brief Turn on/off rendering of this Volume
 */
void Volume::setVisibility(bool state)
{
    if (_volumeProp != NULL) {
        _volumeProp->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the Volume
 * 
 * \return Is PseudoColor visible?
 */
bool Volume::getVisibility()
{
    if (_volumeProp == NULL) {
        return false;
    } else {
        return (_volumeProp->GetVisibility() != 0);
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
 * \brief Set the ambient lighting/shading coefficient
 */
void Volume::setAmbient(double coeff)
{
    if (_volumeProp != NULL) {
        _volumeProp->GetProperty()->SetAmbient(coeff);
    }
}

/**
 * \brief Set the diffuse lighting/shading coefficient
 */
void Volume::setDiffuse(double coeff)
{
    if (_volumeProp != NULL) {
        _volumeProp->GetProperty()->SetDiffuse(coeff);
    }
}

/**
 * \brief Set the specular lighting/shading coefficient and power
 */
void Volume::setSpecular(double coeff, double power)
{
    if (_volumeProp != NULL) {
        _volumeProp->GetProperty()->SetSpecular(coeff);
        _volumeProp->GetProperty()->SetSpecularPower(power);
    }
}

/**
 * \brief Turn on/off lighting of this object
 */
void Volume::setLighting(bool state)
{
    if (_volumeProp != NULL)
        _volumeProp->GetProperty()->SetShade((state ? 1 : 0));
}
