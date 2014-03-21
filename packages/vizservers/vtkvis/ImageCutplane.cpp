/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cassert>
#include <cfloat>
#include <cstring>

#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkImageData.h>
#include <vtkProbeFilter.h>
#include <vtkGaussianSplatter.h>
#include <vtkLookupTable.h>
#include <vtkTransform.h>
#include <vtkExtractVOI.h>
#include <vtkDataSetSurfaceFilter.h>

#include "ImageCutplane.h"
#include "Renderer.h"
#include "Trace.h"

using namespace VtkVis;

ImageCutplane::ImageCutplane() :
    GraphicsObject(),
    _pipelineInitialized(false),
    _colorMap(NULL),
    _renderer(NULL)
{
}

ImageCutplane::~ImageCutplane()
{
#ifdef WANT_TRACE
    if (_dataSet != NULL)
        TRACE("Deleting ImageCutplane for %s", _dataSet->getName().c_str());
    else
        TRACE("Deleting ImageCutplane with NULL DataSet");
#endif
}

/**
 * \brief Create and initialize a VTK Prop to render the object
 */
void ImageCutplane::initProp()
{
    if (_prop == NULL) {
        _prop = vtkSmartPointer<vtkAssembly>::New();

        for (int i = 0; i < 3; i++) {
            _actor[i] = vtkSmartPointer<vtkImageSlice>::New();
            _borderActor[i] = vtkSmartPointer<vtkActor>::New();
            vtkImageProperty *imgProperty = _actor[i]->GetProperty();
            imgProperty->SetInterpolationTypeToLinear();
            imgProperty->SetBackingColor(_color[0], _color[1], _color[2]);
            imgProperty->BackingOff();
            imgProperty->SetOpacity(_opacity);
            imgProperty->SetAmbient(0.0);
            imgProperty->SetDiffuse(1.0);
            vtkProperty *property = _borderActor[i]->GetProperty();
            property->SetColor(_color[0], _color[1], _color[2]);
            property->SetEdgeColor(_edgeColor[0], _edgeColor[1], _edgeColor[2]);
            property->SetLineWidth(_edgeWidth);
            property->SetPointSize(_pointSize);
            property->EdgeVisibilityOff();
            property->SetOpacity(_opacity);
            property->LightingOff();
            
            getAssembly()->AddPart(_borderActor[i]);
        }
    } else {
        for (int i = 0; i < 3; i++) {
            getAssembly()->RemovePart(_actor[i]);
        }
    }
}

void ImageCutplane::setInterpolationType(InterpType type)
{
    for (int i = 0; i < 3; i++) {
        switch (type) {
        case INTERP_NEAREST:
            _actor[i]->GetProperty()->SetInterpolationTypeToNearest();
            break;
        case INTERP_LINEAR:
            _actor[i]->GetProperty()->SetInterpolationTypeToLinear();
            break;
        case INTERP_CUBIC:
            _actor[i]->GetProperty()->SetInterpolationTypeToCubic();
            break;
        }
    }
}

/**
 * \brief Set the material color (sets ambient, diffuse, and specular)
 */
void ImageCutplane::setColor(float color[3])
{
    GraphicsObject::setColor(color);

    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetBackingColor(color[0], color[1], color[2]);
        }
        if (_borderActor[i] != NULL) {
            _borderActor[i]->GetProperty()->SetColor(color[0], color[1], color[2]);
        }
    }
}

void ImageCutplane::setAmbient(double ambient)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetAmbient(ambient);
        }
    }
}

void ImageCutplane::setDiffuse(double diffuse)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetDiffuse(diffuse);
        }
    }
}

void ImageCutplane::setOpacity(double opacity)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetOpacity(opacity);
        }
    }
}

