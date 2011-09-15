/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>

#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkPainterPolyDataMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkCutter.h>
#include <vtkImageMask.h>
#include <vtkImageCast.h>

#include "RpLIC.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

LIC::LIC() :
    VtkGraphicsObject(),
    _sliceAxis(Z_AXIS),
    _colorMap(NULL)
{
}

LIC::~LIC()
{
}

void LIC::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        vtkProperty *property = getActor()->GetProperty();
        property->SetOpacity(_opacity);
        property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        property->SetLineWidth(_edgeWidth);
        property->EdgeVisibilityOff();
        property->SetAmbient(.2);
        if (!_lighting)
            property->LightingOff();
    }
}

void LIC::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    vtkSmartPointer<vtkCellDataToPointData> cellToPtData;
    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetVectors() == NULL) {
         WARN("No vector point data in dataset %s", _dataSet->getName().c_str());
         if (ds->GetCellData() != NULL &&
             ds->GetCellData()->GetVectors() != NULL) {
             cellToPtData = 
                 vtkSmartPointer<vtkCellDataToPointData>::New();
             cellToPtData->SetInput(ds);
             //cellToPtData->PassCellDataOn();
             cellToPtData->Update();
             ds = cellToPtData->GetOutput();
         } else {
             ERROR("No vector cell data in dataset %s", _dataSet->getName().c_str());
             return;
         }
    }

    vtkImageData *imageData = vtkImageData::SafeDownCast(ds);
    if (imageData != NULL) {
        if (_lic == NULL) {
            _lic = vtkSmartPointer<vtkImageDataLIC2D>::New();
        }
        if (!_dataSet->is2D()) {
            // 3D image/volume/uniform grid
            if (_volumeSlicer == NULL)
                _volumeSlicer = vtkSmartPointer<vtkExtractVOI>::New();
            int dims[3];
            imageData->GetDimensions(dims);
            TRACE("Image data dimensions: %d %d %d", dims[0], dims[1], dims[2]);
            _volumeSlicer->SetInput(ds);
            _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, (dims[2]-1)/2, (dims[2]-1)/2);
            _volumeSlicer->SetSampleRate(1, 1, 1);
            _lic->SetInputConnection(_volumeSlicer->GetOutputPort());
        } else {
            // 2D image/volume/uniform grid
            _lic->SetInput(ds);
        }
        if (_mapper == NULL) {
            _mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        }
        _mapper->SetInputConnection(_lic->GetOutputPort());
    } else if (vtkPolyData::SafeDownCast(ds) == NULL) {
        // structured grid, unstructured grid, or rectilinear grid
        if (_lic == NULL) {
            _lic = vtkSmartPointer<vtkImageDataLIC2D>::New();
        }

        // Need to convert to vtkImageData
        double bounds[6];
        ds->GetBounds(bounds);
        double xSize = bounds[1] - bounds[0];
        double ySize = bounds[3] - bounds[2];
        double zSize = bounds[5] - bounds[4];
        int minDir = 2;
        if (xSize < ySize && xSize < zSize)
            minDir = 0;
        if (ySize < xSize && ySize < zSize)
            minDir = 1;
        if (_probeFilter == NULL)
            _probeFilter = vtkSmartPointer<vtkProbeFilter>::New();

        if (!_dataSet->is2D()) {
            // Sample a plane within the grid bounding box
            vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
            if (_cutPlane == NULL) {
                _cutPlane = vtkSmartPointer<vtkPlane>::New();
            }
            if (minDir == 0) {
                _cutPlane->SetNormal(1, 0, 0);
                _cutPlane->SetOrigin(bounds[0] + (bounds[1]-bounds[0])/2.,
                                     0,
                                     0);
            } else if (minDir == 1) {
                _cutPlane->SetNormal(0, 1, 0);
                _cutPlane->SetOrigin(0,
                                     bounds[2] + (bounds[3]-bounds[2])/2.,
                                     0);
            } else {
                _cutPlane->SetNormal(0, 0, 1);
                _cutPlane->SetOrigin(0,
                                     0,
                                     bounds[4] + (bounds[5]-bounds[4])/2.);
            }
            cutter->SetInput(ds);
            cutter->SetCutFunction(_cutPlane);
            _probeFilter->SetSourceConnection(cutter->GetOutputPort());
        } else {
            _probeFilter->SetSource(ds);
        }

        vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
        int xdim, ydim, zdim;
        double origin[3], spacing[3];
        if (minDir == 0) {
            xdim = 1;
            ydim = 128;
            zdim = 128;
            origin[0] = bounds[0] + xSize/2.;
            origin[1] = bounds[2];
            origin[2] = bounds[4];
            spacing[0] = 0;
            spacing[1] = ySize/((double)(ydim-1));
            spacing[2] = zSize/((double)(zdim-1));
            _sliceAxis = X_AXIS;
        } else if (minDir == 1) {
            xdim = 128;
            ydim = 1;
            zdim = 128;
            origin[0] = bounds[0];
            origin[1] = bounds[2] + ySize/2.;
            origin[2] = bounds[4];
            spacing[0] = xSize/((double)(xdim-1));
            spacing[1] = 0;
            spacing[2] = zSize/((double)(zdim-1));
            _sliceAxis = Y_AXIS;
        } else {
            xdim = 128;
            ydim = 128;
            zdim = 1;
            origin[0] = bounds[0];
            origin[1] = bounds[2];
            origin[2] = bounds[4] + zSize/2.;
            spacing[0] = xSize/((double)(xdim-1));
            spacing[1] = ySize/((double)(ydim-1));
            spacing[2] = 0;
            _sliceAxis = Z_AXIS;
        }
        imageData->SetDimensions(xdim, ydim, zdim);
        imageData->SetOrigin(origin);
        imageData->SetSpacing(spacing);
        _probeFilter->SetInput(imageData);
        _lic->SetInputConnection(_probeFilter->GetOutputPort());

        if (_mapper == NULL) {
            _mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            _mapper->SetColorModeToMapScalars();
        }
        _lic->Update();
        vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
        cast->SetInputConnection(_probeFilter->GetOutputPort());
        cast->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                     _probeFilter->GetValidPointMaskArrayName());
        cast->SetOutputScalarTypeToUnsignedChar();
        vtkSmartPointer<vtkImageMask> mask = vtkSmartPointer<vtkImageMask>::New();
        mask->SetInputConnection(0, _lic->GetOutputPort());
        mask->SetInputConnection(1, cast->GetOutputPort());
        _mapper->SetInputConnection(mask->GetOutputPort());
    } else {
        // DataSet is a PolyData
        vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
        assert(pd != NULL);

        // XXX: This mapper seems to conflict with offscreen rendering, so the main
        // renderer needs to be set not to use FBOs for this to work
        _mapper = vtkSmartPointer<vtkPainterPolyDataMapper>::New();
        vtkPainterPolyDataMapper *ppdmapper = vtkPainterPolyDataMapper::SafeDownCast(_mapper);
        if (_painter == NULL) {
            _painter = vtkSmartPointer<vtkSurfaceLICPainter>::New();
            _painter->SetDelegatePainter(ppdmapper->GetPainter());
         }
        ppdmapper->SetPainter(_painter);
        ppdmapper->SetInput(pd);
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getGrayDefault());
    }

    initProp();
    getActor()->SetMapper(_mapper);

    _mapper->Update();
