/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
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
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkTransform.h>

#include "LIC.h"
#include "Trace.h"

using namespace VtkVis;

LIC::LIC() :
    GraphicsObject(),
    _sliceAxis(Z_AXIS),
    _colorMap(NULL),
    _resolution(128)
{
}

LIC::~LIC()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting LIC for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting LIC with NULL DataSet");
#endif
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
         TRACE("No vector point data in dataset %s", _dataSet->getName().c_str());
         if (ds->GetCellData() != NULL &&
             ds->GetCellData()->GetVectors() != NULL) {
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
             USER_ERROR("No vector field was found in the data set");
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
#ifdef USE_VTK6
            _volumeSlicer->SetInputData(ds);
#else
            _volumeSlicer->SetInput(ds);
#endif
            _volumeSlicer->SetVOI(0, dims[0]-1, 0, dims[1]-1, (dims[2]-1)/2, (dims[2]-1)/2);
            _volumeSlicer->SetSampleRate(1, 1, 1);
            _lic->SetInputConnection(_volumeSlicer->GetOutputPort());
        } else {
            // 2D image/uniform grid
#ifdef USE_VTK6
            _lic->SetInputData(ds);
#else
            _lic->SetInput(ds);
#endif
        }
        if (_mapper == NULL) {
            _mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        }
        _mapper->SetInputConnection(_lic->GetOutputPort());
    } else if (vtkPolyData::SafeDownCast(ds) == NULL ||
               _dataSet->isCloud() ||
               _dataSet->is2D()) {
        // structured/rectilinear/unstructured grid, cloud or 2D polydata
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

        PrincipalPlane plane;
        double offset;
        if (!_dataSet->is2D(&plane, &offset)) {
            // 3D Data: Sample a plane within the bounding box
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

            if (_dataSet->isCloud()) {
                // 3D cloud -- Need to mesh it before we can resample
                vtkSmartPointer<vtkDelaunay3D> mesher = vtkSmartPointer<vtkDelaunay3D>::New();
#ifdef USE_VTK6
                mesher->SetInputData(ds);
#else
                mesher->SetInput(ds);
#endif
                cutter->SetInputConnection(mesher->GetOutputPort());
            } else {
#ifdef USE_VTK6
                cutter->SetInputData(ds);
#else
                cutter->SetInput(ds);
#endif
            }
            cutter->SetCutFunction(_cutPlane);
            _probeFilter->SetSourceConnection(cutter->GetOutputPort());
        } else {
            if (_dataSet->isCloud()) {
                // 2D cloud -- Need to mesh it before we can resample
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
                _probeFilter->SetSourceConnection(mesher->GetOutputPort());
            } else {
#ifdef USE_VTK6
                _probeFilter->SetSourceData(ds);
#else
                _probeFilter->SetSource(ds);
#endif
            }
        }

        vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
        int xdim, ydim, zdim;
        double origin[3], spacing[3];
        int res = _resolution;
        if (minDir == 0) {
            xdim = 1;
            ydim = res;
            zdim = res;
            origin[0] = bounds[0] + xSize/2.;
            origin[1] = bounds[2];
            origin[2] = bounds[4];
            spacing[0] = 0;
            spacing[1] = ySize/((double)(ydim-1));
            spacing[2] = zSize/((double)(zdim-1));
            _sliceAxis = X_AXIS;
        } else if (minDir == 1) {
            xdim = res;
            ydim = 1;
            zdim = res;
            origin[0] = bounds[0];
            origin[1] = bounds[2] + ySize/2.;
            origin[2] = bounds[4];
            spacing[0] = xSize/((double)(xdim-1));
            spacing[1] = 0;
            spacing[2] = zSize/((double)(zdim-1));
            _sliceAxis = Y_AXIS;
        } else {
            xdim = res;
            ydim = res;
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
#ifdef USE_VTK6
        _probeFilter->SetInputData(imageData);
#else
        _probeFilter->SetInput(imageData);
#endif
        _lic->SetInputConnection(_probeFilter->GetOutputPort());

        if (_mapper == NULL) {
            _mapper = vtkSmartPointer<vtkDataSetMapper>::New();
            _mapper->SetColorModeToMapScalars();
        }
        _lic->Update();
        if (1) {
            vtkSmartPointer<vtkImageData> imgData = _probeFilter->GetImageDataOutput();
            imgData->GetPointData()->SetActiveScalars(_probeFilter->GetValidPointMaskArrayName());
            vtkSmartPointer<vtkImageCast> maskCast = vtkSmartPointer<vtkImageCast>::New();
            maskCast->SetInputData(imgData);
            //maskCast->SetInputConnection(_probeFilter->GetOutputPort());
            //maskCast->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
            //                                 _probeFilter->GetValidPointMaskArrayName());
            maskCast->SetOutputScalarTypeToUnsignedChar();
            vtkSmartPointer<vtkImageCast> licCast = vtkSmartPointer<vtkImageCast>::New();
            licCast->SetInputConnection(_lic->GetOutputPort());
            licCast->SetOutputScalarTypeToDouble();
            vtkSmartPointer<vtkImageMask> mask = vtkSmartPointer<vtkImageMask>::New();
            mask->SetInputConnection(0, licCast->GetOutputPort());
            mask->SetInputConnection(1, maskCast->GetOutputPort());
            _mapper->SetInputConnection(mask->GetOutputPort());
        } else {
            // Mask not working in VTK 6.1?
            _mapper->SetInputConnection(_lic->GetOutputPort());
        }
    } else {
        // DataSet is a 3D PolyData with cells
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
#ifdef USE_VTK6
        ppdmapper->SetInputData(pd);
#else
        ppdmapper->SetInput(pd);
#endif
    }

    setInterpolateBeforeMapping(true);

    if (_lut == NULL) {
        setColorMap(ColorMap::getGrayDefault());
    }

    initProp();
    getActor()->SetMapper(_mapper);

    _mapper->Update();
}

void LIC::setInterpolateBeforeMapping(bool state)
{
    if (_mapper != NULL) {
        _mapper->SetInterpolateScalarsBeforeMapping((state ? 1 : 0));
    }
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
        int dim = _resolution;

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
    _lut->Modified();
}

void LIC::updateRanges(Renderer *renderer)
{
    GraphicsObject::updateRanges(renderer);

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