void ImageCutplane::update()
{
    if (_dataSet == NULL)
        return;

    vtkDataSet *ds = _dataSet->getVtkDataSet();

    if (_dataSet->is2D()) {
        USER_ERROR("Image cutplane requires a 3D data set");
        _dataSet = NULL;
        return;
    }

    if (ds->GetPointData() == NULL ||
        ds->GetPointData()->GetScalars() == NULL) {
        USER_ERROR("No scalar field was found in the data set");
        return;
    }

    double bounds[6];
    _dataSet->getBounds(bounds);
    // Mapper, actor to render color-mapped data set
    for (int i = 0; i < 3; i++) {
        if (_cutPlane[i] == NULL) {
            _cutPlane[i] = vtkSmartPointer<vtkPlane>::New();
            switch (i) {
            case 0:
                _cutPlane[i]->SetNormal(1, 0, 0);
                _cutPlane[i]->SetOrigin(bounds[0] + (bounds[1]-bounds[0])/2.,
                                        0,
                                        0);
                break;
            case 1:
                _cutPlane[i]->SetNormal(0, 1, 0);
                _cutPlane[i]->SetOrigin(0,
                                        bounds[2] + (bounds[3]-bounds[2])/2.,
                                        0);
                break;
            case 2:
            default:
                _cutPlane[i]->SetNormal(0, 0, 1);
                _cutPlane[i]->SetOrigin(0,
                                        0,
                                        bounds[4] + (bounds[5]-bounds[4])/2.);
                break;
            }
        }
        if (_mapper[i] == NULL) {
            _mapper[i] = vtkSmartPointer<vtkImageResliceMapper>::New();
            _mapper[i]->SetSlicePlane(_cutPlane[i]);
        }
    }

    if (!_pipelineInitialized) {
        vtkImageData *imgData = vtkImageData::SafeDownCast(ds);
        if (imgData == NULL) {
            // Need to resample to ImageData
            if (_dataSet->isCloud()) {
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

                for (int i = 0; i < 3; i++) {
                    _mapper[i]->SetInputConnection(splatter->GetOutputPort());
                }
            } else {
                // (Slow) Resample using ProbeFilter
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

                vtkSmartPointer<vtkImageData> resampleGrid = vtkSmartPointer<vtkImageData>::New();
                resampleGrid->SetDimensions(dims[0], dims[1], dims[2]);
                resampleGrid->SetOrigin(bounds[0], bounds[2], bounds[4]);
                resampleGrid->SetSpacing(xSpacing, ySpacing, zSpacing); 

                vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();
#ifdef USE_VTK6
                probe->SetInputData(resampleGrid);
                probe->SetSourceData(ds);
#else
                probe->SetInput(resampleGrid);
                probe->SetSource(ds);
#endif
                probe->Update();

                TRACE("Done generating volume");

                for (int i = 0; i < 3; i++) {
                    _mapper[i]->SetInputConnection(probe->GetOutputPort());
                }
            }
        } else {
            // Have ImageData
            for (int i = 0; i < 3; i++) {
#ifdef USE_VTK6
                _mapper[i]->SetInputData(imgData);
#else
                _mapper[i]->SetInput(imgData);
#endif
            }
        }
    }

    initProp();

    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL && _borderMapper[i] == NULL) {
            _borderMapper[i] = vtkSmartPointer<vtkPolyDataMapper>::New();
            _outlineSource[i] = vtkSmartPointer<vtkOutlineSource>::New();
            switch (i) {
            case 0:
                _outlineSource[i]->SetBounds(bounds[0] + (bounds[1]-bounds[0])/2.,
                                             bounds[0] + (bounds[1]-bounds[0])/2.,
                                             bounds[2], bounds[3],
                                             bounds[4], bounds[5]);
                break;
            case 1:
                _outlineSource[i]->SetBounds(bounds[0], bounds[1],
                                             bounds[2] + (bounds[3]-bounds[2])/2.,
                                             bounds[2] + (bounds[3]-bounds[2])/2.,
                                             bounds[4], bounds[5]);
                break;
            case 2:
                _outlineSource[i]->SetBounds(bounds[0], bounds[1],
                                             bounds[2], bounds[3],
                                             bounds[4] + (bounds[5]-bounds[4])/2.,
                                             bounds[4] + (bounds[5]-bounds[4])/2.);
                break;
            default:
                ;
            }
            _borderMapper[i]->SetInputConnection(_outlineSource[i]->GetOutputPort());
            _borderMapper[i]->SetResolveCoincidentTopologyToPolygonOffset();
        }
    }

    if (_lut == NULL) {
        setColorMap(ColorMap::getDefault());
    }

    _pipelineInitialized = true;

    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _actor[i]->SetMapper(_mapper[i]);
            vtkImageMapper3D *im3d = _mapper[i];
            im3d->Update();
        }
        if (_borderMapper[i] != NULL) {
            _borderActor[i]->SetMapper(_borderMapper[i]);
            _borderMapper[i]->Update();
        }
        // Only add cutter actor to assembly if geometry was
        // produced, in order to prevent messing up assembly bounds
        double bounds[6];
        _actor[i]->GetBounds(bounds);
        if (bounds[0] <= bounds[1]) {
            getAssembly()->AddPart(_actor[i]);
        }
    }
}

/**
 * \brief Select a 2D slice plane from a 3D DataSet
 *
 * \param[in] axis Axis of slice plane
 * \param[in] ratio Position [0,1] of slice plane along axis
 */
