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

#include "RpLIC.h"
#include "Trace.h"
#include "RpVtkRenderServer.h"

using namespace Rappture::VtkVis;

LIC::LIC() :
    _dataSet(NULL),
    _edgeWidth(1.0f),
    _opacity(1.0),
    _lighting(false),
    _sliceAxis(Z_AXIS)
{
    _edgeColor[0] = 0.0f;
    _edgeColor[1] = 0.0f;
    _edgeColor[2] = 0.0f;
}

LIC::~LIC()
{
}

/**
 * \brief Get the VTK Prop for the LIC
 */
vtkProp *LIC::getProp()
{
    return _prop;
}

void LIC::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkActor>::New();
        _prop->GetProperty()->SetOpacity(_opacity);
        _prop->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
        _prop->GetProperty()->SetLineWidth(_edgeWidth);
        _prop->GetProperty()->EdgeVisibilityOff();
        _prop->GetProperty()->SetAmbient(.2);
        if (!_lighting)
            _prop->GetProperty()->LightingOff();
    }
}

/**
 * \brief Specify input DataSet
 *
 * The DataSet must contain vectors
 */
void LIC::setDataSet(DataSet *dataSet)
{
    if (_dataSet != dataSet) {
        _dataSet = dataSet;
        update();
    }
}

/**
 * \brief Returns the DataSet this LIC renders
 */
DataSet *LIC::getDataSet()
{
    return _dataSet;
}

void LIC::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    double dataRange[2];
    _dataSet->getDataRange(dataRange);

    TRACE("DataSet type: %s, range: %g - %g", _dataSet->getVtkType(),
          dataRange[0], dataRange[1]);

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
        // Sample a plane within the grid bounding box
        if (_probeFilter == NULL)
            _probeFilter = vtkSmartPointer<vtkProbeFilter>::New();
        _probeFilter->SetSource(ds);
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
        }
        _mapper->SetInputConnection(_lic->GetOutputPort());
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

    initProp();
    _prop->SetMapper(_mapper);

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
        vtkDataSet::SafeDownCast(_probeFilter->GetSource())->GetBounds(bounds);
        int dim = 128;

        switch (axis) {
        case X_AXIS:
            imageData->SetDimensions(1, dim, dim);
            imageData->SetOrigin(bounds[0] + (bounds[1]-bounds[0])*ratio, bounds[2], bounds[4]);
            imageData->SetSpacing(0,
                                  (bounds[3]-bounds[2])/((double)(dim-1)), 
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            break;
        case Y_AXIS:
            imageData->SetDimensions(dim, 1, dim);
            imageData->SetOrigin(bounds[0], bounds[2] + (bounds[3]-bounds[2])*ratio, bounds[4]);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  0,
                                  (bounds[5]-bounds[4])/((double)(dim-1)));
            break;
        case Z_AXIS:
            imageData->SetDimensions(dim, dim, 1);
            imageData->SetOrigin(bounds[0], bounds[2], bounds[4] + (bounds[5]-bounds[4])*ratio);
            imageData->SetSpacing((bounds[1]-bounds[0])/((double)(dim-1)), 
                                  (bounds[3]-bounds[2])/((double)(dim-1)),
                                  0);
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

    if (_mapper != NULL)
        _mapper->Update();
}

/**
 * \brief Get the VTK colormap lookup table in use
 */
vtkLookupTable *LIC::getLookupTable()
{ 
    return _lut;
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void LIC::setLookupTable(vtkLookupTable *lut)
{
    if (lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
    } else {
        _lut = lut;
    }

    if (_mapper != NULL) {
        _mapper->SetLookupTable(_lut);
    }
}

/**
 * \brief Turn on/off rendering of this LIC
 */
void LIC::setVisibility(bool state)
{
    if (_prop != NULL) {
        _prop->SetVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Get visibility state of the LIC
 * 
 * \return Is the LIC texture visible?
 */
bool LIC::getVisibility()
{
    if (_prop == NULL) {
        return false;
    } else {
        return (_prop->GetVisibility() != 0);
    }
}

/**
 * \brief Set opacity used to render the LIC
 */
void LIC::setOpacity(double opacity)
{
    _opacity = opacity;
    if (_prop != NULL) {
        _prop->GetProperty()->SetOpacity(opacity);
    }
}

/**
 * \brief Get opacity used to render the LIC
 */
double LIC::getOpacity()
{
    return _opacity;
}

/**
 * \brief Turn on/off rendering of edges
 */
void LIC::setEdgeVisibility(bool state)
{
    if (_prop != NULL) {
        _prop->GetProperty()->SetEdgeVisibility((state ? 1 : 0));
    }
}

/**
 * \brief Set RGB color of edges
 */
void LIC::setEdgeColor(float color[3])
{
    _edgeColor[0] = color[0];
    _edgeColor[1] = color[1];
    _edgeColor[2] = color[2];
    if (_prop != NULL)
        _prop->GetProperty()->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
}

/**
 * \brief Set pixel width of edges (may be a no-op)
 */
void LIC::setEdgeWidth(float edgeWidth)
{
    _edgeWidth = edgeWidth;
    if (_prop != NULL)
        _prop->GetProperty()->SetLineWidth(_edgeWidth);
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

/**
 * \brief Turn on/off lighting of this object
 */
void LIC::setLighting(bool state)
{
    _lighting = state;
    if (_prop != NULL)
        _prop->GetProperty()->SetLighting((state ? 1 : 0));
}