#ifdef WANT_TRACE
    if (_lic != NULL) {
        TRACE("FBO status: %d LIC status: %d", _lic->GetFBOSuccess(), _lic->GetLICSuccess());
    } else if (_painter != NULL) {
        TRACE("LIC status: %d", _painter->GetLICSuccess());
    }
#endif
}

/**
 * \brief Select a 2D slice plane from a 3D DataSet
 *
 * \param[in] axis Axis of slice plane
 * \param[in] ratio Position [0,1] of slice plane along axis
 */
void LIC::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if (_volumeSlicer == NULL &&
        _probeFilter == NULL) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    int dims[3];

    vtkImageData *imageData = vtkImageData::SafeDownCast(_dataSet->getVtkDataSet());
    if (imageData == NULL) {
        if (_probeFilter != NULL) {
            imageData = vtkImageData::SafeDownCast(_probeFilter->GetInput());
            if (imageData == NULL) {
                ERROR("Couldn't get probe filter input image");
                return;
            }
        } else {
            ERROR("Not a volume data set");
            return;
        }
    }
    imageData->GetDimensions(dims);

    _sliceAxis = axis;

    if (_probeFilter != NULL) {
        vtkImageData *imageData = vtkImageData::SafeDownCast(_probeFilter->GetInput());
        double bounds[6];
        assert(vtkDataSet::SafeDownCast(_probeFilter->GetSource()) != NULL);
        _dataSet->getBounds(bounds);
        int dim = 128;

        switch (axis) {
        case X_AXIS:
            imageData->SetDimensions(1, dim, dim);
            imageData->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, bounds[2], bounds[4]);
            imageData->SetSpacing(0,
                                  (bounds[3]-bounds[2])/((double)(dim-1)), 
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            if (_cutPlane != NULL) {
                _cutPlane->SetNormal(1, 0, 0);
                _cutPlane->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, 0, 0);
            }
            break;
        case Y_AXIS:
            imageData->SetDimensions(dim, 1, dim);
            imageData->SetOrigin(bounds[0], bounds[2] + (bounds[3]-bounds[2])*ratio, bounds[4]);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  0,
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            if (_cutPlane != NULL) {
                _cutPlane->SetNormal(0, 1, 0);
                _cutPlane->SetOrigin(0, bounds[2] + (bounds[3]-bounds[2])*ratio, 0);
            }
            break;
        case Z_AXIS:
            imageData->SetDimensions(dim, dim, 1);
            imageData->SetOrigin(bounds[0], bounds[2], bounds[4] + (bounds[5]-bounds[4])*ratio);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  (bounds[3]-bounds[2])/((double)(dim-1)),
                                  0);
            if (_cutPlane != NULL) {
                _cutPlane->SetNormal(0, 0, 1);
                _cutPlane->SetOrigin(0, 0, bounds[4] + (bounds[5]-bounds[4])*ratio);
            }
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }
    } else {
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
            voi[2] = voi[3] = (int)((dims[1]-1) * ratio);
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[4] = 0;
            voi[5] = dims[2]-1;
            break;
        case Z_AXIS:
            voi[4] = voi[5] = (int)((dims[2]-1) * ratio);
            voi[0] = 0;
            voi[1] = dims[0]-1;
            voi[2] = 0;
            voi[3] = dims[1]-1;
            break;
        default:
            ERROR("Invalid Axis");
            return;
        }

        _volumeSlicer->SetVOI(voi);
    }

    if (_lic != NULL)
        _lic->Update();

    if (_mapper != NULL)
        _mapper->Update();
}

/**
 * \brief Called when the color map has been edited
 */
void LIC::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void LIC::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL)
        return;

    _colorMap = cmap;
 
    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        if (_mapper != NULL) {
            _mapper->UseLookupTableScalarRangeOff();
            _mapper->SetLookupTable(_lut);
        }
    }

    _lut->DeepCopy(cmap->getLookupTable());
    _lut->SetRange(_dataRange);
}

void LIC::updateRanges(bool useCumulative,
                       double scalarRange[2],
                       double vectorMagnitudeRange[2],
                       double vectorComponentRange[3][2])
{
    if (useCumulative) {
        _dataRange[0] = scalarRange[0];
        _dataRange[1] = scalarRange[1];
    } else if (_dataSet != NULL) {
        _dataSet->getScalarRange(_dataRange);
    }

    if (_lut != NULL) {
        _lut->SetRange(_dataRange);
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void LIC::setClippingPlanes(vtkPlaneCollection *planes)
{
    if (_mapper != NULL) {
        _mapper->SetClippingPlanes(planes);
    }
}