void ImageCutplane::selectVolumeSlice(Axis axis, double ratio)
{
    if (_dataSet->is2D()) {
        WARN("DataSet not 3D, returning");
        return;
    }

    if ((axis == X_AXIS &&_cutPlane[0] == NULL) ||
        (axis == Y_AXIS &&_cutPlane[1] == NULL) ||
        (axis == Z_AXIS &&_cutPlane[2] == NULL)) {
        WARN("Called before update() or DataSet is not a volume");
        return;
    }

    double bounds[6];
    _dataSet->getBounds(bounds);
#if 0
    if (ratio == 0.0)
        ratio = 0.001;
    if (ratio == 1.0)
        ratio = 0.999;
#endif
    switch (axis) {
    case X_AXIS:
        _cutPlane[0]->SetOrigin(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                0,
                                0);
        _outlineSource[0]->SetBounds(bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                     bounds[0] + (bounds[1]-bounds[0]) * ratio,
                                     bounds[2], bounds[3],
                                     bounds[4], bounds[5]);
        break;
    case Y_AXIS:
        _cutPlane[1]->SetOrigin(0,
                                bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                0);
        _outlineSource[1]->SetBounds(bounds[0], bounds[1],
                                     bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                     bounds[2] + (bounds[3]-bounds[2]) * ratio,
                                     bounds[4], bounds[5]);
        break;
    case Z_AXIS:
        _cutPlane[2]->SetOrigin(0,
                                0,
                                bounds[4] + (bounds[5]-bounds[4]) * ratio);
        _outlineSource[2]->SetBounds(bounds[0], bounds[1],
                                     bounds[2], bounds[3],
                                     bounds[4] + (bounds[5]-bounds[4]) * ratio,
                                     bounds[4] + (bounds[5]-bounds[4]) * ratio);
        break;
    default:
        ERROR("Invalid Axis");
        return;
    }
    update();
}

void ImageCutplane::updateRanges(Renderer *renderer)
{
    GraphicsObject::updateRanges(renderer);

    updateColorMap();
}

/**
 * \brief Called when the color map has been edited
 */
void ImageCutplane::updateColorMap()
{
    setColorMap(_colorMap);
}

/**
 * \brief Associate a colormap lookup table with the DataSet
 */
void ImageCutplane::setColorMap(ColorMap *cmap)
{
    if (cmap == NULL) {
        _colorMap = NULL;
        _lut = NULL;
        for (int i = 0; i < 3; i++) {
            if (_actor[i] != NULL) {
                _actor[i]->GetProperty()->SetLookupTable(NULL);
            }
        }
        return;
    }

    _colorMap = cmap;

    if (_lut == NULL) {
        _lut = vtkSmartPointer<vtkLookupTable>::New();
        for (int i = 0; i < 3; i++) {
            if (_actor[i] != NULL) {
                _actor[i]->GetProperty()->UseLookupTableScalarRangeOn();
                _actor[i]->GetProperty()->SetLookupTable(_lut);
            }
        }
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(_dataRange);
    } else {
        double range[2];
        _lut->GetTableRange(range);
        _lut->DeepCopy(cmap->getLookupTable());
        _lut->SetRange(range);
        _lut->Modified();
    }
}

void ImageCutplane::setUseWindowLevel(bool state)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->SetUseLookupTableScalarRange((state ? 0 : 1));
        }
    }
}

void ImageCutplane::setWindow(double window)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->UseLookupTableScalarRangeOff();
            _actor[i]->GetProperty()->SetColorWindow(window);
        }
    }
}

void ImageCutplane::setLevel(double level)
{
    for (int i = 0; i < 3; i++) {
        if (_actor[i] != NULL) {
            _actor[i]->GetProperty()->UseLookupTableScalarRangeOff();
            _actor[i]->GetProperty()->SetColorLevel(level);
        }
    }
}

/**
 * \brief Turn on/off rendering of outlines
 */
void ImageCutplane::setOutlineVisibility(bool state)
{
    for (int i = 0; i < 3; i++) {
        if (_borderActor[i] != NULL) {
            _borderActor[i]->SetVisibility((state ? 1 : 0));
        }
    }
}

/**
 * \brief Set visibility of cutplane on specified axis
 */
void ImageCutplane::setSliceVisibility(Axis axis, bool state)
{
    switch (axis) {
    case X_AXIS:
        if (_actor[0] != NULL)
            _actor[0]->SetVisibility((state ? 1 : 0));
        if (_borderActor[0] != NULL)
            _borderActor[0]->SetVisibility((state ? 1 : 0));
        break;
    case Y_AXIS:
        if (_actor[1] != NULL)
            _actor[1]->SetVisibility((state ? 1 : 0));
        if (_borderActor[1] != NULL)
            _borderActor[1]->SetVisibility((state ? 1 : 0));
        break;
    case Z_AXIS:
    default:
        if (_actor[2] != NULL)
            _actor[2]->SetVisibility((state ? 1 : 0));
        if (_borderActor[2] != NULL)
            _borderActor[2]->SetVisibility((state ? 1 : 0));
        break;
    }
}

/**
 * \brief Set a group of world coordinate planes to clip rendering
 *
 * Passing NULL for planes will remove all cliping planes
 */
void ImageCutplane::setClippingPlanes(vtkPlaneCollection *planes)
{
    for (int i = 0; i < 3; i++) {
        if (_mapper[i] != NULL) {
            _mapper[i]->SetClippingPlanes(planes);
        }
        if (_borderMapper[i] != NULL) {
            _borderMapper[i]->SetClippingPlanes(planes);
        }
    }
}
